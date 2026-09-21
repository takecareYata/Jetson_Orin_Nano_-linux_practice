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
	int block_size;
	size_t uret1, uret2, copied = 0;
	FILE *src_fp, *dst_fp;
	char *src_name, *dst_name;
	char buf[MAX_BLOCK_SIZE];

	struct stat st; // stat 함수 사용을 위한 구조체

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

	/*stat 함수로 파일 정보 구조체 st에 넣기*/
	if(stat(src_name, &st) == -1) {
        printf("error: %s (%d)\n", strerror(errno), __LINE__);
        return EXIT_FAILURE;
    }

	/* open source file */
	src_fp = fopen(src_name, "r");
	if(src_fp == NULL) {
		printf("error: %s (%d)\n", strerror(errno), __LINE__);
		return EXIT_FAILURE;
	}

	/* open destination file */
	dst_fp = fopen(dst_name, "w");
	if(dst_fp == NULL) {
		printf("error: %s (%d)\n", strerror(errno), __LINE__);
		return EXIT_FAILURE;
	}

	/* 전체 파일 사이즈/block_size */
	size_t full_blocks = (size_t)st.st_size / (size_t)block_size;
	/* 짜투리 */
    size_t remainder = (size_t)st.st_size % (size_t)block_size;

	/* copy */
	for(size_t i = 0; i < full_blocks; i++) {

        uret1 = fread(buf, (size_t)block_size, 1, src_fp);
        if(uret1 < 1) {
            printf("error: fread failed (%d)\n", __LINE__);
            return EXIT_FAILURE;
        }

        uret2 = fwrite(buf, (size_t)block_size, 1, dst_fp);
        if(uret2 < 1) {
            printf("error: fwrite failed (%d)\n", __LINE__);
            return EXIT_FAILURE;
        }

        copied += (size_t)block_size;
    }

	/* 짜투리 연산 */
	if(remainder > 0) {
        uret1 = fread(buf, 1, remainder, src_fp);
        if(uret1 < remainder) {
            printf("error: fread remainder failed (%d)\n", __LINE__);
            return EXIT_FAILURE;
        }

        uret2 = fwrite(buf, 1, uret1, dst_fp);
        if(uret2 < uret1) {
            printf("error: fwrite remainder failed (%d)\n", __LINE__);
            return EXIT_FAILURE;
        }

        copied += uret2;
    }

	/* close */
	fclose(src_fp);
	fclose(dst_fp);

	printf("%lu bytes copied\n", copied);

	return EXIT_SUCCESS;
}

