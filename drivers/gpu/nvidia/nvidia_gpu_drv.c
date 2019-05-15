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

int nvidia_mmu_map_page(struct nv_inst_obj *pgd, u64 virt, u64 phys, u64 priv, u64 type, u64 page_size)
{
	int ret;
	struct nvidia_gpu *gpu = pgd->gpu;
	u64 pd_index = NVC0_PDE(virt);
	u32 pde0;
	u32 pde1;
	struct nv_inst_obj *pt;
	u64 pt_index = NVC0_SPTE(virt);
	u32 pte0 = (phys >> 8) | priv | NV_PTE_PRESENT;
	u32 pte1 = type;
	ret = nv_instobj_rd32(pgd, pd_index * 8 + 0, &pde0);
	ret = nv_instobj_rd32(pgd, pd_index * 8 + 4, &pde1);
	if (pde1 == 0) {
		pt = nv_instobj_alloc(gpu, 0x8000);
		nv_instobj_wr32(pgd, pd_index * 8 + 4, (pt->inst_mem_addr >> 8) | NV_SPT_PRESENT);
		nv_instobj_rd32(pgd, pd_index * 8 + 4, &pde1);
	}

	//printk("pde1 = %x pte = %x\n", pde1, pte0);
	instmem_wr32(gpu, PDE_2_ADDR(pde1) + pt_index * 8, pte0);
	instmem_wr32(gpu, PDE_2_ADDR(pde1) + pt_index * 8 + 4, pte1);

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

	gpu->fb.fb = nv_instobj_alloc(gpu, 0x1000000);

	/*
	for (i = 0; i < 0x1000000; i += 4) {
		nv_instobj_wr32(gpu->fb.fb, i, 0xf00ff0);
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
	gpu->bar[0].inst_mem = nv_instobj_alloc(gpu, 0x1000);
	gpu->bar[0].pgd = nv_instobj_alloc(gpu, 0x2000 * 8);
	for (i = 0; i < 0x2000 * 8; i += 4)
		nv_instobj_wr32(gpu->bar[0].pgd, i, 0);

	nv_instobj_wr32(gpu->bar[0].inst_mem, 0x200, gpu->bar[0].pgd->inst_mem_addr);
	nv_instobj_wr32(gpu->bar[0].inst_mem, 0x204, gpu->bar[0].pgd->inst_mem_addr >> 32);
	nv_instobj_wr32(gpu->bar[0].inst_mem, 0x208, gpu->bar[0].bar_size - 1);
	nv_instobj_wr32(gpu->bar[0].inst_mem, 0x20c, (gpu->bar[0].bar_size - 1) >> 32);

	for (i = 0; i < 0x4000000; i += 0x1000)
		nvidia_mmu_map_page(gpu->bar[0].pgd, i, i, 0, 0, 0x1000);

	printk("switch to vmm bar...\n");

	nvkm_wr32(gpu, 0x001704, 0x80000000 | (gpu->bar[0].inst_mem->inst_mem_addr >> 12));
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

