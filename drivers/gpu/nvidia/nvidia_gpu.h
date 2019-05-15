#include <pci.h>
#include <kernel.h>
#include <mm.h>
#include <init.h>
#include <bitmap.h>

enum nvkm_memory_target {
	NVKM_MEM_TARGET_INST, /* instance memory */
	NVKM_MEM_TARGET_VRAM, /* video memory */
	NVKM_MEM_TARGET_HOST, /* coherent system memory */
	NVKM_MEM_TARGET_NCOH, /* non-coherent system memory */
};

struct nvkm_memory {
	struct list_head zone_list;
	struct bitmap *allocate;
	u32 (*read) (u64 offset);
	void (*write) (u64 offset, u32 value);
};

struct nvkm_mmu {
	int (*map) (u64 virt, u64 phys, u64 len);
	int (*unmap) (u64 virt, u64 phys, u64 len);
	int (*mmu_walk) (u64 virt, u64 *pt);
};

struct gp_fifo {
	
};

struct nvidia_gpu;
struct nv_chipset {
	u64 chipset;
	int (*bios_init)(struct nvidia_gpu *gpu);
	u64 (*vram_probe)(struct nvidia_gpu *gpu);
};


struct nvidia_gpu {
	struct pci_dev *pdev;
	u64 mmio_base;
	volatile u32 *mmio_virt;
	struct nv_chipset *chipset;

	struct nvkm_memory memory;
	struct nvkm_mmu mmu;
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


