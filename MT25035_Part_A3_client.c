#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int main() {
    int sock;
    static char buffer[1024];

    sock = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    connect(sock, (struct sockaddr *)&addr, sizeof(addr));

    strcpy(buffer, "Hello from client (MSG_ZEROCOPY)");

    struct iovec iov;
    struct msghdr msg;

    iov.iov_base = buffer;
    iov.iov_len  = strlen(buffer) + 1;

    memset(&msg, 0, sizeof(msg));
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    sendmsg(sock, &msg, MSG_ZEROCOPY);

    recv(sock, buffer, sizeof(buffer), 0);
    printf("Received: %s\n", buffer);

    close(sock);
    return 0;
}
