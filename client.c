#include <stdio.h>                // Standard I/O library
#include <stdlib.h>               // Standard library for memory allocation, etc.
#include <string.h>               // Library for string handling
#include <unistd.h>               // POSIX API for system calls
#include <pthread.h>              // Library for POSIX threads
#include <arpa/inet.h>            // Definitions for internet operations
#include <sys/socket.h>           // Library for socket programming

#define BUFFER_SIZE 1024          // Size of the buffer for messages

// Function to handle receiving messages from the server
void *receive_handler(void *arg) {
    int sockfd = *((int *)arg);   // Socket descriptor
    char buffer[BUFFER_SIZE];     // Buffer for messages

    while (1) {
        int receive = recv(sockfd, buffer, BUFFER_SIZE, 0);  // Receive message from the server
        if (receive > 0) {         // Check if message received successfully
            printf("%s", buffer);  // Print the received message
        } else if (receive == 0) { // If connection closed by the server
            break;                 // Exit the loop
        } else {
            perror("recv");        // Print error if receive fails
        }
    }

    return NULL;  // Return from the thread
}

int main() {
    int sockfd;                    // Socket descriptor
    struct sockaddr_in server_addr; // Address structure for the server
    char username[32];             // Buffer for the username
    pthread_t recv_thread;         // Thread identifier for receiving messages

    printf("Enter your username: ");
    fgets(username, 32, stdin);    // Read the username from stdin
    username[strcspn(username, "\n")] = '\0';  // Remove the newline character from the username

    if (strlen(username) < 2 || strlen(username) >= 31) {  // Check if username length is valid
        printf("Username must be between 2 and 31 characters.\n");
        return EXIT_FAILURE;      // Exit if username is invalid
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);  // Create a socket
    if (sockfd < 0) {             // Check if socket creation fails
        perror("socket");         // Print error if socket creation fails
        return EXIT_FAILURE;      // Exit if socket creation fails
    }

    server_addr.sin_family = AF_INET;           // Set address family
    server_addr.sin_port = htons(6667);         // Set port number
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");  // Set server IP address

    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {  // Connect to the server
        perror("connect");         // Print error if connection fails
        return EXIT_FAILURE;       // Exit if connection fails
    }

    send(sockfd, username, 32, 0);  // Send the username to the server

    if (pthread_create(&recv_thread, NULL, receive_handler, (void*)&sockfd) != 0) {  // Create a thread to receive messages
        perror("pthread_create");  // Print error if thread creation fails
        return EXIT_FAILURE;       // Exit if thread creation fails
    }

    char buffer[BUFFER_SIZE];     // Buffer for sending messages
    while (1) {
        fgets(buffer, BUFFER_SIZE, stdin);  // Read message from stdin
        str_trim_lf(buffer, BUFFER_SIZE);  // Trim newline character

        if (strcmp(buffer, "/exit") == 0) {  // Check if the message is the exit command
            break;  // Exit the loop
        }

        send(sockfd, buffer, strlen(buffer), 0);  // Send the message to the server
    }

    close(sockfd);  // Close the socket
    return EXIT_SUCCESS;  // Exit successfully
}

// Function to trim the newline character from a string
void str_trim_lf(char* arr, int length) {
    for (int i = 0; i < length; i++) {
        if (arr[i] == '\n') {     // Check for newline character
            arr[i] = '\0';        // Replace newline with null terminator
            break;
        }
    }
}

