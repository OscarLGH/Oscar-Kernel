#include <pci.h>
#include <kernel.h>
#include <mm.h>
#include <init.h>

struct nvkm_memory {
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


struct nvidia_gpu {
	struct pci_dev *pdev;
	u64 mmio_base;
	volatile u32 *mmio_virt;
	u32 chipset;

	struct nvkm_memory memory;
	struct nvkm_mmu mmu;
};

static inline u32 nvkm_rd(struct nvidia_gpu *gpu, u64 offset)
{
	return gpu->mmio_virt[offset / 4];
}

static inline void nvkm_wr(struct nvidia_gpu *gpu, u64 offset, u32 value)
{
	gpu->mmio_virt[offset / 4] = value;
}


