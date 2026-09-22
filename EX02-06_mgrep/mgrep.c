#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>

pid_t pid;

int main(int argc, char **argv)
{
	pid_t pid_temp;	
	int i;
	int num_of_child;

	printf("[%d] running %s", pid = getpid(), argv[0]);
	for(i=1; i<argc; i++) {
		printf(" %s", argv[i]);
	}
	printf("\n");

	num_of_child = argc - 2;

	/* Implement code */
	pid_t pid_wait;
	int status;

	for (int x = 0; x < num_of_child; x++)
	{
		pid_temp = fork();

		if(pid_temp == -1) {
			printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
			return EXIT_FAILURE;
		}
		else if(pid_temp == 0) {
			int ret;
			pid = getpid();
			printf("[%d] about to run [grep -rn %s %s]\n", pid, argv[2 + x], argv[1]);
			ret = execlp("grep", "grep", "-rn", argv[2 + x], argv[1] , NULL);
			printf("[%d] ret = %d\n", pid, ret);
			return EXIT_FAILURE;
		}
		else {
            printf("[%d] created child process PID: %d\n", pid, pid_temp);
        }
	}

	for (int x = 0; x < num_of_child; x++) 
	{
		printf("[%d] waiting child's termination\n", pid);
		pid_wait = wait(&status);
		printf("[%d] pid %d has been terminated with status %#x\n", pid, pid_wait, status);
	}

	printf("[%d] terminted\n", pid);

	return EXIT_SUCCESS;
}