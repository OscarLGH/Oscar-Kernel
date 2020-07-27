Oscar-Kernel:
This is a bare metal kernel that can run on x86_64 physical/virtual machine with UEFI support.

Supported features:

	branch "refactor":
		running on x86_64 long-mode with SMP enabled.
		Process support with multi-core scheduling.
		ACPI.
		Simple virtual machine on Intel CPUs:
			Nested virtualization.
			Unrestricted guest.
			EPT.
			Posted interrupt.
		PCIe drivers:
			MSI/MSI-X interrupt.
			simple NVIDIA GK104/GP104/GP102 driver
				on board IOMMU support.
				hardware accelerated framebuffer copy.
			XHCI:
				basic support for USB host controller.
			IOMMU:
				intel vt-d:
					DMA remapping.
					Interrupt remapping (WIP).
		USB drivers:
			USB Keyboard (WIP)
			USB mouse (WIP)
			USB removable disk (WIP)
		Memory manager:
			bootmem
			slab (TBD)
			buddy system (TBD)
		VFS (TBD)

	branch "master" (obsolete and no longer maintained)
		running on x86 long-mode.
		SMP support.
		Process support with multi-core scheduling.
		Simple virtual machine on Intel VMX.
		PCI driver
			simple NVIDIA GK104/GP104/GP102 driver 
				on board IOMMU support.
				hardware accelerated framebuffer copy.
			SATA controller driver support
			IDE controller driver support

Guide:
	make kernel:
		1. cd arch/x86_64
		2. make -j4
	Run on qemu:
		1.install dependences:
			apt install qemu-system-x86 ovmf kpartx
		2.run kernel:
			vt-x enabled: sudo ./qemu_run_kvm.sh
			otherwise: sudo ./qemu_run_tcg.sh
		3.after choosing resolution,the kernel starts to run.
	Run on a real machine:
		1.find a removable disk, and format it with FAT32.
		2.copy all files and directories in arch/x86_64/bin to removable disk.The directory tree in removable disk is as follows:
			-EFI
			    -Boot
			        bootx64.efi
			kernel.bin
		3.disable secure boot in BIOS configuration menu.
		4.boot removable disk from BIOS
		5.type to choose a resolution, after that, the kernel starts to run.

	Run on PXE:
		1.set boot directory as arch/x86_64/bin.
		2.set bootup file as arch/x86_64/bin/EFI/Boot/bootx64.efi.
		3.connect target computer to a same local network.
		4.boot target computer/virtual machine.

NOTE:
	Running in nested virtualization environment is not recommanded, e.g, VMWARE L0 + kvm L1 + L2 (Oscar-Kernel), due to some bugs for nested vt-x in vmware/kvm.
