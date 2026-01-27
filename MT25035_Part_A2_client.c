#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int main() {
    int sock;

    /* Pre-registered buffer */
    static char buffer[1024];

    sock = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr));

    /* sendmsg setup */
    struct iovec iov;
    struct msghdr msg;

    strcpy(buffer, "Hello from client (sendmsg)");

    iov.iov_base = buffer;
    iov.iov_len  = strlen(buffer) + 1;

    memset(&msg, 0, sizeof(msg));
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    sendmsg(sock, &msg, 0);

    /* Receive reply */
    recv(sock, buffer, sizeof(buffer), 0);
    printf("Received: %s\n", buffer);

    close(sock);
    return 0;
}
