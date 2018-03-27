#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

char buf[4096];

int main(int argc, char *argv[]) {

	char *fn = argv[1];
	int fd;

	if (argc != 2)
		return -1;

	fd = open(fn, O_RDWR);
	printf("fd: %d\n", fd);

	read(fd, buf, 4);
	printf("buf: %s\n", buf);

	// int ret = lseek(fd, 0x1000, SEEK_SET);
	// printf("lseek: %d\n", ret);

	unsigned int i, j;
	void *p;
	for (j = 0; j < 0x10000 / 8; j++) {
		lseek(fd, 0x1000 + j * 8, SEEK_SET);
		read(fd, &p, 8);
		printf("%p\n", p);
	}

	close(fd);

	return 0;
}
