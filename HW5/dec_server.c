/**
 * dec_server.c
 *
 * Description:
 * ------------
 * This program implements the decryption server for a one-time pad encryption/decryption system.
 * The server listens on a specified port for incoming connections from legitimate clients (dec_client).
 * Upon establishing a connection, the server authenticates the client, receives ciphertext and key,
 * performs decryption, and sends the plaintext back to the client.
 *
 * Features:
 * ---------
 * - Handles multiple client connections concurrently using forked child processes.
 * - Implements signal handling to reap zombie processes.
 * - Validates client identity to ensure only authorized clients can connect.
 * - Performs error checking and handles various error conditions gracefully.
 *
 * Compilation:
 * ------------
 * To compile the program, use the following command:
 *
 *     gcc -std=gnu99 -Wall -Wextra -o dec_server dec_server.c
 *
 * Usage:
 * ------
 * Run the server by specifying a listening port:
 *
 *     ./dec_server <listening_port>
 *
 * Example:
 *
 *     ./dec_server 57172
 *
 * Author:
 * -------
 * Ethan Clinick
 * CS374 HW5 FALL 2024
 * 12/2/2024
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ctype.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/wait.h>

#define MAX_CONNECTIONS 5    // Maximum number of queued connections
#define BUFFER_SIZE 1024      // Buffer size for data transmission

/**
 * @brief Handles communication with a connected client.
 *
 * This function performs the following steps:
 * 1. Authenticates the client by verifying its identity.
 * 2. Receives ciphertext and key from the client.
 * 3. Validates the lengths of ciphertext and key.
 * 4. Performs decryption using the one-time pad algorithm.
 * 5. Sends the decrypted plaintext back to the client.
 * 6. Cleans up resources and exits the child process.
 *
 * @param connection_fd File descriptor for the connected client socket.
 */
void handle_client(int connection_fd) {
    // Authenticate Client
    char id_buffer[32];
    memset(id_buffer, 0, sizeof(id_buffer));

    // Receive client ID
    ssize_t bytes_received = recv(connection_fd, id_buffer, sizeof(id_buffer) - 1, 0);
    if (bytes_received <= 0) {
        fprintf(stderr, "dec_server error: failed to receive client ID\n");
        close(connection_fd);
        exit(1);
    }
    id_buffer[bytes_received] = '\0'; // Null-terminate the received string

    // Verify client identity
    if (strncmp(id_buffer, "dec_client", 10) != 0) { // Check for exact match
        char *error = "INVALID_CLIENT";
        send(connection_fd, error, strlen(error), 0);
        fprintf(stderr, "dec_server error: connected client is not dec_client\n");
        close(connection_fd);
        exit(2);
    }

    // Send acknowledgment to client
    char *ack = "DEC_SERVER_ACK";
    size_t ack_len = strlen(ack);
    size_t total_sent = 0;

    // Ensure the entire acknowledgment message is sent
    while (total_sent < ack_len) {
        ssize_t sent = send(connection_fd, ack + total_sent, ack_len - total_sent, 0);
        if (sent < 0) {
            fprintf(stderr, "dec_server error: failed to send acknowledgment\n");
            close(connection_fd);
            exit(1);
        }
        total_sent += sent;
    }

    // Receive Ciphertext Length
    int ciphertext_len;
    ssize_t recv_ret = recv(connection_fd, &ciphertext_len, sizeof(int), 0);
    if (recv_ret < (ssize_t)sizeof(int)) {
        fprintf(stderr, "dec_server error: failed to receive ciphertext length\n");
        close(connection_fd);
        exit(1);
    }

    // : Receive Key Length
    int key_len;
    recv_ret = recv(connection_fd, &key_len, sizeof(int), 0);
    if (recv_ret < (ssize_t)sizeof(int)) {
        fprintf(stderr, "dec_server error: failed to receive key length\n");
        close(connection_fd);
        exit(1);
    }

    // Validate key length
    if (key_len < ciphertext_len) {
        fprintf(stderr, "dec_server error: key length (%d) is less than ciphertext length (%d)\n", key_len, ciphertext_len);
        close(connection_fd);
        exit(1);
    }

    // Allocate Memory for Ciphertext and Key
    char *ciphertext = malloc(ciphertext_len + 1);
    char *key = malloc(key_len + 1);
    if (!ciphertext || !key) {
        fprintf(stderr, "dec_server error: memory allocation failed\n");
        close(connection_fd);
        exit(1);
    }

    // Receive Ciphertext Data
    int total_received = 0;
    while (total_received < ciphertext_len) {
        ssize_t n = recv(connection_fd, ciphertext + total_received, ciphertext_len - total_received, 0);
        if (n <= 0) {
            fprintf(stderr, "dec_server error: failed to receive ciphertext\n");
            free(ciphertext);
            free(key);
            close(connection_fd);
            exit(1);
        }
        total_received += n;
    }
    ciphertext[ciphertext_len] = '\0'; // Null-terminate ciphertext

    //  Receive Key Data (only as much as ciphertext)
    total_received = 0;
    while (total_received < ciphertext_len) { // Only need as much key as ciphertext
        ssize_t n = recv(connection_fd, key + total_received, ciphertext_len - total_received, 0);
        if (n <= 0) {
            fprintf(stderr, "dec_server error: failed to receive key\n");
            free(ciphertext);
            free(key);
            close(connection_fd);
            exit(1);
        }
        total_received += n;
    }
    key[ciphertext_len] = '\0'; // Null-terminate key

    //  Perform Decryption
    char *plaintext = malloc(ciphertext_len + 1);
    if (!plaintext) {
        fprintf(stderr, "dec_server error: memory allocation failed\n");
        free(ciphertext);
        free(key);
        close(connection_fd);
        exit(1);
    }

    // Decrypt each character
    for (int i = 0; i < ciphertext_len; i++) {
        int c_val, k_val, p_val;

        // Convert ciphertext character to numerical value
        if (ciphertext[i] == ' ')
            c_val = 26;
        else
            c_val = ciphertext[i] - 'A';

        // Convert key character to numerical value
        if (key[i] == ' ')
            k_val = 26;
        else
            k_val = key[i] - 'A';

        // Perform modular subtraction for decryption
        p_val = (c_val - k_val + 27) % 27;

        // Convert numerical value back to character
        if (p_val == 26)
            plaintext[i] = ' ';
        else
            plaintext[i] = 'A' + p_val;
    }
    plaintext[ciphertext_len] = '\0'; // Null-terminate plaintext

    // Send Plaintext Length to Client
    if (send(connection_fd, &ciphertext_len, sizeof(int), 0) < 0) {
        fprintf(stderr, "dec_server error: failed to send plaintext length\n");
    }

    // Send Plaintext Data to Client
    size_t total_sent_plain = 0;
    while (total_sent_plain < ciphertext_len) {
        ssize_t n = send(connection_fd, plaintext + total_sent_plain, ciphertext_len - total_sent_plain, 0);
        if (n < 0) {
            fprintf(stderr, "dec_server error: failed to send plaintext\n");
            break;
        }
        total_sent_plain += n;
    }

    // Clean Up Resources
    free(ciphertext);
    free(key);
    free(plaintext);
    close(connection_fd);
    exit(0); // Child process exits after handling client
}

/**
 * @brief Signal handler to reap zombie child processes.
 *
 * This function is invoked when the SIGCHLD signal is received, indicating that
 * a child process has terminated. It uses a loop with waitpid to reap all dead
 * child processes, preventing the accumulation of zombie processes.
 *
 * @param signo The signal number (unused).
 */
void reap_zombies(int signo) {
    (void)signo; // Suppress unused parameter warning

    // Continuously reap all dead child processes
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

/**
 * @brief The main function initializes the server and starts listening for connections.
 *
 * It performs the following steps:
 * 1. Validates command-line arguments.
 * 2. Sets up signal handling for SIGCHLD to reap zombie processes.
 * 3. Creates a listening socket.
 * 4. Binds the socket to the specified port.
 * 5. Begins listening for incoming connections.
 * 6. Enters the server loop to accept and handle client connections.
 *
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return int Exit status.
 */
int main(int argc, char *argv[]) {
    //  Validate Command-Line Arguments
    if (argc != 2) {
        fprintf(stderr, "Usage: dec_server listening_port\n");
        exit(1);
    }

    // Convert port argument to integer
    int port = atoi(argv[1]);
    if (port < 1024 || port > 65535) {
        fprintf(stderr, "dec_server error: invalid port number\n");
        exit(1);
    }

    // Set Up Signal Handler to Reap Zombies
    struct sigaction sa;
    sa.sa_handler = reap_zombies;        // Assign the signal handler function
    sigemptyset(&sa.sa_mask);            // Initialize the signal mask
    sa.sa_flags = SA_RESTART;            // Restart interrupted system calls

    // Register the signal handler for SIGCHLD
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        fprintf(stderr, "dec_server error: cannot handle SIGCHLD\n");
        exit(1);
    }

    //  Create a Listening Socket
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        fprintf(stderr, "dec_server error: cannot open socket\n");
        exit(1);
    }

    //  Allow Reuse of Address
    int yes = 1;
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0) {
        fprintf(stderr, "dec_server error: setsockopt failed\n");
        close(listen_fd);
        exit(1);
    }

    //  Bind the Socket to the Specified Port
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));       // Zero out the structure
    server_addr.sin_family = AF_INET;                   // IPv4
    server_addr.sin_port = htons(port);                 // Host to network byte order
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);    // Listen on all interfaces

    if (bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        fprintf(stderr, "dec_server error: bind failed on port %d\n", port);
        close(listen_fd);
        exit(1);
    }

    //  Listen for Incoming Connections
    if (listen(listen_fd, MAX_CONNECTIONS) < 0) {
        fprintf(stderr, "dec_server error: listen failed\n");
        close(listen_fd);
        exit(1);
    }

    //  Server Loop to Accept and Handle Connections
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int connection_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_len);
        if (connection_fd < 0) {
            fprintf(stderr, "dec_server error: accept failed\n");
            continue; // Continue accepting new connections
        }

        // Fork a child process to handle the new connection
        pid_t pid = fork();
        if (pid < 0) {
            fprintf(stderr, "dec_server error: fork failed\n");
            close(connection_fd);
            continue; // Continue accepting new connections
        } else if (pid == 0) {
            // Child Process: Handle the client
            close(listen_fd);          // Close the listening socket in the child
            handle_client(connection_fd); // Process client communication
        } else {
            // Parent Process: Close the connected socket descriptor
            close(connection_fd);
        }
    }

    // Clean Up (Unreachable Code)
    close(listen_fd);
    return 0;
}