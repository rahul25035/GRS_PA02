#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/uio.h>
#include <time.h>

#include "MT25035_Part_A_A1_common.h"

typedef struct {
    int client_fd;
    int msg_size;
} client_arg_t;

static int active_threads = 0;
static int max_threads = 0;

void *client_handler(void *arg) {
    client_arg_t *carg = (client_arg_t *)arg;

    printf("[Server-A2] Client handler started (msg_size=%d)\n",
           carg->msg_size);

    message_t msg;
    msg.field_size = carg->msg_size;

    struct iovec iov[NUM_FIELDS];
    struct msghdr msg_hdr;

    memset(&msg_hdr, 0, sizeof(msg_hdr));

    for (int i = 0; i < NUM_FIELDS; i++) {
        msg.fields[i] = malloc(msg.field_size);
        memset(msg.fields[i], 'A' + i, msg.field_size);

        iov[i].iov_base = msg.fields[i];
        iov[i].iov_len  = msg.field_size;
    }

    msg_hdr.msg_iov = iov;
    msg_hdr.msg_iovlen = NUM_FIELDS;

    long messages_sent = 0;
    time_t last = time(NULL);

    while (1) {
        ssize_t ret = sendmsg(carg->client_fd, &msg_hdr, 0);
        if (ret <= 0)
            break;

        messages_sent++;

        time_t now = time(NULL);
        if (now != last) {
            printf("[Server-A2] Messages sent: %ld\n", messages_sent);
            last = now;
        }
    }

    printf("[Server-A2] Client disconnected\n");

    for (int i = 0; i < NUM_FIELDS; i++)
        free(msg.fields[i]);

    close(carg->client_fd);
    free(carg);

    active_threads--;
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage: %s <port> <message_size> <threads>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);
    int msg_size = atoi(argv[2]);
    max_threads = atoi(argv[3]);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));
    listen(server_fd, max_threads);

    printf("[Server-A2] Listening on port %d (max threads = %d)\n",
           port, max_threads);

    while (1) {
        int client_fd = accept(server_fd, NULL, NULL);

        if (active_threads >= max_threads) {
            printf("[Server-A2] Thread limit reached, rejecting client\n");
            close(client_fd);
            continue;
        }

        client_arg_t *arg = malloc(sizeof(client_arg_t));
        arg->client_fd = client_fd;
        arg->msg_size = msg_size;

        pthread_t tid;
        active_threads++;
        pthread_create(&tid, NULL, client_handler, arg);
        pthread_detach(tid);
    }
}
