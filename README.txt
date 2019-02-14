Oscar-Kernel:
This is a bare metal kernel that can run on x86_64 physical machine with UEFI support.

Support features:

	branch "refactor":
		running on x86 long-mode
		SMP support
		Process support with multi-core scheduling.
		PCI driver with MSI interrupt
		ACPI support
		Simple virtual machine on Intel CPUs (VMX)

	branch "master"
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
		1.install qemu and ovmf bios:
			apt install qemu ovmf
		2.run kernel:
			sudo ./run_qemu.sh
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
		5.after choosing resolution,the kernel starts to run.

	Run on PXE:
		1.set boot directory to arch/x86_64/bin.
		2.set bootup file as arch/x86_64/bin/EFI/Boot/bootx64.efi.
		3.connect target computer to a same local network.
		4.boot target computer/virtual machine.

	
