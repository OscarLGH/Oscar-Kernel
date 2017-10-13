file_name="vdisk.img"
file_sectors=163840
image_format="vfat"

if [ ! -f $file_name ]; then
 dd if=/dev/zero of=$file_name bs=512 count=$file_sectors
 sleep 1
 sudo fdisk $file_name
fi

free_loop=`losetup -f`
losetup $free_loop $file_name
sudo kpartx -av $free_loop
mount_dev=${free_loop##*/}
mount_path="/dev/mapper/"${mount_dev}"p1"
sleep 1
sudo mkfs.$image_format $mount_path
sudo mount $mount_path /mnt
sudo cp EFI /mnt -r
sudo cp Kernel.bin /mnt
sync
sudo umount /mnt
losetup -d $free_loop


