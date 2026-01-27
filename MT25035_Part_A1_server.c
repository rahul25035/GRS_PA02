// server.c
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main() {
    int server_fd, client_fd;
    char buffer[1024];

    // 1. Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    // 2. Bind
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));

    // 3. Listen
    listen(server_fd, 1);
    printf("Server listening on port 8080...\n");

    // 4. Accept
    client_fd = accept(server_fd, NULL, NULL);

    // 5. Receive
    recv(client_fd, buffer, sizeof(buffer), 0);
    printf("Received: %s\n", buffer);

    // 6. Send
    send(client_fd, "Hello from server", 17, 0);

    // 7. Close
    close(client_fd);
    close(server_fd);

    return 0;
}
