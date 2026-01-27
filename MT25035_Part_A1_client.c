// client.c
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main() {
    int sock;
    char buffer[1024];

    // 1. Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);

    // 2. Server address
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // 3. Connect
    connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr));

    // 4. Send
    send(sock, "Hello from client", 17, 0);

    // 5. Receive
    recv(sock, buffer, sizeof(buffer), 0);
    printf("Received: %s\n", buffer);

    // 6. Close
    close(sock);

    return 0;
}
