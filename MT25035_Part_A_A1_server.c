/* MT25035_Part_A_A1_server.c */

#define _GNU_SOURCE
#include <sched.h>
#include <fcntl.h>

#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

#define NUM_FIELDS 8

struct message {
    char *f[NUM_FIELDS];
    int field_size;
};

struct message *create_message(int field_size) {
    struct message *m = malloc(sizeof(struct message));
    m->field_size = field_size;

    for (int i = 0; i < NUM_FIELDS; i++) {
        m->f[i] = malloc(field_size);
        snprintf(m->f[i], field_size, "field%d", i);
    }
    return m;
}

void free_message(struct message *m) {
    for (int i = 0; i < NUM_FIELDS; i++)
        free(m->f[i]);
    free(m);
}

struct thread_args {
    int sock;
    int field_size;
};

void *handle_client(void *arg) {
    struct thread_args *ta = arg;
    int c = ta->sock;

    struct message *m = create_message(ta->field_size);
    char recv_buf[NUM_FIELDS * ta->field_size];

    dprintf(1, "SERVER THREAD %lu CONNECTED\n",
            (unsigned long)pthread_self());

    while (1) {
        int n = recv(c, recv_buf, sizeof(recv_buf), 0);
        if (n <= 0)
            break;

        for (int i = 0; i < NUM_FIELDS; i++)
            send(c, m->f[i], ta->field_size, 0);
    }

    free_message(m);
    close(c);
    free(ta);
    return NULL;
}

int main(int argc, char *argv[]) {
    /* join server network namespace */
    int fd = open("/var/run/netns/ns_server", O_RDONLY);
    setns(fd, CLONE_NEWNET);
    close(fd);

    if (argc != 3) {
        write(
            2,
            "Usage: ./server <field_size> <max_threads>\n",
            sizeof("Usage: ./server <field_size> <max_threads>\n") - 1
        );
        return 1;
    }

    int field_size = atoi(argv[1]);
    int max_threads = atoi(argv[2]);

    int s = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in a = {
        .sin_family = AF_INET,
        .sin_port = htons(8080),
        .sin_addr.s_addr = INADDR_ANY
    };

    bind(s, (struct sockaddr *)&a, sizeof(a));
    listen(s, max_threads);

    while (1) {
        struct thread_args *ta = malloc(sizeof(struct thread_args));
        ta->sock = accept(s, 0, 0);
        ta->field_size = field_size;

        pthread_t t;
        pthread_create(&t, NULL, handle_client, ta);
        pthread_detach(t);
    }
}
