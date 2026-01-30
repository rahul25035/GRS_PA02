//MT25035_Part_A_A2_client.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

int main(int argc, char *argv[]) {
    if (argc != 5) {
        printf("Usage: %s <server_ip> <port> <message_size> <duration_sec>\n",
               argv[0]);
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

    size_t msg_bytes = msg_size * 8;
    char *recv_buf = malloc(msg_bytes);

    long bytes_received = 0;
    time_t start = time(NULL);
    time_t last = start;

    while (time(NULL) - start < duration) {
        ssize_t r = recv(sock, recv_buf, msg_bytes, 0);
        if (r <= 0)
            break;

        bytes_received += r;

        time_t now = time(NULL);
        if (now != last) {
            printf("[Client-A2] Bytes received: %ld\n", bytes_received);
            last = now;
        }
    }

    printf("[Client-A2] Done. Total bytes received = %ld\n",
           bytes_received);

    free(recv_buf);
    close(sock);
    return 0;
}
