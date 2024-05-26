#include <stdio.h>                // Standard I/O library
#include <stdlib.h>               // Standard library for memory allocation, etc.
#include <string.h>               // Library for string handling
#include <unistd.h>               // POSIX API for system calls
#include <pthread.h>              // Library for POSIX threads
#include <arpa/inet.h>            // Definitions for internet operations
#include <netinet/in.h>           // Definitions for internet address family
#include <sys/socket.h>           // Library for socket programming
#include <sys/types.h>            // Definitions for data types

#define PORT 6667                 // Port number for the server
#define BUFFER_SIZE 1024          // Size of the buffer for messages
#define MAX_CLIENTS 100           // Maximum number of clients

typedef struct {
    int socket;                   // Socket descriptor
    char username[32];            // Username of the client
    char channel[32];             // Channel the client is in
} client_t;

client_t *clients[MAX_CLIENTS];   // Array of client structures
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;  // Mutex for synchronizing access to clients array

// Function to trim the newline character from a string
void str_trim_lf(char* arr, int length) {
    for (int i = 0; i < length; i++) {
        if (arr[i] == '\n') {     // Check for newline character
            arr[i] = '\0';        // Replace newline with null terminator
            break;
        }
    }
}

// Function to send a message to all clients in the same channel
void send_message_to_channel(char *message, char *channel, int sender_socket) {
    pthread_mutex_lock(&clients_mutex);  // Lock the mutex
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i] && strcmp(clients[i]->channel, channel) == 0) {  // Check if client is in the same channel
            if (clients[i]->socket != sender_socket) {  // Do not send to the sender
                if (send(clients[i]->socket, message, strlen(message), 0) < 0) {  // Send the message
                    perror("send");  // Print error if send fails
                    break;
                }
            }
        }
    }
    pthread_mutex_unlock(&clients_mutex);  // Unlock the mutex
}

// Function to handle communication with a client
void *handle_client(void *arg) {
    char buffer[BUFFER_SIZE];  // Buffer for receiving messages
    char message[BUFFER_SIZE + 32];  // Buffer for sending messages
    int leave_flag = 0;  // Flag to indicate if the client wants to leave
    client_t *cli = (client_t *)arg;  // Cast the argument to client_t

    // Notify server that a new connection is accepted
    printf("New connection accepted\n");
    
    // Receive the username from the client
    if (recv(cli->socket, cli->username, 32, 0) <= 0 || strlen(cli->username) < 2 || strlen(cli->username) >= 32 - 1) {
        printf("Didn't enter the name.\n");  // Print error if username is invalid
        leave_flag = 1;  // Set the leave flag
    } else {
        str_trim_lf(cli->username,strlen(cli->username));
        snprintf(buffer, sizeof(buffer), "%s has joined\n", cli->username);  // Create join message
        printf("%s", buffer);  // Print join message
        send_message_to_channel(buffer, cli->channel, cli->socket);  // Send join message to the channel
    }
    
    // Send greeting message to the client
    char greeting_message[BUFFER_SIZE];
    snprintf(greeting_message, sizeof(greeting_message), "Hello %s, you are now connected. Type /help for the list of commands.\n", cli->username);
    send(cli->socket, greeting_message, strlen(greeting_message), 0);
    while (1) {
        if (leave_flag) {  // Check if the client wants to leave
            break;
        }

        int receive = recv(cli->socket, buffer, BUFFER_SIZE, 0);  // Receive message from the client
        if (receive > 0) {  // Check if message is received
            if (strlen(buffer) > 0) {  // Check if buffer is not empty
                str_trim_lf(buffer, strlen(buffer));  // Trim newline character
                if (buffer[0] == '/') {  // Check if the message is a command
                    if (strncmp(buffer, "/list-channels", 14) == 0) {  // List channels command
                        pthread_mutex_lock(&clients_mutex);  // Lock the mutex
                        snprintf(message, sizeof(message), "Available channels:\n");  // Create list channels message
                        for (int i = 0; i < MAX_CLIENTS; ++i) {
                            if (clients[i] && strlen(clients[i]->channel) > 0) {  // Check if client is in a channel
                                strncat(message, clients[i]->channel, sizeof(message) - strlen(message) - 1);  // Add channel to message
                                strncat(message, "\n", sizeof(message) - strlen(message) - 1);  // Add newline to message
                            }
                        }
                        pthread_mutex_unlock(&clients_mutex);  // Unlock the mutex
                        send(cli->socket, message, strlen(message), 0);  // Send list channels message to the client
                    } else if (strncmp(buffer, "/list-users", 11) == 0) {  // List users command
                        pthread_mutex_lock(&clients_mutex);  // Lock the mutex
                        snprintf(message, sizeof(message), "Online users:\n");  // Create list users message
                        for (int i = 0; i < MAX_CLIENTS; ++i) {
                            if (clients[i]) {  // Check if client is online
                                strncat(message, clients[i]->username, sizeof(message) - strlen(message) - 1);  // Add username to message
                                strncat(message, "\n", sizeof(message) - strlen(message) - 1);  // Add newline to message
                            }
                        }
                        pthread_mutex_unlock(&clients_mutex);  // Unlock the mutex
                        send(cli->socket, message, strlen(message), 0);  // Send list users message to the client
                    } else if (strncmp(buffer, "/join", 5) == 0) {  // Join channel command
                        char *channel = strtok(buffer + 6, " ");  // Parse the channel name
                        if (channel && channel[0] == '#') {  // Check if the channel name is valid
                            strcpy(cli->channel, channel);  // Set the client's channel
                            snprintf(buffer, sizeof(buffer), "%s joined channel %s\n", cli->username, cli->channel);  // Create join channel message
                            send_message_to_channel(buffer, cli->channel, cli->socket);  // Send join channel message to the channel
                        } else {
                            send(cli->socket, "Channel name must start with #\n", 32, 0);  // Send error message if channel name is invalid
                        }
                    } else if (strncmp(buffer, "/leave", 6) == 0) {  // Leave channel command
                        snprintf(buffer, sizeof(buffer), "%s left the channel\n", cli->username);  // Create leave channel message
                        send_message_to_channel(buffer, cli->channel, cli->socket);  // Send leave channel message to the channel
                        memset(cli->channel, 0, sizeof(cli->channel));  // Clear the client's channel
                    } else if (strncmp(buffer, "/exit", 5) == 0) {  // Exit command
                        snprintf(buffer, sizeof(buffer), "%s has left\n", cli->username);  // Create exit message
                        printf("%s", buffer);  // Print exit message
                        send_message_to_channel(buffer, cli->channel, cli->socket);  // Send exit message to the channel
                        leave_flag = 1;  // Set the leave flag
                    } else if (strncmp(buffer, "/help", 5) == 0) {  // Help command
                        snprintf(message, sizeof(message), "/list-channels: see a list of available channels.\n/list-users: see a list of online users.\n/join [channel_name]: to join a channel.\n/leave: to exit the channel.\n/exit: to disconnect from the server.\n/help: to see this message.\n");  // Create help message
                        send(cli->socket, message, strlen(message), 0);  // Send help message to the client
                    } else {
                        send(cli->socket, "command not found, try /help\n", 28, 0);  // Send error message if command is not found
                    }
                } else {
                    snprintf(message, sizeof(message), "%s: %.*s\n", cli->username, BUFFER_SIZE - 35, buffer);  // Create chat message
                    send_message_to_channel(message, cli->channel, cli->socket);  // Send chat message to the channel
                }
            }
        } else if (receive == 0 || strcmp(buffer, "/exit") == 0) {  // Check if client wants to exit
            snprintf(buffer, sizeof(buffer), "%s has left\n", cli->username);  // Create exit message
            printf("%s", buffer);  // Print exit message
            send_message_to_channel(buffer, cli->channel, cli->socket);  // Send exit message to the channel
            leave_flag = 1;  // Set the leave flag
        } else {
            perror("recv");  // Print error if receive fails
            leave_flag = 1;  // Set the leave flag
        }
    }

    close(cli->socket);  // Close the client's socket
    pthread_mutex_lock(&clients_mutex);  // Lock the mutex
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i] == cli) {  // Find the client in the clients array
            clients[i] = NULL;  // Remove the client from the clients array
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);  // Unlock the mutex
    free(cli);  // Free the client's memory
    pthread_detach(pthread_self());  // Detach the thread
    return NULL;
}

// Function to print the client's address
void print_client_addr(struct sockaddr_in addr) {
    printf("%d.%d.%d.%d",
           addr.sin_addr.s_addr & 0xff,
           (addr.sin_addr.s_addr & 0xff00) >> 8,
           (addr.sin_addr.s_addr & 0xff0000) >> 16,
           (addr.sin_addr.s_addr & 0xff000000) >> 24);
}

int main() {
    int server_socket, client_socket;  // Socket descriptors
    struct sockaddr_in server_addr, client_addr;  // Address structures
    pthread_t tid;  // Thread identifier

    server_socket = socket(AF_INET, SOCK_STREAM, 0);  // Create a socket
    server_addr.sin_family = AF_INET;  // Set address family
    server_addr.sin_addr.s_addr = INADDR_ANY;  // Set IP address
    server_addr.sin_port = htons(PORT);  // Set port number

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {  // Bind the socket to the address
        perror("bind");  // Print error if bind fails
        return EXIT_FAILURE;
    }

    if (listen(server_socket, 10) < 0) {  // Listen for incoming connections
        perror("listen");  // Print error if listen fails
        return EXIT_FAILURE;
    }

    printf("Server started on port %d\n", PORT);  // Print server started message

    while (1) {
        socklen_t client_len = sizeof(client_addr);  // Get size of client address structure
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);  // Accept incoming connection

        if ((client_socket) < 0) {  // Check if accept fails
            perror("accept");  // Print error if accept fails
            return EXIT_FAILURE;
        }

        if ((client_socket) >= MAX_CLIENTS) {  // Check if maximum clients reached
            printf("Max clients reached. Rejected: ");
            print_client_addr(client_addr);  // Print client address
            close(client_socket);  // Close client socket
            continue;  // Continue to next iteration
        }

        client_t *cli = (client_t *)malloc(sizeof(client_t));  // Allocate memory for client
        cli->socket = client_socket;  // Set client socket
        strcpy(cli->channel, "");  // Initialize client channel

        pthread_mutex_lock(&clients_mutex);  // Lock the mutex
        for (int i = 0; i < MAX_CLIENTS; ++i) {
            if (!clients[i]) {  // Find an empty spot in the clients array
                clients[i] = cli;  // Add the client to the clients array
                break;
            }
        }
        pthread_mutex_unlock(&clients_mutex);  // Unlock the mutex

        pthread_create(&tid, NULL, handle_client, (void*)cli);  // Create a thread for the client
    }

    return EXIT_SUCCESS;  // Exit successfully
}

