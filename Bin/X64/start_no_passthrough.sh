$HOME/Code/qemu-2.9.0/x86_64-softmmu/qemu-system-x86_64 \
		-monitor stdio \
		-m 8192 \
		-smp 4,cores=2,threads=2 \
		--enable-kvm \
		-cpu host,kvm=off \
		-bios OVMF.fd \
		-hda vdisk.img
