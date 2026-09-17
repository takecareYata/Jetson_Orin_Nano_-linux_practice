#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAX_BLOCK_SIZE 4096

int main(int argc, char **argv)
{
	int ret, src_fd, dst_fd, block_size;
	char *src_name, *dst_name;
	char buf[MAX_BLOCK_SIZE];
	unsigned int copied = 0;

	if(argc != 4) {
		printf("usage: %s {src file} {dst file} {block size in bytes}\n", argv[0]);
		return EXIT_FAILURE;
	}
	printf("running %s %s %s %s\n", argv[0], argv[1], argv[2], argv[3]);

	src_name = argv[1];
	dst_name = argv[2];
	block_size = atoi(argv[3]);

	if(block_size > MAX_BLOCK_SIZE) {
		printf("error: block size if too big (maximum is %d bytes (%d)\n", MAX_BLOCK_SIZE, __LINE__);
		return EXIT_FAILURE;
	}

	/* Implement code */


	printf("%u bytes copied\n", copied);

	return EXIT_SUCCESS;
}

