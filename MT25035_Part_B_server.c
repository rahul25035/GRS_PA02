#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int main(int argc, char *argv[]) {
    int server_fd, client_fd;
    char buffer[65536];

    int flags = 0;
    if (argc != 2) {
        printf("Usage: %s -t|-o|-z\n", argv[0]);
        return 1;
    }
    if (strcmp(argv[1], "-z") == 0)
        flags = MSG_ZEROCOPY;

    printf("[SERVER] Starting in %s mode\n", argv[1]);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(8080),
        .sin_addr.s_addr = INADDR_ANY
    };

    bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));
    listen(server_fd, 16);

    while (1) {
        client_fd = accept(server_fd, NULL, NULL);
        printf("[SERVER] Client connected\n");

        recv(client_fd, buffer, sizeof(buffer), 0);
        printf("[SERVER] Received request\n");

        struct iovec iov = {
            .iov_base = buffer,
            .iov_len  = strlen(buffer) + 1
        };

        struct msghdr msg = {0};
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;

        if (flags) {
            sendmsg(client_fd, &msg, flags);
            printf("[SERVER] Reply sent (zero-copy path)\n");
        } else {
            send(client_fd, buffer, iov.iov_len, 0);
            printf("[SERVER] Reply sent (copy path)\n");
        }

        close(client_fd);
        printf("[SERVER] Client handled and connection closed\n");
    }
}
