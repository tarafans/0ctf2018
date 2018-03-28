obj-m := zerofs.o
ccflags-y := -DDEBUG

all: ko util exp

ko:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

util:
	$(CC) -static -o mount mount.c
	$(CC) -static -o umount umount.c

exp:
	$(CC) -static -o mkfs mkfs.c 
	$(CC) -static -o exp exp.c -lpthread

mount:
	sudo mount -o loop,owner,group,users -t zerofs test.img mnt
	# we do not have perm in zerofs
	sudo chown 1000.1000 mnt -R 
	sudo chmod a+rwx mnt -R

umount:
	sudo umount mnt

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

clean-util:
	rm -f mount umount

clean-exp:
	rm -f mkfs exp


