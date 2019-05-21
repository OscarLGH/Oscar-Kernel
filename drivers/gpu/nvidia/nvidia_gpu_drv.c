#include "nvidia_gpu.h"

u64 vram_probe_gm107(struct nvidia_gpu *gpu)
{
	u32 fbps = nvkm_rd32(gpu, 0x022438);
	u32 fbpat = nvkm_rd32(gpu, 0x02243c);
	u32 fbpas = fbpat / fbps;
	u32 fbpao = nvkm_rd32(gpu, 0x021c14);
	u32 fbp;
	u32 fbpa = 1 * fbpas;
	u32 size = 0;

	for (fbp = 0; fbp < fbps; fbp++) {
		while (fbpas--) {
			if (!(fbpao & (1 << fbpa)))
				size += nvkm_rd32(gpu, 0x011020c + (fbpa * 0x1000));
			fbpa++;
		}
	}
	printk("VRAM:%d MB.\n", size);
	return size << 20;
}

u64 vram_probe_gp100(struct nvidia_gpu *gpu)
{
	u32 fbps = nvkm_rd32(gpu, 0x022438);
	u32 ltcs = nvkm_rd32(gpu, 0x022450);
	u32 fbpao = nvkm_rd32(gpu, 0x021c14);
	u32 fbp;
	u32 fbpas;

	u32 fbpa = fbp * fbpas;
	u64 size = 0;
	
	for (fbp = 0; fbp < fbps; fbp++) {
		if (!(nvkm_rd32(gpu, 0x021d38) & (1 << fbp))) {
			//u32 ltco = nvkm_rd32(gpu, 0x021d70 + (fbp * 4));
			//u32 ltcm = ~ltco & ((1 << ltcs) - 1);
			fbpas = nvkm_rd32(gpu, 0x022458);
			while (fbpas--) {
				if (!(fbpao & (1 << fbpa)))
					size += nvkm_rd32(gpu, 0x90020c + (fbpa * 0x4000));
				fbpa++;
			}
		}
	}
	printk("VRAM:%d MB.\n", size);
	return size << 20;
}


int nvidia_tlb_flush(struct nvidia_gpu *gpu, u64 pt_addr)
{
	nvkm_wr32(gpu, 0x100cb8, pt_addr >> 8);
	nvkm_wr32(gpu, 0x100cbc, 0x80000000 | 0x1);
}

int nvidia_mmu_map_page(struct nvkm_gpuobj *pgd, u64 virt, u64 phys, u64 priv, u64 type, u64 page_size)
{
	int ret;
	struct nvidia_gpu *gpu = pgd->gpu;
	u64 pd_index = NVC0_PDE(virt);
	u32 pde0;
	u32 pde1;
	struct nvkm_gpuobj *pt;
	u64 pt_index = NVC0_SPTE(virt);
	u32 pte0 = (phys >> 8) | priv | NV_PTE_PRESENT;
	u32 pte1 = type;
	ret = nvkm_gpuobj_rd32(pgd, pd_index * 8 + 0, &pde0);
	ret = nvkm_gpuobj_rd32(pgd, pd_index * 8 + 4, &pde1);
	if (pde1 == 0) {
		pt = nvkm_gpuobj_new(gpu, 0x8000, 1);
		nvkm_gpuobj_wr32(pgd, pd_index * 8 + 4, (pt->vram_paddr >> 8) | NV_SPT_PRESENT);
		nvkm_gpuobj_rd32(pgd, pd_index * 8 + 4, &pde1);
	}

	//printk("pde1 = %x pte = %x\n", pde1, pte0);
	nvkm_memory_wr32(gpu, PDE_2_ADDR(pde1) + pt_index * 8, pte0);
	nvkm_memory_wr32(gpu, PDE_2_ADDR(pde1) + pt_index * 8 + 4, pte1);

	nvidia_tlb_flush(gpu, pde1 + pt_index * 8);
	return 0;
}


struct nv_chipset nv_chipsets[] = {
	{0x118, 0, vram_probe_gm107},
	{0x132, 0, vram_probe_gp100},
	{0x134, 0, vram_probe_gp100}
};

int nvidia_gpu_chip_probe(struct nvidia_gpu *gpu)
{
	int i;
	u32 chipset = (nvkm_rd32(gpu, 0) >> 20) & 0x1ff;
	printk("nvidia chipset = %x\n", chipset);
	for (i = 0; i < sizeof(nv_chipsets) / sizeof(struct nv_chipset); i++) {
		if (nv_chipsets[i].chipset == chipset) {
			gpu->chipset = &nv_chipsets[i];
			break;
		}
	}
}

int nvidia_gpu_memory_init(struct nvidia_gpu *gpu)
{
	int i;
	u64 ram_size = gpu->chipset->vram_probe(gpu);
	gpu->memory.alloc_bitmap = bitmap_alloc(ram_size / 0x1000);

	/*TODO: 0x800000 0x1000000 cause crash. find out why */
	gpu->fb.fb = nvkm_gpuobj_new(gpu, 0x2000000, 0);

	//nvkm_gpuobj_new(gpu, 0x10000000);
	/*
	for (i = 0; i < 0x1000000; i += 4) {
		nvkm_gpuobj_wr32(gpu->fb.fb, i, 0xf00ff0);
	}
	*/
	
	
}

int nvidia_gpu_bar_init(struct nvidia_gpu *gpu)
{
	int i;
	struct pci_dev *pdev = gpu->pdev;
	gpu->bar[1].bar_base = pci_get_bar_base(pdev, 1);
	gpu->bar[1].bar_size = pci_get_bar_size(pdev, 1);
	gpu->bar[1].alloc_bitmap = bitmap_alloc(gpu->bar[1].bar_size / 0x1000);
	gpu->bar[3].bar_base = pci_get_bar_base(pdev, 3);
	gpu->bar[3].bar_size = pci_get_bar_size(pdev, 3);
	gpu->bar[3].alloc_bitmap = bitmap_alloc(gpu->bar[3].bar_size / 0x1000);
	printk("bar1 base:0x%x, size = 0x%x\n", gpu->bar[1].bar_base, gpu->bar[1].bar_size);
	printk("bar3 base:0x%x, size = 0x%x\n", gpu->bar[3].bar_base, gpu->bar[3].bar_size);
	gpu->bar[1].inst_mem = nvkm_gpuobj_new(gpu, 0x1000, 1);
	gpu->bar[1].pgd = nvkm_gpuobj_new(gpu, 0x2000 * 8, 1);
	for (i = 0; i < 0x2000 * 8; i += 4)
		nvkm_gpuobj_wr32(gpu->bar[1].pgd, i, 0);

	nvkm_gpuobj_wr32(gpu->bar[1].inst_mem, 0x200, gpu->bar[1].pgd->vram_paddr);
	nvkm_gpuobj_wr32(gpu->bar[1].inst_mem, 0x204, gpu->bar[1].pgd->vram_paddr >> 32);
	nvkm_gpuobj_wr32(gpu->bar[1].inst_mem, 0x208, gpu->bar[1].bar_size - 1);
	nvkm_gpuobj_wr32(gpu->bar[1].inst_mem, 0x20c, (gpu->bar[1].bar_size - 1) >> 32);

	/* allocate vma on bar1 for framebuffer. */
	gpu->fb.fb->bar_addr = nvkm_vma_new(gpu, 1, gpu->fb.fb->size);
	gpu->fb.fb->mapped_ptr = ioremap(gpu->fb.fb->bar_addr, gpu->fb.fb->size);

	nvkm_map_area(gpu->bar[1].pgd, 
		gpu->fb.fb->bar_addr,
		gpu->fb.fb->vram_paddr,
		gpu->fb.fb->size,
		0,
		NV_VM_TARGET_VRAM
	);

	printk("switch to vmm bar...\n");

	nvkm_mask(gpu, 0x200, 0x00000100, 0x00000000);
	nvkm_mask(gpu, 0x200, 0x00000100, 0x00000100);

	nvkm_wr32(gpu, 0x001704, 0x80000000 | (gpu->bar[1].inst_mem->vram_paddr >> 12));
}

#define PUSH_BUFFER_SIZE 0x12000
#define IB_OFFSET 0x10000
#define IB_SIZE (PUSH_BUFFER_SIZE - IB_OFFSET)

int nv_chan_nr = 0;
int nvidia_fifo_channel_new(struct nvidia_gpu *gpu, struct nvkm_fifo_chan **chan_ptr)
{
	int i;
	struct nvkm_fifo_chan *chan;
	u64 user_mem;

	u64 ib_length = 0;
	u64 ib_vma_addr = 0;

	chan = *chan_ptr = kmalloc(sizeof(struct gp_fifo), GFP_KERNEL);
	chan->gpu = gpu;

	/* instance memory */
	chan->inst = nvkm_gpuobj_new(gpu, 0x1000, 1);
	chan->runl = 0;
	chan->id = nv_chan_nr++;

	//chan->pgd = nvkm_gpuobj_new(gpu, 0x10000);
	nvkm_gpuobj_wr32(chan->inst, 0x0200, gpu->bar[1].pgd->vram_paddr);
	nvkm_gpuobj_wr32(chan->inst, 0x0204, gpu->bar[1].pgd->vram_paddr >> 32);
	nvkm_gpuobj_wr32(chan->inst, 0x0208, 0xffffffff);
	nvkm_gpuobj_wr32(chan->inst, 0x020c, 0x000000ff);

	chan->push.size = PUSH_BUFFER_SIZE;
	chan->push.push_vaddr = kmalloc(chan->push.size, GFP_KERNEL);
	chan->push.push_paddr = VIRT2PHYS(chan->push.push_vaddr);
	memset(chan->push.push_vaddr, 0, PUSH_BUFFER_SIZE);

	/* determine address of this channel's user registers */
	chan->addr = gpu->fifo.user_mem->bar_addr + 0x200 * chan->id;
	chan->user = ioremap(gpu->bar[1].bar_base + chan->addr, 0x200);
	/* allocate vma from bar1 vm space */
	chan->push.push_vma_bar1 = nvkm_vma_new(gpu, 1, chan->push.size);

	/* map push buffer in bar1 vm space */
	nvkm_map_area(gpu->bar[1].pgd,
		chan->push.push_vma_bar1,
		chan->push.push_paddr,
		chan->push.size,
		0,
		NV_VM_TARGET_SYSRAM_SNOOP
	);

	/* Clear channel control registers. */
	for (i = 0; i < 0x200; i += 4) {
		nvkm_gpuobj_wr32(gpu->fifo.user_mem, chan->id * 0x200 + i, 0);
	}

	user_mem = chan->id * 0x200 + gpu->fifo.user_mem->vram_paddr;
	ib_length = order_base_2(IB_SIZE / 8);
	ib_vma_addr = chan->push.push_vma_bar1 + IB_OFFSET;
	/*RAMFC*/
	nvkm_gpuobj_wr32(chan->inst, 0x08, user_mem);
	nvkm_gpuobj_wr32(chan->inst, 0x0c, user_mem >> 32);
	nvkm_gpuobj_wr32(chan->inst, 0x10, 0x0000face);
	nvkm_gpuobj_wr32(chan->inst, 0x30, 0xfffff902);
	nvkm_gpuobj_wr32(chan->inst, 0x48, ib_vma_addr);
	nvkm_gpuobj_wr32(chan->inst, 0x4c, (ib_vma_addr >> 32) | (ib_length << 16));
	nvkm_gpuobj_wr32(chan->inst, 0x84, 0x20400000);
	nvkm_gpuobj_wr32(chan->inst, 0x94, 0x30000001);
	nvkm_gpuobj_wr32(chan->inst, 0x9c, 0x00000100);
	nvkm_gpuobj_wr32(chan->inst, 0xac, 0x0000001f);
	nvkm_gpuobj_wr32(chan->inst, 0xe8, chan->id);
	nvkm_gpuobj_wr32(chan->inst, 0xb8, 0xf8000000);
	nvkm_gpuobj_wr32(chan->inst, 0xf8, 0x10003080);
	nvkm_gpuobj_wr32(chan->inst, 0xfc, 0x10000010);

	return 0;
	
}

int nvidia_fifo_runlist_init(struct nvidia_gpu *gpu)
{
	int i;
	u64 pbdma_nr;
	pbdma_nr = nvkm_rd32(gpu, 0x002004) & 0xff;
	u64 runlist_nr = 1;
	u64 user_virt = 0;

	printk("nvidia gpu:pbdma:%d\n", pbdma_nr);
	/* Enable PBDMAs */
	nvkm_wr32(gpu, 0x00204, 0xffffffff);

	/* PBDMA[n] */
	for (i = 0; i < pbdma_nr; i++) {
		nvkm_mask(gpu, 0x04013c + (i * 0x2000), 0x10000100, 0x00000000);
		nvkm_wr32(gpu, 0x040108 + (i * 0x2000), 0xffffffff);
		nvkm_wr32(gpu, 0x04010c + (i * 0x2000), 0xfffffeff);
	}

	/* PBDMA[n].HCE */
	for (i = 0; i < pbdma_nr; i++) {
		nvkm_wr32(gpu, 0x040148 + (i * 0x2000), 0xffffffff);
		nvkm_wr32(gpu, 0x04014c + (i * 0x2000), 0xffffffff);
	}

	for (i = 0; i < runlist_nr; i++) {
		gpu->fifo.runlist[i].mem[0] = nvkm_gpuobj_new(gpu, 0x8000, 0);
		gpu->fifo.runlist[i].mem[1] = nvkm_gpuobj_new(gpu, 0x8000, 0);
	}

	gpu->fifo.user_mem = nvkm_gpuobj_new(gpu, 0x200 * 16, 1);

	/* Map Chan->User to bar1.*/
	gpu->fifo.user_mem->bar_addr = nvkm_vma_new(gpu, 1, 0x200 * 16);
	nvkm_map_area(gpu->bar[1].pgd,
		gpu->fifo.user_mem->bar_addr,
		gpu->fifo.user_mem->vram_paddr,
		gpu->fifo.user_mem->size,
		0,
		NVKM_MEM_TARGET_INST
	);
	gpu->fifo.user_mem->mapped_ptr = ioremap(gpu->bar[1].bar_base + gpu->fifo.user_mem->bar_addr, gpu->fifo.user_mem->size);
	nvkm_wr32(gpu, 0x002254, 0x10000000 | (gpu->fifo.user_mem->bar_addr >> 12));

	nvkm_wr32(gpu, 0x002100, 0xffffffff);
	nvkm_wr32(gpu, 0x002140, 0x7fffffff);
}

static int nvidia_fifo_runlist_commit(struct nvidia_gpu *gpu, u64 runl)
{
	int i = 0;
	int cnt = 1;
	int target = 0;
	struct nvkm_gpuobj *mem = gpu->fifo.runlist[runl].mem[gpu->fifo.runlist[runl].next];
	gpu->fifo.runlist[runl].next = !gpu->fifo.runlist[runl].next;

	/* for each channel */
	nvkm_gpuobj_wr32(mem, i * 8 + 0x0, 0);
	nvkm_gpuobj_wr32(mem, i * 8 + 0x4, 0);

	nvkm_wr32(gpu, 0x002270, (mem->vram_paddr >> 12) | (target << 28));
	nvkm_wr32(gpu, 0x002274, (runl << 20) | cnt);

	while (nvkm_rd32(gpu, 0x002284 + runl * 8) & 0x00100000);
	return 0;
}

int nvidia_fifo_gpfifo_init(struct nvidia_gpu *gpu, struct nvkm_fifo_chan *chan)
{
	u64 addr = chan->inst->vram_paddr >> 12;
	u32 coff = chan->id * 8;

	nvkm_mask(gpu, 0x800004 + coff, 0x000f0000,  chan->runl << 16);
	nvkm_wr32(gpu, 0x800000 + coff, 0x80000000 | addr);

	/* Enable trigger. */
	nvkm_mask(gpu, 0x800004 + coff, 0x00000400, 0x00000400);
	nvidia_fifo_runlist_commit(gpu, chan->runl);
	nvkm_mask(gpu, 0x800004 + coff, 0x00000400, 0x00000400);

	return 0;
}

int nv_dma_wait(struct nvkm_fifo_chan *chan, int slots, u32 size);

int RING_SPACE(struct nvkm_fifo_chan *chan, int size)
{
	int ret = nv_dma_wait(chan, 1, size);
	if (ret)
		return ret;

	chan->ib_status_host.free -= size;
	return 0;
}

void OUT_RING(struct nvkm_fifo_chan *chan, u32 data)
{
	u32 *ptr = chan->push.push_vaddr;
	ptr[chan->ib_status_host.current++] = data;
}

s64 READ_GET(struct nvkm_fifo_chan *chan, u64 *prev_get, u64 timeout)
{
	u64 val;
	val = chan->user[chan->user_get_low / 4] | ((u64)chan->user[chan->user_get_high / 4] << 32);

	if (val != *prev_get) {
		*prev_get = val;
	}

	//if (timeout == 0) {
	//	return -1;
	//}

	if (val < chan->push.push_vma_bar1 || 
		val > chan->push.push_vma_bar1 + (chan->ib_status_host.max << 2))
		return -1;

	return (val - chan->push.push_vma_bar1) >> 2;
}

void FIRE_RING(struct nvkm_fifo_chan *chan)
{
	u32 *push_buffer_ptr = chan->push.push_vaddr;
	u64 ib_entry, delta, length, offset;

	if (chan->ib_status_host.current == chan->ib_status_host.put)
		return;

	ib_entry = chan->ib_status_host.ib_put * 2 + chan->ib_status_host.ib_base;
	delta = chan->ib_status_host.put << 2;
	length = (chan->ib_status_host.current - chan->ib_status_host.put) << 2;
	offset = chan->push.push_vma_bar1 + delta;

	push_buffer_ptr[ib_entry++] = offset;
	push_buffer_ptr[ib_entry++] = (offset >> 32) | (length << 8);

	chan->ib_status_host.ib_put = (chan->ib_status_host.ib_put + 1) & chan->ib_status_host.ib_max;

	//mb()

	offset = push_buffer_ptr[0];

	chan->user[0x8c / 4] = chan->ib_status_host.ib_put;

	chan->ib_status_host.ib_free--;
	chan->ib_status_host.put = chan->ib_status_host.current;
}

void WIND_RING(struct nvkm_fifo_chan *chan)
{
	chan->ib_status_host.current = chan->ib_status_host.put;
}

int nv_dma_wait(struct nvkm_fifo_chan *chan, int slots, u32 size)
{
	u64 prev_get = 0;
	u64 cnt = 0;
	s64 get = 0;

	while (chan->ib_status_host.ib_free < size + 1) {
		get = chan->user[0x88 / 4];
		if (get != prev_get) {
			prev_get = get;
			cnt = 0;
		}

		if ((++cnt & 0xff) == 0) {
			/* timeout */
		}

		chan->ib_status_host.ib_free = get - chan->ib_status_host.ib_put;
		if (chan->ib_status_host.ib_free <= 0) {
			chan->ib_status_host.ib_free += chan->ib_status_host.ib_max;
		}
	}

	prev_get = 0;
	cnt = 0;
	get = 0;

	while (chan->ib_status_host.free < size) {
		get = READ_GET(chan, &prev_get, 0);
		/* timeout */

		if (get <= chan->ib_status_host.current) {
			chan->ib_status_host.free = chan->ib_status_host.max - chan->ib_status_host.current;

			if (chan->ib_status_host.free >= size)
				break;

			FIRE_RING(chan);

			do {
				get = READ_GET(chan, &prev_get, 0);
			} while (get == 0);
			chan->ib_status_host.current = 0;
			chan->ib_status_host.put = 0;
		}

		chan->ib_status_host.free = get - chan->ib_status_host.current - 1;
	}

	return 0;
}

int nvidia_memory_copy(struct nvkm_fifo_chan *chan, u64 src, u64 dst, u64 len)
{
	RING_SPACE(chan, 2);
	BEGIN_NVC0(chan, 4, 0x0000, 1);
	OUT_RING(chan, 0x0000c0b5);

	RING_SPACE(chan, 10);
	BEGIN_NVC0(chan, 4, 0x0400, 8);
	OUT_RING(chan, src >> 32);
	OUT_RING(chan, src);
	OUT_RING(chan, dst >> 32);
	OUT_RING(chan, dst);

	OUT_RING(chan, 0x1000);
	OUT_RING(chan, 0x1000);
	OUT_RING(chan, 0x1000);
	OUT_RING(chan, len / 0x1000);
	BEGIN_IMC0(chan, 4, 0x0300, 0x0386);
	FIRE_RING(chan);
	return 0;
}

int nvidia_fence_emit32(struct nvkm_fifo_chan *chan)
{
	RING_SPACE(chan, 6);
	BEGIN_NVC0(chan, 0, NV84_SUBCHAN_SEMAPHORE_ADDRESS_HIGH, 5);
	OUT_RING(chan, chan->fence->bar_addr << 32);
	OUT_RING(chan, chan->fence->bar_addr);
	OUT_RING(chan, chan->fence_sequence);
	OUT_RING(chan, NV84_SUBCHAN_SEMAPHORE_TRIGGER_WRITE_LONG);
	OUT_RING(chan, 0x00000000);
	FIRE_RING(chan);
	return 0;
}

int nvidia_fence_sync32(struct nvkm_fifo_chan *chan)
{
	RING_SPACE(chan, 6);
	BEGIN_NVC0(chan, 0, NV84_SUBCHAN_SEMAPHORE_ADDRESS_HIGH, 5);
	OUT_RING(chan, chan->fence->bar_addr << 32);
	OUT_RING(chan, chan->fence->bar_addr);
	OUT_RING(chan, chan->fence_sequence);
	OUT_RING(chan, NV84_SUBCHAN_SEMAPHORE_TRIGGER_ACQUIRE_GEQUAL |
			NVC0_SUBCHAN_SEMAPHORE_TRIGGER_YIELD);
	OUT_RING(chan, 0x00000000);
	FIRE_RING(chan);
	return 0;
}

int nvidia_fence_wait(struct nvkm_fifo_chan *chan)
{
	u32 val;
	do {
		nvkm_gpuobj_rd32(chan->fence, 0, &val);
	} while (val != chan->fence_sequence);
	return 0;
}

int nvidia_ib_init(struct nvidia_gpu *gpu, struct nvkm_fifo_chan **chan_p)
{
	struct nvkm_fifo_chan *chan;
	int i;

	nvidia_fifo_channel_new(gpu, chan_p);
	chan = *chan_p;
	nvidia_fifo_gpfifo_init(gpu, chan);

	chan->user_put = 0x40;
	chan->user_get_low = 0x44;
	chan->user_get_high = 0x60;
	chan->ib_status_host.ib_base = IB_OFFSET / 4;
	chan->ib_status_host.ib_max = (IB_SIZE / 8) - 1;
	chan->ib_status_host.ib_put = 0;
	chan->ib_status_host.ib_free = chan->ib_status_host.ib_max - chan->ib_status_host.ib_put;
	chan->ib_status_host.max = chan->ib_status_host.ib_base;

	chan->ib_status_host.put = 0;
	chan->ib_status_host.current = chan->ib_status_host.put;
	chan->ib_status_host.free = chan->ib_status_host.max - chan->ib_status_host.current;

	RING_SPACE(chan, 128 / 4);
	for (i = 0; i < 128 / 4; i++) {
		OUT_RING(chan, 0);
	}

	chan->fence_sequence = 0x12345678;
	chan->fence = nvkm_gpuobj_new(gpu, 0x1000, 1);
	chan->fence->bar_addr = nvkm_vma_new(gpu, 1, chan->fence->size);
	nvkm_map_area(gpu->bar[1].pgd, chan->fence->bar_addr, chan->fence->vram_paddr, chan->fence->size, 0, 0);
	return 0;
}

void nvidia_fb_copyarea(struct fb_info *info, const struct fb_copyarea *region)
{
	struct nvidia_gpu *gpu = (struct nvidia_gpu *)info->dev;
	struct nvkm_fifo_chan *chan = gpu->memcpy_chan;
	u64 base = gpu->fb.fb->bar_addr;
	u64 src = (region->sy * info->var.xres_virtual + region->sx) * (info->var.bits_per_pixel / 8);
	u64 dst = (region->dy * info->var.xres_virtual + region->dx) * (info->var.bits_per_pixel / 8);
	u64 len = region->height * info->var.xres_virtual * (info->var.bits_per_pixel / 8);
	nvidia_memory_copy(chan, base + src, base + dst, len);
	nvidia_fence_emit32(chan);
	nvidia_fence_wait(chan);
}

void register_nvidia_fb(struct nvidia_gpu *gpu)
{
	int i = 0;
	extern struct fb_info boot_fb;
	extern struct fb_ops bootfb_ops;
	boot_fb.dev = (struct device *)gpu;
	bootfb_ops.fb_copyarea = nvidia_fb_copyarea;
}

int nvidia_gpu_probe(struct pci_dev *pdev, struct pci_device_id *pent)
{
	u16 pci_command_reg;
	u64 mmio_base;
	u32 *mmio_virt;
	u32 boot0, strap;
	struct nvidia_gpu *gpu = kmalloc(sizeof(*gpu), GFP_KERNEL);
	memset(gpu, 0, sizeof(*gpu));
	gpu->pdev = pdev;
	pci_read_config_word(pdev, PCI_COMMAND, &pci_command_reg);
	pci_command_reg |= (PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER);
	pci_write_config_word(pdev, PCI_COMMAND, pci_command_reg);
	gpu->mmio_virt = ioremap(pci_get_bar_base(pdev, 0), pci_get_bar_size(pdev, 0));

	pdev->private_data = gpu;

	nvidia_gpu_chip_probe(gpu);
	nvidia_gpu_memory_init(gpu);
	nvidia_gpu_bar_init(gpu);
	nvidia_fifo_runlist_init(gpu);

	nvkm_wr32(gpu, 0x200, 0xffffffff);

	nvidia_ib_init(gpu, &gpu->memcpy_chan);
	register_nvidia_fb(gpu);
	return 0;
}

void nvidia_gpu_remove(struct pci_dev *pdev)
{
	
}


struct pci_device_id nvidia_ids[64] = {
	{0x134f10de, 0x030200, 0xffffffff, 0x0},
	{0x1b8010de, 0x030200, 0xffffffff, 0x0},
	{0x1b0610de, 0x030200, 0xffffffff, 0x0}
};

struct pci_driver nvidia_gpu_driver = {
	.name = "nvidia_gpu_driver",
	.id_array = nvidia_ids,
	.pci_probe = nvidia_gpu_probe,
	.pci_remove = nvidia_gpu_remove
};

void nvidia_gpu_init()
{
	pci_register_driver(&nvidia_gpu_driver);
}

module_init(nvidia_gpu_init);

