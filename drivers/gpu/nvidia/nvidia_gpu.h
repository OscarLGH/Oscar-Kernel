#include <pci.h>
#include <kernel.h>
#include <mm.h>
#include <init.h>
#include <bitmap.h>
#include <string.h>
#include <math.h>
#include <fb.h>

struct nvidia_gpu;

enum nvkm_memory_target {
	NVKM_MEM_TARGET_INST, /* instance memory */
	NVKM_MEM_TARGET_VRAM, /* video memory */
	NVKM_MEM_TARGET_HOST, /* coherent system memory */
	NVKM_MEM_TARGET_NCOH, /* non-coherent system memory */
};

struct nvkm_gpuobj {
	struct nvidia_gpu *gpu;
	u64 vram_paddr;
	u64 bar_addr;
	u64 *mapped_ptr;
	u64 size;
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
	int (*map_page) (struct nvkm_gpuobj *pgd, u64 virt, u64 phys, u64 page_size);
	int (*unmap) (struct nvkm_gpuobj *pgd, u64 virt, u64 phys, u64 len);
	int (*flush) ();
	int (*mmu_walk) (struct nvkm_gpuobj *pgd, u64 virt, u64 *pt);
};

struct nvkm_fifo_chan {
	struct nvidia_gpu *gpu;
	//const struct nvkm_fifo_chan_func *func;
	//struct nvkm_fifo *fifo;
	u64 engines;
	//struct nvkm_object object;
	u64 id;
	u64 runl;

	struct list_head head;
	struct nvkm_gpuobj *inst;
	struct push_buffer {
		u64 push_paddr;
		u32 *push_vaddr;
		u64 push_vma_bar1;
		u64 size;
	} push;
	//struct nvkm_vmm *vmm;
	struct nvkm_gpuobj *pgd;
	volatile u32 *user;
	u64 addr;
	u32 size;

	struct {
		u32 max;
		u32 free;
		u32 current;
		u32 put;
		u32 ib_base;
		u32 ib_max;
		u32 ib_free;
		u32 ib_put;
	} ib_status_host;

	u32 user_get_high;
	u32 user_get_low;
	u32 user_put;

	struct nvkm_gpuobj *fence;
	u32 fence_sequence;

	//struct nvkm_fifo_engn engn[NVKM_SUBDEV_NR];
};


struct gp_fifo {
	struct nvidia_gpu *gpu;

	struct {
		int runl;
		int pbid;
	} engine[16];
	int engine_nr;

	struct {
		int next;
		struct list_head cgrp;
		struct list_head chan;
		struct nvkm_gpuobj *mem[2];
		u32 engm;
	} runlist[16];
	int runlist_nr;

	struct nvkm_gpuobj *user_mem;
	void *user;

	struct list_head chan_list;
	u64 (*pbdma_nr)(struct nvidia_gpu *gpu);
	u64 (*pbdma_init)(struct nvidia_gpu *gpu);
	
};


struct nv_chipset {
	u64 chipset;
	int (*bios_init)(struct nvidia_gpu *gpu);
	u64 (*vram_probe)(struct nvidia_gpu *gpu);
};

struct fb {
	struct nvkm_gpuobj *fb;
};

struct bar_vm {
	u64 bar_no;
	u64 bar_base;
	u64 bar_size;
	struct bitmap *alloc_bitmap;
	struct nvkm_gpuobj *inst_mem;
	struct nvkm_gpuobj *pgd;
};

struct nvidia_gpu {
	struct pci_dev *pdev;
	u64 mmio_base;
	volatile u32 *mmio_virt;
	struct nv_chipset *chipset;

	struct nvkm_memory memory;
	struct nvkm_mmu mmu;
	struct fb fb;
	struct bar_vm bar[6];

	struct gp_fifo fifo;
	struct nvkm_fifo_chan *memcpy_chan;
};

static inline u32 nvkm_rd32(struct nvidia_gpu *gpu, u64 offset)
{
	return gpu->mmio_virt[offset / 4];
}

static inline void nvkm_wr32(struct nvidia_gpu *gpu, u64 offset, u32 value)
{
	gpu->mmio_virt[offset / 4] = value;
}

#define nvkm_mask(d,a,m,v) ({                                                  \
	struct nvidia_gpu *_device = (d);                                     \
	u32 _addr = (a), _temp = nvkm_rd32(_device, _addr);                    \
	nvkm_wr32(_device, _addr, (_temp & ~(m)) | (v));                       \
	_temp;                                                                 \
})


static inline u32 nvkm_memory_rd32(struct nvidia_gpu *gpu, u64 offset)
{
	u64 base = offset & 0xffffff00000ULL;
	u64 addr = offset & 0x000000fffffULL;
	u32 data;

	//TODO:add spinlock here.

	nvkm_wr32(gpu, 0x001700, base >> 16);
	data = nvkm_rd32(gpu, 0x700000 + addr);

	return data;
}

static inline void nvkm_memory_wr32(struct nvidia_gpu *gpu, u64 offset, u32 data)
{
	u64 base = offset & 0xffffff00000ULL;
	u64 addr = offset & 0x000000fffffULL;

	//TODO:add spinlock here.

	nvkm_wr32(gpu, 0x001700, base >> 16);
	nvkm_wr32(gpu, 0x700000 + addr, data);
}

static inline u64 nvkm_memory_new(struct nvidia_gpu *gpu, u64 len)
{
	u32 pages_allocate = round_up(len, 0x1000) / 0x1000;
	int ret = bitmap_allocate_bits(gpu->memory.alloc_bitmap, pages_allocate);
	if (ret == -1) {
		printk("instmem allocate failed.\n");
		return -1;
	}
	//printk("vram = %x\n", ret * 0x1000);
	return ret * 0x1000;
}

static inline u64 nvkm_vma_new(struct nvidia_gpu *gpu, u64 bar, u64 len)
{
	u32 pages_allocate = round_up(len, 0x1000) / 0x1000;
	int ret = bitmap_allocate_bits(gpu->bar[bar].alloc_bitmap, pages_allocate);
	if (ret == -1) {
		printk("vma allocate failed.\n");
		return -1;
	}
	//printk("vma = %x\n", ret * 0x1000);
	return ret * 0x1000;
}

#define NV_VM_TARGET_VRAM 0x0
#define NV_VM_TARGET_SYSRAM_SNOOP 0x5
#define NV_VM_TARGET_SYSRAM_NOSNOOP 0x7

int nvidia_mmu_map_page(struct nvkm_gpuobj *pgd, u64 virt, u64 phys, u64 priv, u64 type, u64 page_size);

static inline int nvkm_map_area(struct nvkm_gpuobj *pgd, u64 virt, u64 phys, u64 len, u64 attr, u64 type)
{
	int i;
	for (i = 0; i < round_up(len, 0x1000) / 0x1000; i++) {
		nvidia_mmu_map_page(pgd, virt + i * 0x1000, phys + i * 0x1000, attr, type, 0x1000);
	}

	return 0;
}


static inline void instmem_free(struct nvidia_gpu *gpu, u64 offset)
{
	//TODO
}

static inline struct nvkm_gpuobj *nvkm_gpuobj_new(struct nvidia_gpu *gpu, u64 size, bool zero)
{
	u64 addr = nvkm_memory_new(gpu, size);
	int i;
	struct nvkm_gpuobj *obj;
	if (addr == -1) {
		return NULL;
	}
	obj = kmalloc(sizeof(*obj), GFP_KERNEL);
	obj->gpu = gpu;
	obj->vram_paddr = addr;
	obj->size = size;
	obj->mapped_ptr = NULL;

	if (zero) {
		for (i = 0; i < size; i += 4)
			nvkm_memory_wr32(gpu, addr + i, 0x0);
	}

	return obj;
}

static inline void nvkm_gpuobj_free(struct nvkm_gpuobj *obj)
{
	instmem_free(obj->gpu, obj->vram_paddr);
	kfree(obj);
}

static inline int nvkm_gpuobj_rd32(struct nvkm_gpuobj *obj, u64 offset, u32 *value)
{
	if (offset > obj->vram_paddr + obj->size) {
		return -1;
	}
	*value = nvkm_memory_rd32(obj->gpu, obj->vram_paddr + offset);
	return 0;
}

static inline int nvkm_gpuobj_wr32(struct nvkm_gpuobj *obj, u64 offset, u32 value)
{
	if (offset > obj->vram_paddr + obj->size) {
		return -1;
	}
	nvkm_memory_wr32(obj->gpu, obj->vram_paddr + offset, value);
	return 0;
}

void OUT_RING(struct nvkm_fifo_chan *chan, u32 data);

static inline void
BEGIN_NVC0(struct nvkm_fifo_chan *chan, u32 sub_chan, u32 mthd, u16 size)
{
	OUT_RING(chan, 0x20000000 | (size << 16) | (sub_chan << 13) | (mthd >> 2));
}

static inline void
BEGIN_IMC0(struct nvkm_fifo_chan *chan, u32 sub_chan, u32 mthd, u16 data)
{
	OUT_RING(chan, 0x80000000 | (data << 16) | (sub_chan << 13) | (mthd >> 2));
}

/* FIFO methods */
#define NV01_SUBCHAN_OBJECT                                          0x00000000
#define NV84_SUBCHAN_SEMAPHORE_ADDRESS_HIGH                          0x00000010
#define NV84_SUBCHAN_SEMAPHORE_ADDRESS_LOW                           0x00000014
#define NV84_SUBCHAN_SEMAPHORE_SEQUENCE                              0x00000018
#define NV84_SUBCHAN_SEMAPHORE_TRIGGER                               0x0000001c
#define NV84_SUBCHAN_SEMAPHORE_TRIGGER_ACQUIRE_EQUAL                 0x00000001
#define NV84_SUBCHAN_SEMAPHORE_TRIGGER_WRITE_LONG                    0x00000002
#define NV84_SUBCHAN_SEMAPHORE_TRIGGER_ACQUIRE_GEQUAL                0x00000004
#define NVC0_SUBCHAN_SEMAPHORE_TRIGGER_YIELD                         0x00001000
#define NV84_SUBCHAN_UEVENT                                          0x00000020
#define NV84_SUBCHAN_WRCACHE_FLUSH                                   0x00000024
#define NV10_SUBCHAN_REF_CNT                                         0x00000050
#define NV11_SUBCHAN_DMA_SEMAPHORE                                   0x00000060
#define NV11_SUBCHAN_SEMAPHORE_OFFSET                                0x00000064
#define NV11_SUBCHAN_SEMAPHORE_ACQUIRE                               0x00000068
#define NV11_SUBCHAN_SEMAPHORE_RELEASE                               0x0000006c
#define NV40_SUBCHAN_YIELD                                           0x00000080



