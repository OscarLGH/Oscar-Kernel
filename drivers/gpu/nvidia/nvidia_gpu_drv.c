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
		pt = nvkm_gpuobj_new(gpu, 0x8000);
		nvkm_gpuobj_wr32(pgd, pd_index * 8 + 4, (pt->inst_mem_addr >> 8) | NV_SPT_PRESENT);
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

	gpu->fb.fb = nvkm_gpuobj_new(gpu, 0x1000000);

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
	gpu->bar[0].bar_base = pci_get_bar_base(pdev, 1);
	gpu->bar[0].bar_size = pci_get_bar_size(pdev, 1);
	gpu->bar[1].bar_base = pci_get_bar_base(pdev, 3);
	gpu->bar[1].bar_size = pci_get_bar_size(pdev, 3);
	printk("bar1 base:0x%x, size = 0x%x\n", gpu->bar[0].bar_base, gpu->bar[0].bar_size);
	printk("bar3 base:0x%x, size = 0x%x\n", gpu->bar[1].bar_base, gpu->bar[1].bar_size);
	gpu->bar[0].inst_mem = nvkm_gpuobj_new(gpu, 0x1000);
	gpu->bar[0].pgd = nvkm_gpuobj_new(gpu, 0x2000 * 8);
	for (i = 0; i < 0x2000 * 8; i += 4)
		nvkm_gpuobj_wr32(gpu->bar[0].pgd, i, 0);

	nvkm_gpuobj_wr32(gpu->bar[0].inst_mem, 0x200, gpu->bar[0].pgd->inst_mem_addr);
	nvkm_gpuobj_wr32(gpu->bar[0].inst_mem, 0x204, gpu->bar[0].pgd->inst_mem_addr >> 32);
	nvkm_gpuobj_wr32(gpu->bar[0].inst_mem, 0x208, gpu->bar[0].bar_size - 1);
	nvkm_gpuobj_wr32(gpu->bar[0].inst_mem, 0x20c, (gpu->bar[0].bar_size - 1) >> 32);

	for (i = 0; i < 0x4000000; i += 0x1000)
		nvidia_mmu_map_page(gpu->bar[0].pgd, i, i, 0, 0, 0x1000);

	printk("switch to vmm bar...\n");

	nvkm_wr32(gpu, 0x001704, 0x80000000 | (gpu->bar[0].inst_mem->inst_mem_addr >> 12));
}

int nvidia_fifo_channel_new(struct nvidia_gpu *gpu, struct fifo_chan **chan)
{
	int i;
	struct fifo_chan *chan_ptr;

	u64 user_mem = 0;
	u64 ib_vma_addr = 0;
	u64 ib_length = 0;

	chan_ptr = *chan = kmalloc(sizeof(struct gp_fifo), GFP_KERNEL);
	chan_ptr->inst = nvkm_gpuobj_new(gpu, 0x1000);
	chan_ptr->addr = PHYS2VIRT(gpu->bar[0].bar_base + 0);

	chan_ptr->pgd = nvkm_gpuobj_new(gpu, 0x10000);
	nvkm_gpuobj_wr32(chan_ptr->inst, 0x0200, chan_ptr->pgd->inst_mem_addr);
	nvkm_gpuobj_wr32(chan_ptr->inst, 0x0204, chan_ptr->pgd->inst_mem_addr >> 32);
	nvkm_gpuobj_wr32(chan_ptr->inst, 0x0208, 0xffffffff);
	nvkm_gpuobj_wr32(chan_ptr->inst, 0x020c, 0x000000ff);

	for (i = 0; i < 0x4000000; i += 0x1000)
		nvidia_mmu_map_page(chan_ptr->pgd, i, i, 0, 0, 0x1000);

	/*RAMFC*/
	nvkm_gpuobj_wr32(chan_ptr->inst, 0x08, user_mem);
	nvkm_gpuobj_wr32(chan_ptr->inst, 0x0c, user_mem >> 32);
	nvkm_gpuobj_wr32(chan_ptr->inst, 0x10, 0x0000face);
	nvkm_gpuobj_wr32(chan_ptr->inst, 0x30, 0xfffff902);
	nvkm_gpuobj_wr32(chan_ptr->inst, 0x48, ib_vma_addr);
	nvkm_gpuobj_wr32(chan_ptr->inst, 0x4c, (ib_vma_addr >> 32) | (ib_length << 16));
	nvkm_gpuobj_wr32(chan_ptr->inst, 0x84, 0x20400000);
	nvkm_gpuobj_wr32(chan_ptr->inst, 0x94, 0x30000001);
	nvkm_gpuobj_wr32(chan_ptr->inst, 0x9c, 0x00000100);
	nvkm_gpuobj_wr32(chan_ptr->inst, 0xac, 0x0000001f);
	nvkm_gpuobj_wr32(chan_ptr->inst, 0xe8, chan_ptr->id);
	nvkm_gpuobj_wr32(chan_ptr->inst, 0xb8, 0xf8000000);
	nvkm_gpuobj_wr32(chan_ptr->inst, 0xf8, 0x10003080);
	nvkm_gpuobj_wr32(chan_ptr->inst, 0xfc, 0x10000010);

	return 0;
	
}

int nvidia_fifo_runlist_init(struct nvidia_gpu *gpu)
{
	int i;
	u64 pbdma_cnt;
	pbdma_cnt = nvkm_rd32(gpu, 0x002004) & 0xff;
	u64 runlist_cnt = 1;
	u64 user_virt = 0;

	/* Enable PBDMAs */
	nvkm_wr32(gpu, 0x00204, (1 << pbdma_cnt) - 1);

	/* PBDMA[n] */
	for (i = 0; i < pbdma_cnt; i++) {
		//nvkm_mask(gpu, 0x04013c + (i * 0x2000), 0x10000100, 0x00000000);
		nvkm_wr32(gpu, 0x040108 + (i * 0x2000), 0xffffffff);
		nvkm_wr32(gpu, 0x04010c + (i * 0x2000), 0xffffffff);
	}

	/* PBDMA[n].HCE */
	for (i = 0; i < pbdma_cnt; i++) {
		nvkm_wr32(gpu, 0x040148 + (i * 0x2000), 0xffffffff);
		nvkm_wr32(gpu, 0x04014c + (i * 0x2000), 0xffffffff);
	}

	for (i = 0; i < runlist_cnt; i++) {
		gpu->fifo.runlist[i].mem[0] = nvkm_gpuobj_new(gpu, 0x8000);
		gpu->fifo.runlist[i].mem[1] = nvkm_gpuobj_new(gpu, 0x8000);
	}

	gpu->fifo.user_mem = nvkm_gpuobj_new(gpu, 0x200 * 4);

	nvidia_mmu_map_page(gpu->bar[0].pgd, user_virt, gpu->fifo.user_mem->inst_mem_addr, 0, 0, 0x1000);

	nvkm_wr32(gpu, 0x002254, 0x1000000 | (user_virt >> 12));
	nvkm_wr32(gpu, 0x002100, 0xffffffff);
	nvkm_wr32(gpu, 0x002140, 0xffffffff);
}

int nvidia_fifo_runlist_commit(struct nvidia_gpu *gpu, u64 runl)
{
	int i = 0;
	int cnt = 1;
	int target = 3;
	struct nvkm_gpuobj *mem = gpu->fifo.runlist[runl].mem[gpu->fifo.runlist[runl].next];
	gpu->fifo.runlist[runl].next ^= 1;

	nvkm_gpuobj_wr32(mem, i * 8 + 0x0, 0);
	nvkm_gpuobj_wr32(mem, i * 8 + 0x4, 0);

	nvkm_wr32(gpu, 0x002270, (mem->inst_mem_addr >> 12) | (target << 28));
	nvkm_wr32(gpu, 0x002274, (runl << 28) | cnt);
}

int nvidia_gpu_probe(struct pci_dev *pdev, struct pci_device_id *pent)
{
	u16 pci_command_reg;
	u64 mmio_base;
	u32 *mmio_virt;
	u32 boot0, strap;
	struct nvidia_gpu *gpu = kmalloc(sizeof(*gpu), GFP_KERNEL);
	gpu->pdev = pdev;
	pci_read_config_word(pdev, PCI_COMMAND, &pci_command_reg);
	pci_command_reg |= (PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER);
	pci_write_config_word(pdev, PCI_COMMAND, pci_command_reg);
	gpu->mmio_virt = ioremap(pci_get_bar_base(pdev, 0), pci_get_bar_size(pdev, 0));

	pdev->private_data = gpu;

	nvidia_gpu_chip_probe(gpu);
	nvidia_gpu_memory_init(gpu);
	nvidia_gpu_bar_init(gpu);
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

