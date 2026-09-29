#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ipc.h>

#if 0 /* org */
#include <sys/sem.h>
#include "sem.h"
#else /* POSIX */
#include <sys/stat.h>
#include <semaphore.h>
#define SEM_NAME "/mysem"
#endif

#define FILE_TEST "./test.txt"
#define LOOP_COUNT 50

pid_t pid;

int main(int argc, char **argv)
{
        int ret;
        int i;
        char c1 = 'A';
        char c2;
        int fd;
#if 0 /* org */
        int id_sem;
        struct sembuf sops_dec[1] = {{0, -1, SEM_UNDO}};
        struct sembuf sops_inc[1] = {{0, 1, SEM_UNDO}};
#else /* POSIX */
        sem_t *sem;
#endif

        if(argc != 1) {
                printf("usage: %s\n", argv[0]);
                return EXIT_FAILURE;
        }
        printf("[%d] running %s\n", pid = getpid(), argv[0]);

#if 0 /* org */
        id_sem = semget((key_t)KEY_SEM, 1, 0666|IPC_CREAT);
        if(id_sem == -1) {
                printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
                return EXIT_FAILURE;
        }
#else /* POSIX */
        sem = sem_open(SEM_NAME, O_CREAT, 0666, 1);
        if (sem == SEM_FAILED)
        {
                printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
                return EXIT_FAILURE;
        }
#endif

        for(i=0; i<LOOP_COUNT; i++) {
                printf("[%d] LOOP %d\n", pid, i);

#if 0 /* org */
                ret = semop(id_sem, sops_dec, 1);
                if(ret == -1) {
                        printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
                        return EXIT_FAILURE;
                }
#else /* POSIX */
                ret = sem_wait(sem);
                if (ret == -1)
                {
                        printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
                        sem_close(sem);
                        return EXIT_FAILURE;
                }
#endif

                fd = open(FILE_TEST, O_RDWR|O_CREAT, 0777);
                if(ret == -1) {
                        printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
                        return EXIT_FAILURE;
                }
                ret = write(fd, &c1, sizeof(char));
                if(ret == -1) {
                        printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
                        return EXIT_FAILURE;
                }
                ret = close(fd);
                if(ret == -1) {
                        printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
                        return EXIT_FAILURE;
                }

                usleep(rand()%500000);

                fd = open(FILE_TEST, O_RDWR);
                if(ret == -1) {
                        printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
                        return EXIT_FAILURE;
                }
                ret = read(fd, &c2, sizeof(char));
                if(ret == -1) {
                        printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
                        return EXIT_FAILURE;
                }
                ret = close(fd);
                if(ret == -1) {
                        printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
                        return EXIT_FAILURE;
                }

                if(c1 != c2) {
                        printf("[%d] error: not same (%d)\n", pid,  __LINE__);
                        return EXIT_FAILURE;
                }

#if 0 /* org */
                ret = semop(id_sem, sops_inc, 1);
                if(ret == -1) {
                        printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
                        return EXIT_FAILURE;
                }
#else /* POSIX */
                ret = sem_post(sem);
                if (ret == -1)
                {
                        printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
                        sem_close(sem);
                        return EXIT_FAILURE;
                }
#endif

                c1++;
        }

#if 0 /* org */
#else /* POSIX */
        ret = sem_close(sem);
        if (ret == -1)
        {
                printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
                return EXIT_FAILURE;
        }
#endif

        printf("[%d] terminted\n", pid);

        return EXIT_SUCCESS;
}