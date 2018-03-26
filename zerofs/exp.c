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

	int ret = lseek(fd, 13337, SEEK_SET);
	printf("lseek: %d\n", ret);

	read(fd, buf, 4);
	printf("buf: %s\n", buf);

	ret = write(fd, buf, 4);
	printf("write: %d\n", ret);

	close(fd);

	return 0;
}
