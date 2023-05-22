#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/time.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    if (argc < 4) {
        printf("Usage: %s <receiver IP> <receiver port> <listening port>\n", argv[0]);
        return 1;
    }

    char *receiver_ip = argv[1];
    int receiver_port = atoi(argv[2]);
    int listening_port = atoi(argv[3]);

    int sender_fd, receiver_fd, max_fd;
    struct sockaddr_in sender_address, receiver_address;
    char buffer[BUFFER_SIZE];

    // Create sender socket
    if ((sender_fd = socket(AF_INET, SOCK_DGRAM, 0)) == 0) {
        perror("Sender socket failed");
        exit(EXIT_FAILURE);
    }

    // Set sender address
    sender_address.sin_family = AF_INET;
    sender_address.sin_addr.s_addr = INADDR_ANY;
    sender_address.sin_port = htons(0); // Let the OS assign a random port

    // Bind sender socket to address
    if (bind(sender_fd, (struct sockaddr *)&sender_address, sizeof(sender_address)) < 0) {
        perror("Sender bind failed");
        exit(EXIT_FAILURE);
    }

    // Create receiver socket
    if ((receiver_fd = socket(AF_INET, SOCK_DGRAM, 0)) == 0) {
        perror("Receiver socket failed");
        exit(EXIT_FAILURE);
    }

    // Set receiver address
    receiver_address.sin_family = AF_INET;
    if (inet_pton(AF_INET, receiver_ip, &(receiver_address.sin_addr)) <= 0) {
        perror("Invalid address/Address not supported");
        exit(EXIT_FAILURE);
    }
    receiver_address.sin_port = htons(receiver_port);

    // Set listening address
    struct sockaddr_in listening_address;
    listening_address.sin_family = AF_INET;
    listening_address.sin_addr.s_addr = INADDR_ANY;
    listening_address.sin_port = htons(listening_port);

    // Bind receiver socket to listening address
    if (bind(receiver_fd, (struct sockaddr *)&listening_address, sizeof(listening_address)) < 0) {
        perror("Receiver bind failed");
        exit(EXIT_FAILURE);
    }

    // Set up the file descriptor set for select/poll
    fd_set readfds;
    max_fd = (sender_fd > receiver_fd) ? sender_fd : receiver_fd;

    printf("Chat started. Enter 'exit' to quit.\n");

    while (1) {
        // Clear the socket set
        FD_ZERO(&readfds);

        // Add sender and receiver sockets to the set
        FD_SET(sender_fd, &readfds);
        FD_SET(receiver_fd, &readfds);

        // Wait for activity on any of the sockets
        int activity = select(max_fd + 1, &readfds, NULL, NULL, NULL);
        if (activity < 0) {
            perror("Select failed");
            exit(EXIT_FAILURE);
        }

        // Check for sender activity
        if (FD_ISSET(sender_fd, &readfds)) {
            // Read input from user
            fgets(buffer, BUFFER_SIZE, stdin);

            // Remove trailing newline character
            buffer[strcspn(buffer, "\n")] = '\0';

            // Check if user wants to exit
            if (strcmp(buffer, "exit") == 0) {
                printf("Goodbye.\n");
                break;
            }

            // Send the message to the receiver
            if (sendto(sender_fd, buffer, strlen(buffer), 0, (struct sockaddr *)&receiver_address, sizeof(receiver_address)) < 0) {
                perror("Sendto failed");
                exit(EXIT_FAILURE);
            }
        }

        // Check for receiver activity
        if (FD_ISSET(receiver_fd, &readfds)) {
            memset(buffer, 0, sizeof(buffer));

            // Receive message from the sender
            if (recvfrom(receiver_fd, buffer, BUFFER_SIZE, 0, NULL, NULL) < 0) {
                perror("Recvfrom failed");
                exit(EXIT_FAILURE);
            }

            printf("Received message: %s\n", buffer);
        }
    }

    close(sender_fd);
    close(receiver_fd);

    return 0;
}
