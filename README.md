# 0ctf2018

## Config
kaslr + smep

## Organization
public/: give to players
release/: deploy
zerofs/: source

## Bug
In llseek, I only check whether the offset is smaller than file_size or not.
However, the image can be crafted by the attacker. After reversing the disk layout
of the image, the attacker can craft an image, which contains a normal file, but 
has a file_size 0x7fffffffffffffff (which I specify in mkfs.c).

With llseek, kernel memory read and write can be achieved. But the implemented llseek 
only supports positive seeking, which means that the attacker cannot access the data
before the buffer of the file. This creates certain difficulties.

## Flag
sha256sum flag
flag{600291f9a05a1e78215aa48c9ff6a4b1bb207c2b4ffa66223fcc67c04281397f}
