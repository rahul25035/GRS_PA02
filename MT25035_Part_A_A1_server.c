#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <time.h>

#include "MT25035_Part_A_A1_common.h"

typedef struct {
    int client_fd;
    int msg_size;
} client_arg_t;

void *client_handler(void *arg) {
    client_arg_t *carg = (client_arg_t *)arg;

    printf("[Server] Client connected (msg_size=%d bytes)\n",
           carg->msg_size);

    message_t msg;
    msg.field_size = carg->msg_size;

    for (int i = 0; i < NUM_FIELDS; i++) {
        msg.fields[i] = malloc(msg.field_size);
        memset(msg.fields[i], 'A' + i, msg.field_size);
    }

    long messages_sent = 0;
    time_t last = time(NULL);

    while (1) {
        for (int i = 0; i < NUM_FIELDS; i++) {
            if (send(carg->client_fd,
                     msg.fields[i],
                     msg.field_size,
                     0) <= 0) {
                goto cleanup;
            }
        }

        messages_sent++;

        time_t now = time(NULL);
        if (now != last) {
            printf("[Server] Messages sent: %ld\n", messages_sent);
            last = now;
        }
    }

cleanup:
    printf("[Server] Client disconnected\n");

    for (int i = 0; i < NUM_FIELDS; i++)
        free(msg.fields[i]);

    close(carg->client_fd);
    free(carg);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage: %s <port> <message_size> <threads>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);
    int msg_size = atoi(argv[2]);
    int max_threads = atoi(argv[3]);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));
    listen(server_fd, max_threads);

    printf("[Server] Listening on port %d\n", port);

    while (1) {
        int client_fd = accept(server_fd, NULL, NULL);

        client_arg_t *arg = malloc(sizeof(client_arg_t));
        arg->client_fd = client_fd;
        arg->msg_size = msg_size;

        pthread_t tid;
        pthread_create(&tid, NULL, client_handler, arg);
        pthread_detach(tid);
    }
}
