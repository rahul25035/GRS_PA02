/* MT25035_Part_A_A1_client.c */

#define _GNU_SOURCE
#include <sched.h>
#include <fcntl.h>

#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define NUM_FIELDS 8

struct thread_args {
    int field_size;
    int duration;
};

void *client_thread(void *arg) {
    struct thread_args *ta = arg;

    int s = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in a = {
        .sin_family = AF_INET,
        .sin_port = htons(8080)
    };

    /* server IP inside server namespace */
    inet_pton(AF_INET, "10.0.0.1", &a.sin_addr);

    connect(s, (struct sockaddr *)&a, sizeof(a));

    int msg_size = ta->field_size * NUM_FIELDS;
    char send_buf[msg_size];
    char recv_buf[ta->field_size];

    memset(send_buf, 'A', msg_size);

    long messages = 0;
    time_t start = time(NULL);

    while (time(NULL) - start < ta->duration) {
        send(s, send_buf, msg_size, 0);
        messages++;

        for (int i = 0; i < NUM_FIELDS; i++)
            recv(s, recv_buf, ta->field_size, 0);
    }

    dprintf(1, "THREAD %lu MESSAGES %ld\n",
            (unsigned long)pthread_self(), messages);

    close(s);
    return NULL;
}

int main(int argc, char *argv[]) {
    /* join client network namespace */
    int fd = open("/var/run/netns/ns_client", O_RDONLY);
    setns(fd, CLONE_NEWNET);
    close(fd);

    if (argc != 4) {
        write(
            2,
            "Usage: ./client <field_size> <num_threads> <duration>\n",
            sizeof("Usage: ./client <field_size> <num_threads> <duration>\n") - 1
        );
        return 1;
    }

    int field_size = atoi(argv[1]);
    int num_threads = atoi(argv[2]);
    int duration = atoi(argv[3]);

    pthread_t t[num_threads];
    struct thread_args ta = {
        .field_size = field_size,
        .duration = duration
    };

    for (int i = 0; i < num_threads; i++)
        pthread_create(&t[i], NULL, client_thread, &ta);

    for (int i = 0; i < num_threads; i++)
        pthread_join(t[i], NULL);

    return 0;
}
