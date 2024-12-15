#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <sys/mman.h>
#include <signal.h>
#include <errno.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_USERS 100
#define USER_FILE "users.txt" // File untuk menyimpan data pengguna

// Struct untuk menyimpan data user
typedef struct {
    char username[50];
    char password[50];
    int in_use; // Menandakan apakah user sedang login
} User;

// Shared memory for users and user count
User *users;
int *user_count;

// Fungsi untuk menyimpan data pengguna ke file
void save_users_to_file() {
    FILE *file = fopen(USER_FILE, "w");
    if (file == NULL) {
        perror("Error opening file");
        return;
    }

    for (int i = 0; i < *user_count; i++) {
        fprintf(file, "%s %s %d\n", users[i].username, users[i].password, users[i].in_use);
    }

    fclose(file);
}

// Fungsi untuk memuat data pengguna dari file
void load_users_from_file() {
    FILE *file = fopen(USER_FILE, "r");
    if (!file) {
        printf("Error: Could not open users file.\n");
        return;
    }

    *user_count = 0;
    while (fscanf(file, "%s %s %d", users[*user_count].username, users[*user_count].password, &users[*user_count].in_use) == 3) {
        (*user_count)++;
    }

    fclose(file);
}

// Fungsi untuk menangani client
void handle_client(int client_sock) {
    char buffer[BUFFER_SIZE];
    char response[BUFFER_SIZE];
    char logged_user[50] = "";
    int logged_in = 0;

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_read = read(client_sock, buffer, BUFFER_SIZE - 1);
        if (bytes_read <= 0) {
            printf("%s disconnected.\n", logged_user);

            if (logged_in) {
                for (int i = 0; i < *user_count; i++) {
                    if (strcmp(users[i].username, logged_user) == 0) {
                        users[i].in_use = 0; // Reset status login
                        break;
                    }
                }
                save_users_to_file(); // Simpan perubahan ke file
            }

            close(client_sock);
            exit(0);
        }

        buffer[strcspn(buffer, "\n")] = 0; // Remove newline

        if (strncmp(buffer, "signup", 6) == 0) {
            char username[50], password[50];
            int args = sscanf(buffer + 7, "%s %s", username, password);

            if (args != 2) {
                snprintf(response, BUFFER_SIZE, "Error: Invalid signup format. Use 'signup <username> <password>'\n");
            } else {
                int exists = 0;
                for (int i = 0; i < *user_count; i++) {
                    if (strcmp(users[i].username, username) == 0) {
                        exists = 1;
                        break;
                    }
                }
                if (exists) {
                    snprintf(response, BUFFER_SIZE, "Error: Username already exists.\n");
                } else {
                    // Menambahkan user baru ke dalam array dan menyimpan ke file
                    strcpy(users[*user_count].username, username);
                    strcpy(users[*user_count].password, password);
                    users[*user_count].in_use = 0;
                    (*user_count)++;
                    save_users_to_file(); // Simpan data pengguna baru ke file
                    snprintf(response, BUFFER_SIZE, "Signup successful.\n");
                }
            }
            write(client_sock, response, strlen(response));
        } else if (strncmp(buffer, "login", 5) == 0) {
            char username[50], password[50];
            int args = sscanf(buffer + 6, "%s %s", username, password);

            if (args != 2) {
                snprintf(response, BUFFER_SIZE, "Error: Invalid login format. Use 'login <username> <password>'\n");
            } else {
                int valid = 0;
                for (int i = 0; i < *user_count; i++) {
                    if (strcmp(users[i].username, username) == 0 && strcmp(users[i].password, password) == 0) {
                        if (users[i].in_use == 1) {
                            valid = -1; // User sedang login di client lain
                        } else {
                            valid = 1; // Login berhasil
                            users[i].in_use = 1; // Tandai user sedang login
                            strcpy(logged_user, username);
                            logged_in = 1;
                            save_users_to_file(); // Simpan perubahan status login
                        }
                        break;
                    }
                }

                if (valid == 1) {
                    snprintf(response, BUFFER_SIZE, "Login successful. Welcome, %s!\n", username);
                } else if (valid == -1) {
                    snprintf(response, BUFFER_SIZE, "Error: User is already logged in on another device.\n");
                } else {
                    snprintf(response, BUFFER_SIZE, "Error: Invalid username or password.\n");
                }
            }
            write(client_sock, response, strlen(response));
        } else if (logged_in) {
            snprintf(response, BUFFER_SIZE, "[%s]: %s\n", logged_user, buffer);
            printf("%s", response); // Tampilkan pesan di server
            write(client_sock, response, strlen(response));
        } else {
            snprintf(response, BUFFER_SIZE, "Error: You must be logged in to send messages.\n");
            write(client_sock, response, strlen(response));
        }
    }
}

void sigchld_handler(int sig) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size;

    // Alokasi shared memory untuk users dan user_count
    users = mmap(NULL, sizeof(User) * MAX_USERS, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (users == MAP_FAILED) {
        perror("mmap failed");
        exit(EXIT_FAILURE);
    }

    user_count = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (user_count == MAP_FAILED) {
        perror("mmap failed");
        exit(EXIT_FAILURE);
    }
    *user_count = 0;

    // Memuat data pengguna dari file
    load_users_from_file();

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    if (listen(server_sock, 10) < 0) {
        perror("Listen failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    signal(SIGCHLD, sigchld_handler);

    printf("Server listening on port %d...\n", PORT);

    while (1) {
        addr_size = sizeof(client_addr);
        client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &addr_size);
        if (client_sock < 0) {
            if (errno == EINTR) {
                continue;
            } else {
                perror("Accept failed");
                continue;
            }
        }

        printf("New client connected.\n");

        if (fork() == 0) {
            close(server_sock);
            handle_client(client_sock);
            exit(0);
        } else {
            close(client_sock);
        }
    }

    close(server_sock);
    return 0;
}
