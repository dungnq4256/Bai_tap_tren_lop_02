#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/time.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

void handle_client(int client_fd, int* client_count) {
    char buffer[BUFFER_SIZE];

    // Send welcome message with number of connected clients
    char welcome_msg[BUFFER_SIZE];
    sprintf(welcome_msg, "Xin chào. Hiện có %d clients đang kết nối.\n", *client_count);
    send(client_fd, welcome_msg, strlen(welcome_msg), 0);

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int valread = read(client_fd, buffer, BUFFER_SIZE);

        if (valread <= 0) {
            // Connection closed by client
            printf("Client disconnected\n");
            break;
        }

        // Check if client wants to exit
        if (strcmp(buffer, "exit\n") == 0) {
            send(client_fd, "Xin chào tạm biệt.\n", strlen("Xin chào tạm biệt.\n"), 0);
            printf("Client requested to exit\n");
            break;
        }

        // Normalize the string
        int i = 0, j = 0;
        int word_start = 1;
        while (buffer[i] != '\0') {
            if (buffer[i] == ' ') {
                if (!word_start)
                    buffer[j++] = ' ';
                word_start = 1;
            } else if ((buffer[i] >= 'A' && buffer[i] <= 'Z') || (buffer[i] >= 'a' && buffer[i] <= 'z')) {
                if (word_start) {
                    buffer[j++] = (buffer[i] >= 'a' && buffer[i] <= 'z') ? buffer[i] - 32 : buffer[i];
                    word_start = 0;
                } else {
                    buffer[j++] = (buffer[i] >= 'A' && buffer[i] <= 'Z') ? buffer[i] + 32 : buffer[i];
                }
            }
            i++;
        }
        buffer[j] = '\0';

        // Send the normalized string back to the client
        send(client_fd, buffer, strlen(buffer), 0);
    }

    close(client_fd);
    (*client_count)--;
}

int main() {
    int server_fd, new_socket, activity, valread;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    int opt = 1;
    char buffer[BUFFER_SIZE];
    int client_sockets[MAX_CLIENTS] = {0};
    int client_count = 0;
    fd_set readfds;
    int max_fd, i;

    // Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0) {
        perror("Setsockopt failed");
        exit(EXIT_FAILURE);
    }

    // Set server address
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    // Bind socket to address
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Start listening for connections
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server started. Waiting for connections...\n");

    while (1) {
        // Clear the socket set
        FD_ZERO(&readfds);

        // Add server socket to the set
        FD_SET(server_fd, &readfds);
        max_fd = server_fd;

        // Add client sockets to the set
        for (i = 0; i < MAX_CLIENTS; i++) {
            int client_fd = client_sockets[i];
            if (client_fd > 0) {
                FD_SET(client_fd, &readfds);
                if (client_fd > max_fd)
                    max_fd = client_fd;
            }
        }

        // Wait for activity on any of the sockets
        activity = select(max_fd + 1, &readfds, NULL, NULL, NULL);
        if (activity < 0) {
            perror("Select failed");
            exit(EXIT_FAILURE);
        }

        // Check for new connection
        if (FD_ISSET(server_fd, &readfds)) {
            if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
                perror("Accept failed");
                exit(EXIT_FAILURE);
            }

            printf("New client connected\n");

            // Add new client to the list
            int client_index = -1;
            for (i = 0; i < MAX_CLIENTS; i++) {
                if (client_sockets[i] == 0) {
                    client_sockets[i] = new_socket;
                    client_index = i;
                    break;
                }
            }

            if (client_index == -1) {
                printf("Maximum number of clients reached. Connection rejected.\n");
                close(new_socket);
            } else {
                client_count++;
                handle_client(new_socket, &client_count);
            }
        }

        // Check for client activity
        for (i = 0; i < MAX_CLIENTS; i++) {
            int client_fd = client_sockets[i];
            if (client_fd > 0 && FD_ISSET(client_fd, &readfds)) {
                handle_client(client_fd, &client_count);
            }
        }
    }

    return 0;
}
