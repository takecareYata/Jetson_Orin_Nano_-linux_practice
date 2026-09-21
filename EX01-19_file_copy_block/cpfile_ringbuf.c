#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>

#define MAX_BLOCK_SIZE 4096
#define RING_BUFFER_SLOTS 8 // 링 버퍼가 담을 수 있는 슬롯(블록) 개수

/* 링 버퍼 각 슬롯 구조체 */
typedef struct {
    char data[MAX_BLOCK_SIZE];
    int size; // 읽어온 실제 바이트 수 (0 이하이면 작업 종료 신호)
} BufferSlot;

/* 링 버퍼 및 동기화 전역 상태 */
typedef struct {
    BufferSlot slots[RING_BUFFER_SLOTS];
    int head; // 생산자가 데이터를 쓸 위치
    int tail; // 소비자가 데이터를 읽을 위치
    int count; // 현재 채워진 슬롯 수
    int done; // 파일 읽기 완료 여부
    int error; // 읽기/쓰기 도중 에러 발생 여부
    
    pthread_mutex_t mutex;
    pthread_cond_t not_full;  // 버퍼가 꽉 차지 않음 (생산자 대기용)
    pthread_cond_t not_empty; // 버퍼가 비어있지 않음 (소비자 대기용)
} RingBuffer;

typedef struct {
    int src_fd;
    int dst_fd;
    int block_size;
    RingBuffer *ring;
    unsigned int bytes_copied;
} ThreadArgs;

/* 생산자 쓰레드: src_fd -> Ring Buffer */
void *producer_thread(void *arg)
{
    ThreadArgs *targs = (ThreadArgs *)arg;
    RingBuffer *ring = targs->ring;

    for (;;) {
        char temp_buf[MAX_BLOCK_SIZE];
        int bytes_read = read(targs->src_fd, temp_buf, targs->block_size);

        if (bytes_read == -1) {
            pthread_mutex_lock(&ring->mutex);
            ring->error = errno;
            ring->done = 1;
            pthread_cond_signal(&ring->not_empty);
            pthread_mutex_unlock(&ring->mutex);
            break;
        }

        pthread_mutex_lock(&ring->mutex);

        // 버퍼가 가득 차면 빈자리가 생길 때까지 대기
        while (ring->count == RING_BUFFER_SLOTS && !ring->error) {
            pthread_cond_wait(&ring->not_full, &ring->mutex);
        }

        if (ring->error) {
            pthread_mutex_unlock(&ring->mutex);
            break;
        }

        // 링 버퍼에 데이터 복사
        memcpy(ring->slots[ring->head].data, temp_buf, (bytes_read > 0) ? bytes_read : 0);
        ring->slots[ring->head].size = bytes_read;
        ring->head = (ring->head + 1) % RING_BUFFER_SLOTS;
        ring->count++;

        if (bytes_read == 0) { // EOF 도달
            ring->done = 1;
            pthread_cond_signal(&ring->not_empty);
            pthread_mutex_unlock(&ring->mutex);
            break;
        }

        // 소비자 알림
        pthread_cond_signal(&ring->not_empty);
        pthread_mutex_unlock(&ring->mutex);
    }

    return NULL;
}

/* 소비자 쓰레드: Ring Buffer -> dst_fd */
void *consumer_thread(void *arg)
{
    ThreadArgs *targs = (ThreadArgs *)arg;
    RingBuffer *ring = targs->ring;

    for (;;) {
        pthread_mutex_lock(&ring->mutex);

        // 버퍼가 비어있고 작업이 끝나지 않았다면 대기
        while (ring->count == 0 && !ring->done && !ring->error) {
            pthread_cond_wait(&ring->not_empty, &ring->mutex);
        }

        // 에러 발생 처리
        if (ring->error) {
            pthread_mutex_unlock(&ring->mutex);
            break;
        }

        // 버퍼가 비어있고 생산자가 작업을 완료했다면 종료
        if (ring->count == 0 && ring->done) {
            pthread_mutex_unlock(&ring->mutex);
            break;
        }

        // 링 버퍼에서 데이터 꺼내기
        BufferSlot slot = ring->slots[ring->tail];
        ring->tail = (ring->tail + 1) % RING_BUFFER_SLOTS;
        ring->count--;

        pthread_cond_signal(&ring->not_full);
        pthread_mutex_unlock(&ring->mutex);

        if (slot.size == 0) break; // EOF 신호 처리

        // 파일에 쓰기 (Partial Write 처리 포함)
        int written_total = 0;
        while (written_total < slot.size) {
            int ret = write(targs->dst_fd, slot.data + written_total, slot.size - written_total);
            if (ret == -1) {
                pthread_mutex_lock(&ring->mutex);
                ring->error = errno;
                pthread_cond_signal(&ring->not_full);
                pthread_mutex_unlock(&ring->mutex);
                return NULL;
            }
            written_total += ret;
        }

        targs->bytes_copied += written_total;
    }

    return NULL;
}

int main(int argc, char **argv)
{
    int src_fd, dst_fd, block_size;
    char *src_name, *dst_name;

    if (argc != 4) {
        printf("usage: %s {src file} {dst file} {block size in bytes}\n", argv[0]);
        return EXIT_FAILURE;
    }
    printf("running %s %s %s %s\n", argv[0], argv[1], argv[2], argv[3]);

    src_name = argv[1];
    dst_name = argv[2];
    block_size = atoi(argv[3]);

    if (block_size > MAX_BLOCK_SIZE || block_size <= 0) {
        printf("error: invalid block size (1 ~ %d bytes) (%d)\n", MAX_BLOCK_SIZE, __LINE__);
        return EXIT_FAILURE;
    }

    /* 원본 파일 열기 */
    src_fd = open(src_name, O_RDONLY);
    if (src_fd == -1) {
        printf("error opening src: %s (%d)\n", strerror(errno), __LINE__);
        return EXIT_FAILURE;
    }

    /* 대상 파일 열기 */
    dst_fd = open(dst_name, O_WRONLY | O_EXCL | O_CREAT, S_IRUSR | S_IWUSR);
    if (dst_fd == -1) {
        printf("error opening dst: %s (%d)\n", strerror(errno), __LINE__);
        close(src_fd);
        return EXIT_FAILURE;
    }

    /* 링 버퍼 초기화 */
    RingBuffer ring;
    memset(&ring, 0, sizeof(RingBuffer));
    pthread_mutex_init(&ring.mutex, NULL);
    pthread_cond_init(&ring.not_full, NULL);
    pthread_cond_init(&ring.not_empty, NULL);

    ThreadArgs targs = {
        .src_fd = src_fd,
        .dst_fd = dst_fd,
        .block_size = block_size,
        .ring = &ring,
        .bytes_copied = 0
    };

    /* 쓰레드 생성 */
    pthread_t prod_tid, cons_tid;
    pthread_create(&prod_tid, NULL, producer_thread, &targs);
    pthread_create(&cons_tid, NULL, consumer_thread, &targs);

    /* 쓰레드 종료 대기 */
    pthread_join(prod_tid, NULL);
    pthread_join(cons_tid, NULL);

    /* 자원 정리 */
    close(src_fd);
    close(dst_fd);
    pthread_mutex_destroy(&ring.mutex);
    pthread_cond_destroy(&ring.not_full);
    pthread_cond_destroy(&ring.not_empty);

    if (ring.error != 0) {
        printf("error during copy: %s (%d)\n", strerror(ring.error), __LINE__);
        return EXIT_FAILURE;
    }

    printf("%u bytes copied\n", targs.bytes_copied);

    return EXIT_SUCCESS;
}