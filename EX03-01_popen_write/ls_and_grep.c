#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

pid_t pid;

#define BUF_SIZE 32


/* ls /etc | grep init 실행을 표현 */
int main(int argc, char **argv)
{
	int ret;
	int ret2, len;
	size_t ulen;
	FILE *fp_r;
	FILE *fp_w;
	char buf[BUF_SIZE];

	pid_t pid = getpid(); // 현재 프로세스 PID 할당

	fp_r = popen("ls /etc", "r");
	if(fp_r == NULL) {
		printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
		return EXIT_FAILURE;
	}

	fp_w = popen("grep init", "w");
	if(fp_w == NULL) {
		printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
		return EXIT_FAILURE;
	}

	while ((ulen = fread(buf, 1, sizeof(buf), fp_r)) > 0) {
        fwrite(buf, 1, ulen, fp_w);
    }

	ret = pclose(fp_r);
	ret2 = pclose(fp_w);


	printf("[%d] ret_r = 0x%x, ret_w = 0x%x\n", pid, ret, ret2);
    printf("[%d] terminated\n", pid);

	return EXIT_SUCCESS;
}
