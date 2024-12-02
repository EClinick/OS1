/**
 * dec_client.c
 *
 * Description:
 * ------------
 * This program implements the decryption client for a one-time pad encryption/decryption system.
 * The client connects to the decryption server (`dec_server`), sends ciphertext and key data,
 * receives the decrypted plaintext, and outputs it to standard output.
 *
 * Features:
 * ---------
 * - Connects to the specified decryption server using TCP sockets.
 * - Authenticates with the server to ensure proper client-server communication.
 * - Reads ciphertext and key from provided files, validating their integrity.
 * - Sends ciphertext and key data to the server for decryption.
 * - Receives decrypted plaintext from the server and displays it.
 * - Handles various error conditions gracefully with informative messages.
 *
 * Compilation:
 * ------------
 * To compile the program, use the following command:
 *
 *     gcc -std=gnu99 -Wall -Wextra -o dec_client dec_client.c
 *
 * Usage:
 * ------
 * Run the client by specifying the ciphertext file, key file, and server port:
 *
 *     ./dec_client <ciphertext_file> <key_file> <server_port>
 *
 * Example:
 *
 *     ./dec_client ciphertext1.txt mykey 57181
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
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

/**
 * @brief Reads the content of a file into a dynamically allocated buffer.
 *
 * This function opens the specified file, reads its contents into a buffer,
 * strips any trailing newline characters, and validates that all characters
 * are uppercase letters or spaces. It returns the buffer containing the file
 * content and sets the length of the content through the provided pointer.
 *
 * @param filename The name of the file to read.
 * @param length Pointer to an integer where the length of the file content will be stored.
 * @return char* Pointer to the dynamically allocated buffer containing the file content.
 *               The caller is responsible for freeing this memory.
 */
char* read_file(const char *filename, int *length) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "dec_client error: cannot open file %s\n", filename);
        exit(1);
    }

    // Determine file size
    fseek(fp, 0, SEEK_END);
    long filesize = ftell(fp);
    rewind(fp);

    // Allocate buffer (+1 for null terminator)
    char *buffer = malloc(filesize + 1);
    if (!buffer) {
        fprintf(stderr, "dec_client error: memory allocation failed\n");
        fclose(fp);
        exit(1);
    }

    // Read file
    size_t read_size = fread(buffer, sizeof(char), filesize, fp);
    if (read_size < filesize && ferror(fp)) {
        fprintf(stderr, "dec_client error: failed to read file %s\n", filename);
        free(buffer);
        fclose(fp);
        exit(1);
    }
    buffer[read_size] = '\0';

    fclose(fp);

    // Strip newline if present
    if (read_size > 0 && buffer[read_size - 1] == '\n') {
        buffer[read_size - 1] = '\0';
        read_size--;
    }

    // Validate characters: only uppercase letters and spaces allowed
    for (int i = 0; i < read_size; i++) {
        if (buffer[i] != ' ' && (buffer[i] < 'A' || buffer[i] > 'Z')) {
            fprintf(stderr, "dec_client error: input contains bad characters\n");
            free(buffer);
            exit(1);
        }
    }

    *length = read_size;
    return buffer;
}

/**
 * @brief The main function initializes the client, connects to the server,
 *        sends ciphertext and key data, receives decrypted plaintext, and
 *        outputs it.
 *
 * It performs the following tasks:
 * - Validates command-line arguments.
 * - Reads ciphertext and key files, ensuring the key is sufficiently long.
 * - Sets up a TCP socket and connects to the specified decryption server.
 * - Authenticates with the server using a predefined client identifier.
 * - Sends ciphertext and key data to the server for decryption.
 * - Receives the decrypted plaintext from the server.
 * - Outputs the plaintext to standard output.
 * - Handles errors and cleans up resources appropriately.
 *
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return int Exit status.
 */
int main(int argc, char *argv[]) {
    // Validate Command-Line Arguments
    if (argc != 4) {
        fprintf(stderr, "Usage: dec_client ciphertext key port\n");
        exit(1);
    }

    char *ciphertext_file = argv[1]; // Path to ciphertext file
    char *key_file = argv[2];        // Path to key file
    int port = atoi(argv[3]);         // Server port number

    // Validate port number
    if (port < 1024 || port > 65535) {
        fprintf(stderr, "dec_client error: invalid port number\n");
        exit(1);
    }

    // Read Ciphertext
    int ciphertext_len;
    char *ciphertext = read_file(ciphertext_file, &ciphertext_len);

    // Read Key
    int key_len;
    char *key = read_file(key_file, &key_len);

    // Check if key is long enough
    if (key_len < ciphertext_len) {
        fprintf(stderr, "Error: key '%s' is too short\n", key_file);
        free(ciphertext);
        free(key);
        exit(1);
    }

    // Set Up Socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        fprintf(stderr, "dec_client error: cannot open socket\n");
        free(ciphertext);
        free(key);
        exit(1);
    }

    // Set Up Server Address
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr)); // Zero out the structure
    server_addr.sin_family = AF_INET;              // IPv4
    server_addr.sin_port = htons(port);            // Host to network byte order

    // Convert localhost to binary form and assign to server_addr
    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
        fprintf(stderr, "dec_client error: invalid address\n");
        close(sockfd);
        free(ciphertext);
        free(key);
        exit(1);
    }

    // Connect to Server
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        fprintf(stderr, "Error: could not contact dec_server on port %d\n", port);
        close(sockfd);
        free(ciphertext);
        free(key);
        exit(2);
    }

    // Send Client Identification
    char id_msg[] = "dec_client";                   // Identifier string for authentication
    size_t id_len = strlen(id_msg);
    if (send(sockfd, id_msg, id_len, 0) != id_len) {
        fprintf(stderr, "dec_client error: failed to send ID\n");
        close(sockfd);
        free(ciphertext);
        free(key);
        exit(1);
    }

    // Wait for Server Acknowledgment with Timeout
    struct timeval tv;
    tv.tv_sec = 5;  // 5 second timeout
    tv.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv)) < 0) {
        fprintf(stderr, "dec_client error: setsockopt failed\n");
        close(sockfd);
        free(ciphertext);
        free(key);
        exit(1);
    }

    char ack_buffer[32];
    memset(ack_buffer, 0, sizeof(ack_buffer));

    // Define expected acknowledgment
    const char *expected_ack = "DEC_SERVER_ACK"; // Must match server's acknowledgment
    size_t expected_ack_len = strlen(expected_ack);
    size_t total_received = 0;

    // Receive acknowledgment in a loop until all expected bytes are received
    while (total_received < expected_ack_len) {
        ssize_t n = recv(sockfd, ack_buffer + total_received, expected_ack_len - total_received, 0);
        if (n <= 0) {
            fprintf(stderr, "Error: could not contact dec_server on port %d\n", port);
            close(sockfd);
            free(ciphertext);
            free(key);
            exit(2);
        }
        total_received += n;
    }
    ack_buffer[total_received] = '\0'; // Null-terminate the string

    // Check if server rejected the connection
    if (strncmp(ack_buffer, "INVALID_CLIENT", 13) == 0) {
        fprintf(stderr, "Error: could not contact dec_server on port %d\n", port);
        close(sockfd);
        free(ciphertext);
        free(key);
        exit(2);
    }

    // Verify proper server acknowledgment
    if (strncmp(ack_buffer, expected_ack, expected_ack_len) != 0) {
        fprintf(stderr, "dec_client error: server response not recognized\n");
        close(sockfd);
        free(ciphertext);
        free(key);
        exit(2);
    }

    // Send Ciphertext Length
    if (send(sockfd, &ciphertext_len, sizeof(int), 0) < 0) {
        fprintf(stderr, "dec_client error: failed to send ciphertext length\n");
        close(sockfd);
        free(ciphertext);
        free(key);
        exit(1);
    }

    // Send Key Length
    if (send(sockfd, &key_len, sizeof(int), 0) < 0) { // Send actual key length
        fprintf(stderr, "dec_client error: failed to send key length\n");
        close(sockfd);
        free(ciphertext);
        free(key);
        exit(1);
    }

    // Send Ciphertext Data
    size_t total_sent = 0;
    while (total_sent < ciphertext_len) {
        ssize_t n = send(sockfd, ciphertext + total_sent, ciphertext_len - total_sent, 0);
        if (n < 0) {
            fprintf(stderr, "dec_client error: failed to send ciphertext\n");
            close(sockfd);
            free(ciphertext);
            free(key);
            exit(1);
        }
        total_sent += n;
    }

    // Send Key Data (only as much as ciphertext)
    total_sent = 0;
    while (total_sent < ciphertext_len) { // Send only as much key as ciphertext
        ssize_t n = send(sockfd, key + total_sent, ciphertext_len - total_sent, 0);
        if (n < 0) {
            fprintf(stderr, "dec_client error: failed to send key\n");
            close(sockfd);
            free(ciphertext);
            free(key);
            exit(1);
        }
        total_sent += n;
    }

    // Receive Plaintext Length from Server
    int plaintext_len_received;
    if (recv(sockfd, &plaintext_len_received, sizeof(int), 0) < sizeof(int)) {
        fprintf(stderr, "dec_client error: failed to receive plaintext length\n");
        close(sockfd);
        free(ciphertext);
        free(key);
        exit(1);
    }

    // Allocate Buffer for Plaintext
    char *plaintext = malloc(plaintext_len_received + 1);
    if (!plaintext) {
        fprintf(stderr, "dec_client error: memory allocation failed\n");
        close(sockfd);
        free(ciphertext);
        free(key);
        exit(1);
    }

    // Receive Plaintext Data
    int total_received_plain = 0;
    while (total_received_plain < plaintext_len_received) {
        ssize_t n = recv(sockfd, plaintext + total_received_plain, plaintext_len_received - total_received_plain, 0);
        if (n <= 0) {
            fprintf(stderr, "dec_client error: failed to receive plaintext\n");
            close(sockfd);
            free(ciphertext);
            free(key);
            free(plaintext);
            exit(1);
        }
        total_received_plain += n;
    }
    plaintext[plaintext_len_received] = '\0'; // Null-terminate plaintext

    // Output Plaintext with Newline
    printf("%s\n", plaintext);

    // Clean Up Resources
    close(sockfd);       // Close the socket connection
    free(ciphertext);    // Free allocated ciphertext buffer
    free(key);           // Free allocated key buffer
    free(plaintext);     // Free allocated plaintext buffer

    return 0; // Exit successfully
}