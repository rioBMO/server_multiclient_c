#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main() {
    int sock;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    char message[BUFFER_SIZE];

    // Membuat socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Terhubung ke server
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection to the server failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

    printf("Connected to the server at port %d.\n", PORT);
    printf("Use 'signup <username> <password>' to register.\n");
    printf("Use 'login <username> <password>' to log in.\n");

    fd_set read_fds;
    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        FD_SET(sock, &read_fds);

        int max_fd = sock > STDIN_FILENO ? sock : STDIN_FILENO;
        int activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);

        if (activity < 0) {
            perror("Select failed");
            break;
        }

        // Jika input dari keyboard
        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            memset(message, 0, BUFFER_SIZE);
            fgets(message, BUFFER_SIZE, stdin);

            if (strncmp(message, "exit", 4) == 0) {
                printf("Exiting...\n");
                break;
            }

            send(sock, message, strlen(message), 0);
        }

        // Jika ada pesan dari server
        if (FD_ISSET(sock, &read_fds)) {
            memset(buffer, 0, BUFFER_SIZE);
            int bytes_read = recv(sock, buffer, BUFFER_SIZE - 1, 0);
            if (bytes_read <= 0) {
                printf("Server disconnected.\n");
                break;
            }

            printf("Server: %s", buffer);
        }
    }

    close(sock);
    return 0;
}
