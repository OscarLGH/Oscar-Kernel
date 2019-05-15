#include "nvidia_gpu.h"

u64 vram_probe_gm107(struct nvidia_gpu *gpu)
{
	u32 fbpt = nvkm_rd32(gpu, 0x022438);
	u32 fbpat = nvkm_rd32(gpu, 0x02243c);
	u32 fbpas = fbpat / fbpt;
	u32 fbpao = nvkm_rd32(gpu, 0x021c14);
	u32 fbpa = 1 * fbpas;
	u32 size = 0;

	while (fbpas--) {
		if (!(fbpao & (1 << fbpa)))
			size += nvkm_rd32(gpu, 0x011020c + (fbpa * 0x1000));
			printk("size = %x\n", size);
		fbpa++;
	}
	printk("VRAM:%d MB.\n", size >> 20);
}

struct nv_chipset nv_chipsets[] = {
	{0x118, 0, vram_probe_gm107},
	{0x132, 0, 0},
	{0x134, 0, 0}
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
	gpu->memory.allocate = bitmap_alloc(ram_size / 0x1000);
}

int nvidia_gpu_bar_init(struct nvidia_gpu *gpu)
{
	u64 bar1_base, bar1_size, bar3_base, bar3_size;
	struct pci_dev *pdev = gpu->pdev;
	bar1_base = pci_get_bar_base(pdev, 1);
	bar1_size = pci_get_bar_size(pdev, 1);
	bar3_base = pci_get_bar_base(pdev, 3);
	bar3_size = pci_get_bar_size(pdev, 3);
	printk("bar1 base:0x%x, size = 0x%x\n", bar1_base, bar1_size);
	printk("bar3 base:0x%x, size = 0x%x\n", bar3_base, bar3_size);
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
	{0x134f10de, 0x030200, 0xffffffff, 0x0}
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

