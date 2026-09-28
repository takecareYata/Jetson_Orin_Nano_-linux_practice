#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <semaphore.h>

#define FILE_TEST "./test.txt"
#define SEM_NAME "/my_posix_sem"
#define LOOP_COUNT 50

pid_t pid;

int main(int argc, char **argv)
{
    int ret;
    int i;
    char c1 = 'A';
    char c2;
    int fd;
    sem_t *sem;

    if(argc != 1) {
        printf("usage: %s\n", argv[0]);
        return EXIT_FAILURE;
    }
    printf("[%d] running %s\n", pid = getpid(), argv[0]);

    sem = sem_open(SEM_NAME, O_CREAT, 0666, 1);
    if(sem == SEM_FAILED) {
        printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
        return EXIT_FAILURE;
    }

    for(i=0; i<LOOP_COUNT; i++) {
        printf("[%d] LOOP %d\n", pid, i);

        ret = sem_wait(sem);
        if(ret == -1) {
            printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
            sem_close(sem);
            return EXIT_FAILURE;
        }

        fd = open(FILE_TEST, O_RDWR|O_CREAT, 0777);
        if(fd == -1) {
            printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
            sem_post(sem);
            sem_close(sem);
            return EXIT_FAILURE;
        }

        ret = write(fd, &c1, sizeof(char));
        if(ret == -1) {
            printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
            close(fd);
            sem_post(sem);
            sem_close(sem);
            return EXIT_FAILURE;
        }
        close(fd);

        usleep(rand() % 500000);

        fd = open(FILE_TEST, O_RDWR);
        if(fd == -1) {
            printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
            sem_post(sem);
            sem_close(sem);
            return EXIT_FAILURE;
        }

        ret = read(fd, &c2, sizeof(char));
        if(ret == -1) {
            printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
            close(fd);
            sem_post(sem);
            sem_close(sem);
            return EXIT_FAILURE;
        }
        close(fd);

        if(c1 != c2) {
            printf("[%d] error: not same (%d)\n", pid, __LINE__);
            sem_post(sem);
            sem_close(sem);
            return EXIT_FAILURE;
        }

        ret = sem_post(sem);
        if(ret == -1) {
            printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
            sem_close(sem);
            return EXIT_FAILURE;
        }

        c1++;
    }

    sem_close(sem);
    sem_unlink(SEM_NAME);

    printf("[%d] terminated\n", pid);

    return EXIT_SUCCESS;
}