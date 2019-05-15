#include <pci.h>
#include <kernel.h>
#include <mm.h>
#include <init.h>
#include <bitmap.h>

struct nvidia_gpu;

enum nvkm_memory_target {
	NVKM_MEM_TARGET_INST, /* instance memory */
	NVKM_MEM_TARGET_VRAM, /* video memory */
	NVKM_MEM_TARGET_HOST, /* coherent system memory */
	NVKM_MEM_TARGET_NCOH, /* non-coherent system memory */
};

struct nv_inst_obj {
	struct nvidia_gpu *gpu;
	u64 inst_mem_addr;
	u64 size;
	u64 *mapped_ptr;
};

struct nvkm_memory {
	struct list_head zone_list;
	struct bitmap *alloc_bitmap;
	u32 (*read) (u64 offset);
	void (*write) (u64 offset, u32 value);
};

/* memory type/access flags, do not match hardware values */
#define NV_MEM_ACCESS_RO  1
#define NV_MEM_ACCESS_WO  2
#define NV_MEM_ACCESS_RW (NV_MEM_ACCESS_RO | NV_MEM_ACCESS_WO)
#define NV_MEM_ACCESS_SYS 4
#define NV_MEM_ACCESS_VM  8
#define NV_MEM_ACCESS_NOSNOOP 16

#define NV_MEM_LARGE_PAGE 32

#define NV_MEM_TARGET_VRAM        (0 << 8)
#define NV_MEM_TARGET_HOST         (1 << 8)
#define NV_MEM_TARGET_HOST_NOSNOOP (2 << 8)
#define NV_MEM_TARGET_VM          (3 << 8)
#define NV_MEM_TARGET_GART        (4 << 8)

//NV PDE attributes
#define NV_LPT_PRESENT BIT0
#define NV_PT_SIZE_FULL  0
#define NV_PT_SIZE_64M   (1 << 2)
#define NV_PT_SIZE_32M	 (2 << 2)
#define NV_PT_SIZE_16M   (3 << 3)
#define NV_SPT_PRESENT BIT0

//NV PTE attributes
#define NV_PTE_PRESENT BIT0
#define NV_PTE_SUPERVISOR_ONLY BIT1
#define NV_PTE_READ_ONLY BIT2
#define NV_PTE_ENCRYPTED BIT3  //SYSRAM only
#define NV_PTE_TARGET_VRAM (0ULL << 32)
#define NV_PTE_TARGET_SYSRAM_SNOOP (5ULL << 32)
#define NV_PTE_TARGET_SYSRAM_NOSNOOP (7ULL << 32)

#define NV_PAGE_SIZE_128KB		0
#define NV_PAGE_SIZE_4KB		1

#define NVC0_SPAGE_SHIFT        12
#define NVC0_LPAGE_SHIFT        17
#define NVC0_SPAGE_MASK         0x00fff
#define NVC0_LPAGE_MASK         0x1ffff

#define NVC0_VM_PDE_COUNT       0x2000
#define NVC0_VM_BLOCK_SIZE      0x8000000
#define NVC0_VM_BLOCK_MASK      0x7ffffff
#define NVC0_VM_SPTE_COUNT      (NVC0_VM_BLOCK_SIZE >> NVC0_SPAGE_SHIFT)
#define NVC0_VM_LPTE_COUNT      (NVC0_VM_BLOCK_SIZE >> NVC0_LPAGE_SHIFT)

#define NVC0_PDE(a)             ((a) / NVC0_VM_BLOCK_SIZE)
#define NVC0_SPTE(a)            (((a) & NVC0_VM_BLOCK_MASK) >> NVC0_SPAGE_SHIFT)
#define NVC0_LPTE(a)            (((a) & NVC0_VM_BLOCK_MASK) >> NVC0_LPAGE_SHIFT)

#define NVC0_PDE_HT_SIZE 32
#define NVC0_PDE_HASH(n) (n % NVC0_PDE_HT_SIZE)

#define PDE_2_ADDR(a) (((a) & 0xfffffffe) << 8)

struct nvkm_mmu {
	int (*map_page) (struct nv_inst_obj *pgd, u64 virt, u64 phys, u64 page_size);
	int (*unmap) (struct nv_inst_obj *pgd, u64 virt, u64 phys, u64 len);
	int (*flush) ();
	int (*mmu_walk) (struct nv_inst_obj *pgd, u64 virt, u64 *pt);
};

struct gp_fifo {
	struct nv_inst_obj *ramfc;
	struct nv_inst_obj *pgd;
	
};


struct nv_chipset {
	u64 chipset;
	int (*bios_init)(struct nvidia_gpu *gpu);
	u64 (*vram_probe)(struct nvidia_gpu *gpu);
};

struct fb {
	struct nv_inst_obj *fb;
};

struct bar_vm {
	u64 bar_no;
	u64 bar_base;
	u64 bar_size;
	struct nv_inst_obj *inst_mem;
	struct nv_inst_obj *pgd;
};

struct nvidia_gpu {
	struct pci_dev *pdev;
	u64 mmio_base;
	volatile u32 *mmio_virt;
	struct nv_chipset *chipset;

	struct nvkm_memory memory;
	struct nvkm_mmu mmu;
	struct fb fb;
	struct bar_vm bar[2];
};

static inline u32 nvkm_rd32(struct nvidia_gpu *gpu, u64 offset)
{
	return gpu->mmio_virt[offset / 4];
}

static inline void nvkm_wr32(struct nvidia_gpu *gpu, u64 offset, u32 value)
{
	gpu->mmio_virt[offset / 4] = value;
}

static inline u32 instmem_rd32(struct nvidia_gpu *gpu, u64 offset)
{
	u64 base = offset & 0xffffff00000ULL;
	u64 addr = offset & 0x000000fffffULL;
	u32 data;

	//TODO:add spinlock here.

	nvkm_wr32(gpu, 0x001700, base >> 16);
	data = nvkm_rd32(gpu, 0x700000 + addr);

	return data;
}

static inline void instmem_wr32(struct nvidia_gpu *gpu, u64 offset, u32 data)
{
	u64 base = offset & 0xffffff00000ULL;
	u64 addr = offset & 0x000000fffffULL;

	//TODO:add spinlock here.

	nvkm_wr32(gpu, 0x001700, base >> 16);
	nvkm_wr32(gpu, 0x700000 + addr, data);
}

static inline u64 instmem_allocate(struct nvidia_gpu *gpu, u64 len)
{
	u32 pages_allocate = len > 0x1000 ? len / 0x1000 : 1;
	int ret = bitmap_allocate_bits(gpu->memory.alloc_bitmap, pages_allocate);
	if (ret == -1) {
		printk("instmem allocate failed.\n");
		return -1;
	}

	return ret * 0x1000;
}

static inline void instmem_free(struct nvidia_gpu *gpu, u64 offset)
{
	//TODO
}

static inline struct nv_inst_obj *nv_instobj_alloc(struct nvidia_gpu *gpu, u64 size)
{
	u64 addr = instmem_allocate(gpu, size);
	struct nv_inst_obj *obj;
	if (addr == -1) {
		return NULL;
	}
	obj = kmalloc(sizeof(*obj), GFP_KERNEL);
	obj->gpu = gpu;
	obj->inst_mem_addr = addr;
	obj->size = size;
	obj->mapped_ptr = NULL;

	return obj;
}

static inline void nv_instobj_free(struct nv_inst_obj *obj)
{
	instmem_free(obj->gpu, obj->inst_mem_addr);
	kfree(obj);
}

static inline int nv_instobj_rd32(struct nv_inst_obj *obj, u64 offset, u32 *value)
{
	if (offset > obj->inst_mem_addr + obj->size) {
		return -1;
	}
	*value = instmem_rd32(obj->gpu, obj->inst_mem_addr + offset);
	return 0;
}

static inline int nv_instobj_wr32(struct nv_inst_obj *obj, u64 offset, u32 value)
{
	if (offset > obj->inst_mem_addr + obj->size) {
		return -1;
	}
	instmem_wr32(obj->gpu, obj->inst_mem_addr + offset, value);
	return 0;
}


