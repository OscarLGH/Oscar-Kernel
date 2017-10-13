#include "../NvidiaGpu.h"

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

VOID * NvMapUser(SUBDEV_MEM * Mem, UINT64 PhysAddr, UINT64 Length);
VOID * NvMapKernel(SUBDEV_MEM * Mem, UINT64 PhysAddr, UINT64 Length);

INT64 VramAllocate(SUBDEV_MEM * Vram, UINT64 Size, UINT64 Align);
INT64 VramZallocate(SUBDEV_MEM * Vram, UINT64 Size, UINT64 Align);
RETURN_STATUS VramFree(SUBDEV_MEM * Vram, UINT64 Offset);
RETURN_STATUS VramRead(NVIDIA_GPU * Gpu, UINT32 * Buffer, UINT64 Length, UINT64 Addr);
RETURN_STATUS VramWrite(NVIDIA_GPU * Gpu, UINT32 * Buffer, UINT64 Length, UINT64 Addr);
RETURN_STATUS NvGpuObjNew(NVIDIA_GPU * Gpu, UINT64 Size, UINT64 Align, NV_GPU_OBJ **Obj);
RETURN_STATUS NvGpuObjFree(NVIDIA_GPU * Gpu, NV_GPU_OBJ **Obj);
UINT32 GpuObjRd32(NV_GPU_OBJ * Obj, UINT64 Offset);
VOID GpuObjWr32(NV_GPU_OBJ * Obj, UINT64 Offset, UINT32 Value);
RETURN_STATUS NvGpuObjCopy(NV_GPU_OBJ *SrcObj, NV_GPU_OBJ *DstObj);

INT64 VmAllocate(SUBDEV_MEM * Mem, UINT64 Size, UINT64 Align, UINT64 Bar);
VOID * VramMap(SUBDEV_MEM * Mem, UINT64 VirtAddr, UINT64 PhysAddr, UINT64 Length, UINT64 Attr, UINT64 Bar);

VOID * NvMapUser(SUBDEV_MEM * Mem, UINT64 PhysAddr, UINT64 Length);
VOID * NvMapKernel(SUBDEV_MEM * Mem, UINT64 PhysAddr, UINT64 Length);
RETURN_STATUS BarVmInit(NVIDIA_GPU * Gpu, BAR_VM * BarVm, UINT64 Bar);