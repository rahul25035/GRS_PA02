#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int MSG_SIZE;
int MODE_FLAGS;

void *client_thread(void *arg) {
    long tid = (long)arg;
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    char *buffer = malloc(MSG_SIZE);

    printf("[CLIENT][Thread %ld] Started\n", tid);

    memset(buffer, 'A', MSG_SIZE - 1);
    buffer[MSG_SIZE - 1] = '\0';

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(8080),
        .sin_addr.s_addr = inet_addr("127.0.0.1")
    };

    connect(sock, (struct sockaddr *)&addr, sizeof(addr));
    printf("[CLIENT][Thread %ld] Connected to server\n", tid);

    struct iovec iov = {
        .iov_base = buffer,
        .iov_len  = MSG_SIZE
    };

    struct msghdr msg = {0};
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    if (MODE_FLAGS) {
        sendmsg(sock, &msg, MODE_FLAGS);
        printf("[CLIENT][Thread %ld] Sent message (zero-copy path)\n", tid);
    } else {
        send(sock, buffer, MSG_SIZE, 0);
        printf("[CLIENT][Thread %ld] Sent message (copy path)\n", tid);
    }

    recv(sock, buffer, MSG_SIZE, 0);
    printf("[CLIENT][Thread %ld] Received reply\n", tid);

    close(sock);
    free(buffer);

    printf("[CLIENT][Thread %ld] Finished\n", tid);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage: %s -t|-o|-z <msg_size> <threads>\n", argv[0]);
        return 1;
    }

    MSG_SIZE = atoi(argv[2]);
    int threads = atoi(argv[3]);

    MODE_FLAGS = 0;
    if (strcmp(argv[1], "-z") == 0)
        MODE_FLAGS = MSG_ZEROCOPY;

    printf("[CLIENT] Mode: %s | Message Size: %d | Threads: %d\n",
           argv[1], MSG_SIZE, threads);

    pthread_t tid[threads];

    for (long i = 0; i < threads; i++)
        pthread_create(&tid[i], NULL, client_thread, (void *)i);

    for (int i = 0; i < threads; i++)
        pthread_join(tid[i], NULL);

    printf("[CLIENT] All threads completed\n");
    return 0;
}
