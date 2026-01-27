#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int main() {
    int server_fd, client_fd;
    static char buffer[1024];

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));
    listen(server_fd, 1);

    printf("Server listening...\n");
    client_fd = accept(server_fd, NULL, NULL);

    recv(client_fd, buffer, sizeof(buffer), 0);
    printf("Received: %s\n", buffer);

    strcpy(buffer, "Hello from server (MSG_ZEROCOPY)");

    struct iovec iov;
    struct msghdr msg;

    iov.iov_base = buffer;
    iov.iov_len  = strlen(buffer) + 1;

    memset(&msg, 0, sizeof(msg));
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    sendmsg(client_fd, &msg, MSG_ZEROCOPY);

    close(client_fd);
    close(server_fd);
    return 0;
}
