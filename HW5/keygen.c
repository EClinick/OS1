/**
 * keygen.c
 *
 * Description:
 * ------------
 * This program generates a random key for use in a one-time pad encryption/decryption system.
 * The key consists of uppercase letters and spaces, matching the character set used by the
 * encryption and decryption clients. The program ensures that the generated key is of the
 * specified length and outputs it to standard output followed by a newline.
 *
 * Features:
 * ---------
 * - Generates random keys composed of uppercase letters and spaces.
 * - Ensures reproducibility by seeding the random number generator with the current time
 *   and the process ID.
 * - Validates input to ensure that the key length is a positive integer.
 * - Handles potential output errors gracefully with informative messages.
 *
 * Compilation:
 * ------------
 * To compile the program, use the following command:
 *
 *     gcc -std=gnu99 -Wall -Wextra -o keygen keygen.c
 *
 * Usage:
 * ------
 * Run the key generator by specifying the desired key length:
 *
 *     ./keygen <keylength>
 *
 * Example:
 *
 *     ./keygen 100
 *
 * This command generates a random key of 100 characters and outputs it to standard output.
 *
 * Author:
 * -------
 * Ethan Clinick
 * CS374 HW5 FALL 2024
 * 12/2/2024
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h> // Included for getpid()

#define CHAR_SET_SIZE 27 // 26 letters + space

/**
 * @brief Generates a random character from the defined character set.
 *
 * This function selects a random integer between 0 and 26 inclusive.
 * If the integer is 26, it returns a space character. Otherwise, it returns
 * the corresponding uppercase letter ('A' to 'Z').
 *
 * @return char A randomly selected character (uppercase letter or space).
 */
char get_random_char() {
    int r = rand() % CHAR_SET_SIZE; // Generate a random number between 0 and 26

    if (r == 26)
        return ' '; // Return space if r is 26
    else
        return 'A' + r; // Return corresponding uppercase letter
}

/**
 * @brief The main function generates a random key of specified length.
 *
 * It performs the following operations:
 * - Validates the number of command-line arguments.
 * - Converts the key length argument to an integer and validates it.
 * - Seeds the random number generator using the current time and process ID.
 * - Generates and outputs the random key character by character.
 * - Appends a newline character at the end of the key.
 *
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return int Exit status.
 */
int main(int argc, char *argv[]) {
    // Validate the number of command-line arguments
    if (argc != 2) {
        fprintf(stderr, "Usage: keygen keylength\n");
        exit(1);
    }

    // Convert the key length argument to an integer
    int keylength = atoi(argv[1]);

    // Validate that the key length is a positive integer
    if (keylength < 1) {
        fprintf(stderr, "Error: keylength must be a positive integer\n");
        exit(1);
    }

    // Seed the random number generator with current time XORed with process ID for uniqueness
    srand(time(NULL) ^ getpid());

    // Generate and output the random key characters
    for (int i = 0; i < keylength; i++) {
        char c = get_random_char(); // Generate a random character

        // Output the generated character and handle potential output errors
        if (putchar(c) == EOF) {
            fprintf(stderr, "Error: failed to write key character\n");
            exit(1);
        }
    }

    // Append a newline character at the end of the key
    if (putchar('\n') == EOF) {
        fprintf(stderr, "Error: failed to write newline\n");
        exit(1);
    }

    return 0; // Exit successfully
}