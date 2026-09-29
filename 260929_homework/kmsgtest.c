#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#define BUFFER_SIZE 1024

int main(int argc, char **argv)
{
    int fd;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read, bytes_written;

    if (argc == 1) 
	{
        fd = open("/dev/kmsg", O_RDONLY | O_NONBLOCK);
        if (fd == -1) {
            fprintf(stderr, "kmsgtest open error: %s (line %d)\n", strerror(errno), __LINE__);
            return EXIT_FAILURE;
        }

        printf("=== 최근 /dev/kmsg 메시지 읽기 ===\n");
        
		
        while ((bytes_read = read(fd, buffer, sizeof(buffer) - 1)) > 0) 
		{
            buffer[bytes_read] = '\0';
        	printf("%s", buffer);
        }

        if (bytes_read == -1 && errno != EAGAIN && errno != EWOULDBLOCK) 
		{
            fprintf(stderr, "kmsgtest read error: %s (line %d)\n", strerror(errno), __LINE__);
            close(fd);
            return EXIT_FAILURE;
        }
    } 
    else 
	{
        fd = open("/dev/kmsg", O_WRONLY);
        if (fd == -1) 
		{
            fprintf(stderr, "kmsgtest open error: %s (line %d)\n", strerror(errno), __LINE__);
            return EXIT_FAILURE;
        }

        snprintf(buffer, sizeof(buffer), "%s\n", argv[1]);

        bytes_written = write(fd, buffer, strlen(buffer));
        if (bytes_written == -1) 
		{
            fprintf(stderr, "kmsgtest write error: %s (line %d)\n", strerror(errno), __LINE__);
            close(fd);
            return EXIT_FAILURE;
        }

        printf("커널 로그 메시지 전송 성공: %s", buffer);
    }

    close(fd);
    return EXIT_SUCCESS;
}