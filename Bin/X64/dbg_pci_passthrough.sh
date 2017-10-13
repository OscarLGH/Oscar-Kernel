modprobe vfio
modprobe vfio_pci
vfio-bind 0000:02:00.0 0000:02:00.1 0000:04:00.0 0000:04:00.1 0000:0e:00.0

$HOME/Code/qemu-2.9.0/x86_64-softmmu/qemu-system-x86_64 \
		-monitor stdio \
		-m 8192 \
		-smp 4,cores=2,threads=2 \
		-vga none \
		--enable-kvm \
		-cpu host,kvm=off \
		-device vfio-pci,host=0000:02:00.0 \
		-device vfio-pci,host=0000:02:00.1 \
		-device vfio-pci,host=0000:04:00.0 \
		-device vfio-pci,host=0000:04:00.1 \
		-device vfio-pci,host=0000:0e:00.0 \
		-bios OVMF.fd \
		-hda vdisk.img \
		-gdb tcp::1234 -S
#		-hda /media/ProgramFiles/VMWare/"Windows 7 x64"/"Windows 7 x64.vmdk"
#		-hda ../vdisk/redhat73_20170712.img
