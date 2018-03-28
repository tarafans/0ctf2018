# 0ctf2018

## Config
kaslr + smep

## Organization

public/: give to players

release/: deploy

zerofs/: source

## Disk layout
Check zerofs/mkfs.c

## Mounting
Normal user does not have privilege to mount disk. 
So I create 2 SUID binary mount(zerofs/mount.c) and 
umount(zerofs/umount.c), given to players to mount 
their created zerofs image.

## Bug
In llseek, I only check whether the offset is smaller than file_size or not.
However, the image can be crafted by the attacker. After reversing the disk layout
of the image, the attacker can mount an image which contains a normal file having
file_size 0x7fffffffffffffff (which I specify in mkfs.c).

With llseek, kernel memory read and write can be achieved. But the implemented llseek 
only supports positive seeking, which means that the attacker cannot access the data
before the buffer of the file. This creates certain difficulties.

I believe this file system has more bugs...very buggy...but this way is most intuitive...

## Flag

sha256sum /root/flag

flag{600291f9a05a1e78215aa48c9ff6a4b1bb207c2b4ffa66223fcc67c04281397f}
