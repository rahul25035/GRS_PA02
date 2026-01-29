#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

int main(int argc, char *argv[]) {
    if (argc != 5) {
        printf("Usage: %s <server_ip> <port> <message_size> <duration_sec>\n", argv[0]);
        return 1;
    }

    char *server_ip = argv[1];
    int port = atoi(argv[2]);
    int msg_size = atoi(argv[3]);
    int duration = atoi(argv[4]);

    int sock = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, server_ip, &addr.sin_addr);

    connect(sock, (struct sockaddr *)&addr, sizeof(addr));
    printf("[Client-A2] Connected\n");

    char *send_buf = malloc(msg_size);
    char *recv_buf = malloc(msg_size * 8);
    memset(send_buf, 'X', msg_size);

    long bytes_sent = 0;
    time_t start = time(NULL);
    time_t last = start;

    while (time(NULL) - start < duration) {
        ssize_t s = send(sock, send_buf, msg_size, 0);
        if (s > 0)
            bytes_sent += s;

        recv(sock, recv_buf, msg_size * 8, MSG_DONTWAIT);

        time_t now = time(NULL);
        if (now != last) {
            printf("[Client-A2] Bytes sent: %ld\n", bytes_sent);
            last = now;
        }
    }

    printf("[Client-A2] Done. Total bytes sent = %ld\n", bytes_sent);

    free(send_buf);
    free(recv_buf);
    close(sock);
    return 0;
}
