obj-m := zerofs.o
ccflags-y := -DDEBUG

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

mkfs:
	$(CC) -o mkfs mkfs.c

create:
	dd bs=4096 count=100 if=/dev/zero of=test.img
	./mkfs test.img

mount:
	sudo mount -o loop,owner,group,users -t zerofs test.img mnt
	# we do not have perm in zerofs
	sudo chown wen.wen mnt -R 
	sudo chmod a+rwx mnt -R

umount:
	sudo umount mnt

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean


