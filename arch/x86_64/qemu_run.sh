#!/bin/sh
file_name="vdisk.img"
file_sectors=16
image_format="vfat"

rm vdisk.img
if [ ! -f $file_name ]; then
	dd if=/dev/zero of=$file_name bs=1M count=$file_sectors
	sleep 1
	sudo fdisk $file_name << FCMD
	n
	p
	
	
	
	w
FCMD
fi

free_loop=`losetup -f`
losetup $free_loop $file_name
kpartx -av $free_loop
mount_dev=${free_loop##*/}
mount_path="/dev/mapper/"${mount_dev}"p1"
sleep 1
mkfs.$image_format $mount_path
mkdir tmp
mount $mount_path tmp
cp bin/* tmp -r
sync
umount tmp
rm -rf tmp
sudo losetup -d $free_loop

qemu-system-x86_64 \
	-cpu Broadwell \
	-m 1G \
	-bios OVMF.fd \
	-hda vdisk.img
