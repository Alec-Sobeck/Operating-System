
sudo losetup /dev/loop0 floppy.img
sudo mount /dev/loop0 /mnt
sudo cp kernel_ken/src/kernel /mnt/kernel
sudo umount /dev/loop0
sudo losetup -d /dev/loop0



