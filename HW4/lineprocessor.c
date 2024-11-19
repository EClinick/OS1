/******************************************************************************
 * 
 * File: line_processor.c
 * Author: Ethan Clinick
 * Date: 11/19/2024
 * 
 * Description:
 * ------------
 * This program implements a multi-threaded line processor using the Producer-
 * Consumer pattern. It creates a pipeline of four threads to process input
 * from standard input and produce formatted output to standard output.
 * 
 * The pipeline consists of the following threads:
 * 
 * 1. **Input Thread:** Reads lines from standard input and places them into Buffer 1.
 * 2. **Line Separator Thread:** Replaces line separators (`\n`) with spaces and
 *    places the modified lines into Buffer 2.
 * 3. **Plus Sign Thread:** Replaces every pair of plus signs (`++`) with a caret (`^`)
 *    and places the processed lines into Buffer 3.
 * 4. **Output Thread:** Aggregates the processed data and writes lines of exactly
 *    80 characters to standard output.
 * 
 * Each pair of communicating threads uses a shared buffer with mutual exclusion
 * and condition variables to ensure synchronized access and data integrity.
 * 
 * The program processes input until it encounters a line containing only "STOP",
 * after which it terminates gracefully, ensuring all complete 80-character lines
 * are outputted.
 * 
 * Compilation:
 * ------------
 * To compile the program, use the following command:
 * 
 *     gcc -pthread -o line_processor line_processor.c
 * 
 * Usage:
 * ------
 * Run the program and provide input via keyboard or by redirecting input from a file.
 * 
 * Examples:
 * 
 * 1. **Keyboard Input:**
 *      ./line_processor
 * 
 * 2. **File Redirection:**
 *      ./lineprocessor < ./tests/input1-1.txt > ./tests/generated-output/output1.txt
 * 3. **Running test_output.sh:**
 *    ./test_output.sh
 * Notes:
 * ------
 * - The program does not produce any output other than the processed 80-character
 *   lines.
 * - Incomplete lines (less than 80 characters) are discarded upon termination.
 * - The program uses dynamic memory allocation for processing lines; ensure sufficient
 *   memory is available.
 * 
 *****************************************************************************/

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Constants Definitions */
#define BUFFER_SIZE 50               // Maximum number of lines each buffer can hold
#define MAX_LINE_LENGTH 1000         // Maximum length of an input line (including '\n')
#define OUTPUT_LINE_LENGTH 80        // Fixed length of each output line

/**
 * @brief Structure representing a thread-safe circular buffer for producer-consumer communication.
 */
typedef struct {
    char *lines[BUFFER_SIZE];       // Array of pointers to hold lines of text
    int count;                      // Current number of lines in the buffer
    int in;                         // Index for the next insertion
    int out;                        // Index for the next removal
    pthread_mutex_t mutex;          // Mutex to protect buffer access
    pthread_cond_t not_empty;       // Condition variable signaled when buffer is not empty
    pthread_cond_t not_full;        // Condition variable signaled when buffer is not full
} Buffer;

/**
 * @brief Initializes a Buffer structure by setting initial indices, count, and initializing
 *        synchronization primitives.
 * 
 * @param buffer Pointer to the Buffer structure to initialize.
 */
void buffer_init(Buffer *buffer) {
    buffer->count = 0;               // Initialize line count to 0
    buffer->in = 0;                  // Initialize insertion index
    buffer->out = 0;                 // Initialize removal index

    /* Initialize mutex for mutual exclusion */
    if (pthread_mutex_init(&buffer->mutex, NULL) != 0) {
        perror("Mutex initialization failed");
        exit(EXIT_FAILURE);
    }

    /* Initialize condition variable for buffer not empty */
    if (pthread_cond_init(&buffer->not_empty, NULL) != 0) {
        perror("Condition variable (not_empty) initialization failed");
        exit(EXIT_FAILURE);
    }

    /* Initialize condition variable for buffer not full */
    if (pthread_cond_init(&buffer->not_full, NULL) != 0) {
        perror("Condition variable (not_full) initialization failed");
        exit(EXIT_FAILURE);
    }
}

/**
 * @brief Destroys a Buffer structure by destroying its mutex and condition variables.
 * 
 * @param buffer Pointer to the Buffer structure to destroy.
 */
void buffer_destroy(Buffer *buffer) {
    pthread_mutex_destroy(&buffer->mutex);           // Destroy the mutex
    pthread_cond_destroy(&buffer->not_empty);        // Destroy the not_empty condition variable
    pthread_cond_destroy(&buffer->not_full);         // Destroy the not_full condition variable
}

/**
 * @brief Adds a line to the buffer in a thread-safe manner. If the buffer is full, the
 *        calling thread waits until space becomes available.
 * 
 * @param buffer Pointer to the Buffer structure.
 * @param line   Pointer to the line to add to the buffer.
 */
void buffer_add(Buffer *buffer, char *line) {
    /* Acquire the mutex before modifying the buffer */
    pthread_mutex_lock(&buffer->mutex);

    /* Wait while the buffer is full */
    while (buffer->count == BUFFER_SIZE) {
        pthread_cond_wait(&buffer->not_full, &buffer->mutex);
    }

    /* Add the line to the buffer at the 'in' index */
    buffer->lines[buffer->in] = line;
    buffer->in = (buffer->in + 1) % BUFFER_SIZE;  // Move 'in' index forward in a circular manner
    buffer->count++;                               // Increment the line count

    /* Signal that the buffer is not empty, in case consumers are waiting */
    pthread_cond_signal(&buffer->not_empty);

    /* Release the mutex */
    pthread_mutex_unlock(&buffer->mutex);
}

/**
 * @brief Removes and returns a line from the buffer in a thread-safe manner. If the buffer
 *        is empty, the calling thread waits until data becomes available.
 * 
 * @param buffer Pointer to the Buffer structure.
 * @return char* Pointer to the removed line.
 */
char* buffer_remove(Buffer *buffer) {
    /* Acquire the mutex before accessing the buffer */
    pthread_mutex_lock(&buffer->mutex);

    /* Wait while the buffer is empty */
    while (buffer->count == 0) {
        pthread_cond_wait(&buffer->not_empty, &buffer->mutex);
    }

    /* Remove the line from the buffer at the 'out' index */
    char *line = buffer->lines[buffer->out];
    buffer->out = (buffer->out + 1) % BUFFER_SIZE; // Move 'out' index forward in a circular manner
    buffer->count--;                                 // Decrement the line count

    /* Signal that the buffer is not full, in case producers are waiting */
    pthread_cond_signal(&buffer->not_full);

    /* Release the mutex */
    pthread_mutex_unlock(&buffer->mutex);

    return line; // Return the removed line
}

/* Argument Structures for Threads */

/**
 * @brief Structure to hold arguments for the Input Thread.
 */
typedef struct {
    Buffer *buffer1;                // Pointer to Buffer 1 (Input -> Line Separator)
} InputArgs;

/**
 * @brief Structure to hold arguments for the Line Separator Thread.
 */
typedef struct {
    Buffer *buffer1;                // Pointer to Buffer 1 (Input -> Line Separator)
    Buffer *buffer2;                // Pointer to Buffer 2 (Line Separator -> Plus Sign)
} LineSeparatorArgs;

/**
 * @brief Structure to hold arguments for the Plus Sign Thread.
 */
typedef struct {
    Buffer *buffer2;                // Pointer to Buffer 2 (Line Separator -> Plus Sign)
    Buffer *buffer3;                // Pointer to Buffer 3 (Plus Sign -> Output)
} PlusSignArgs;

/**
 * @brief Structure to hold arguments for the Output Thread.
 */
typedef struct {
    Buffer *buffer3;                // Pointer to Buffer 3 (Plus Sign -> Output)
} OutputArgs;

/**
 * @brief Function executed by the Input Thread.
 * 
 * This thread reads lines from standard input and places them into Buffer 1.
 * It continues reading until it encounters a line containing only "STOP\n",
 * after which it sends a sentinel (`NULL`) to indicate termination and exits.
 * 
 * @param args Pointer to InputArgs structure containing buffer information.
 * @return void* Always returns NULL.
 */
void* input_thread(void *args) {
    InputArgs *inputArgs = (InputArgs*) args;  // Cast the argument to InputArgs pointer
    Buffer *buffer1 = inputArgs->buffer1;      // Retrieve Buffer 1
    char line[MAX_LINE_LENGTH + 2];            // Buffer to hold input line (+2 for '\n' and '\0')

    /* Continuously read lines from standard input */
    while (fgets(line, sizeof(line), stdin)) {
        /* Check if the line is the stop-processing line "STOP\n" */
        if (strcmp(line, "STOP\n") == 0) {
            /* Add NULL sentinel to Buffer 1 to signal termination */
            buffer_add(buffer1, NULL);
            break; // Exit the loop and terminate the thread
        }

        /* Duplicate the line to allocate memory dynamically */
        char *dup_line = strdup(line);
        if (dup_line == NULL) {
            perror("strdup failed");
            exit(EXIT_FAILURE); // Exit if memory allocation fails
        }

        /* Add the duplicated line to Buffer 1 */
        buffer_add(buffer1, dup_line);
    }

    /* In case of EOF without encountering "STOP\n", still send the sentinel */
    buffer_add(buffer1, NULL);

    return NULL; // Thread exits
}

/**
 * @brief Function executed by the Line Separator Thread.
 * 
 * This thread removes lines from Buffer 1, replaces the line separator (`\n`)
 * with a space (` `), and places the modified lines into Buffer 2.
 * It continues processing until it receives a sentinel (`NULL`), after which
 * it forwards the sentinel to Buffer 2 and exits.
 * 
 * @param args Pointer to LineSeparatorArgs structure containing buffer information.
 * @return void* Always returns NULL.
 */
void* line_separator_thread(void *args) {
    LineSeparatorArgs *lsArgs = (LineSeparatorArgs*) args; // Cast the argument to LineSeparatorArgs pointer
    Buffer *buffer1 = lsArgs->buffer1;                     // Retrieve Buffer 1
    Buffer *buffer2 = lsArgs->buffer2;                     // Retrieve Buffer 2
    char *line;                                             // Pointer to hold the removed line

    /* Continuously remove lines from Buffer 1 */
    while ((line = buffer_remove(buffer1)) != NULL) {
        size_t len = strlen(line); // Get the length of the line

        /* Replace the last character '\n' with a space ' ' if present */
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = ' ';
        }

        /* Add the modified line to Buffer 2 */
        buffer_add(buffer2, line);
    }

    /* After receiving the sentinel, pass it to Buffer 2 to signal termination */
    buffer_add(buffer2, NULL);

    return NULL; // Thread exits
}

/**
 * @brief Function executed by the Plus Sign Thread.
 * 
 * This thread removes lines from Buffer 2, replaces every occurrence of "++"
 * with a caret (`^`), and places the processed lines into Buffer 3.
 * It continues processing until it receives a sentinel (`NULL`), after which
 * it forwards the sentinel to Buffer 3 and exits.
 * 
 * @param args Pointer to PlusSignArgs structure containing buffer information.
 * @return void* Always returns NULL.
 */
void* plus_sign_thread(void *args) {
    PlusSignArgs *psArgs = (PlusSignArgs*) args; // Cast the argument to PlusSignArgs pointer
    Buffer *buffer2 = psArgs->buffer2;          // Retrieve Buffer 2
    Buffer *buffer3 = psArgs->buffer3;          // Retrieve Buffer 3
    char *line;                                  // Pointer to hold the removed line

    /* Continuously remove lines from Buffer 2 */
    while ((line = buffer_remove(buffer2)) != NULL) {
        /* Allocate memory for the processed line */
        char *processed = malloc(MAX_LINE_LENGTH + 1); // +1 for null terminator
        if (processed == NULL) {
            perror("malloc failed");
            exit(EXIT_FAILURE); // Exit if memory allocation fails
        }

        int i = 0; // Index for reading the original line
        int j = 0; // Index for writing to the processed line

        /* Iterate through the original line to replace "++" with "^" */
        while (line[i] != '\0') {
            if (line[i] == '+' && line[i + 1] == '+') {
                /* Replace "++" with "^" */
                processed[j++] = '^';
                i += 2; // Skip the next '+' as it's part of the pair
            } else {
                /* Copy the current character as is */
                processed[j++] = line[i++];
            }

            /* Prevent buffer overflow by ensuring we don't exceed MAX_LINE_LENGTH */
            if (j >= MAX_LINE_LENGTH) {
                break; // Exit the loop if maximum length is reached
            }
        }

        processed[j] = '\0'; // Null-terminate the processed string

        free(line); // Free the original line as it's no longer needed

        /* Add the processed line to Buffer 3 */
        buffer_add(buffer3, processed);
    }

    /* After receiving the sentinel, pass it to Buffer 3 to signal termination */
    buffer_add(buffer3, NULL);

    return NULL; // Thread exits
}

/**
 * @brief Function executed by the Output Thread.
 * 
 * This thread removes processed lines from Buffer 3, accumulates characters
 * until it forms an 80-character line, and writes the line to standard output.
 * It continues processing until it receives a sentinel (`NULL`), after which
 * it checks for any remaining complete lines and discards incomplete lines before
 * exiting.
 * 
 * @param args Pointer to OutputArgs structure containing buffer information.
 * @return void* Always returns NULL.
 */
void* output_thread(void *args) {
    OutputArgs *outArgs = (OutputArgs*) args; // Cast the argument to OutputArgs pointer
    Buffer *buffer3 = outArgs->buffer3;      // Retrieve Buffer 3
    char *line;                               // Pointer to hold the removed line
    char output_buffer[OUTPUT_LINE_LENGTH + 1]; // Buffer to accumulate output characters (+1 for '\0')
    int buf_index = 0;                        // Current index in the output buffer

    /* Continuously remove lines from Buffer 3 */
    while ((line = buffer_remove(buffer3)) != NULL) {
        int i = 0; // Index for reading the processed line

        /* Iterate through each character in the processed line */
        while (line[i] != '\0') {
            output_buffer[buf_index++] = line[i++]; // Add character to output buffer

            /* Check if the output buffer has reached 80 characters */
            if (buf_index == OUTPUT_LINE_LENGTH) {
                output_buffer[OUTPUT_LINE_LENGTH] = '\0'; // Null-terminate the output line
                printf("%s\n", output_buffer);              // Write the 80-character line to stdout
                buf_index = 0;                              // Reset buffer index for the next line
            }

            /* Safety check to prevent buffer overflow */
            if (buf_index > OUTPUT_LINE_LENGTH) {
                fprintf(stderr, "Output buffer overflow\n");
                exit(EXIT_FAILURE); // Exit if buffer overflow occurs
            }
        }

        free(line); // Free the processed line as it's no longer needed
    }

    /* After receiving the sentinel, discard any remaining characters that do not form a complete line */
    /* As per instructions, incomplete lines are not written to output */

    return NULL; // Thread exits
}

/**
 * @brief The main function initializes buffers, creates threads, and waits for their
 *        completion. It ensures proper cleanup of resources before terminating.
 * 
 * @return int Exit status of the program.
 */
int main() {
    /* Initialize three buffers for inter-thread communication */
    Buffer buffer1, buffer2, buffer3;
    buffer_init(&buffer1); // Buffer between Input Thread and Line Separator Thread
    buffer_init(&buffer2); // Buffer between Line Separator Thread and Plus Sign Thread
    buffer_init(&buffer3); // Buffer between Plus Sign Thread and Output Thread

    /* Set up thread argument structures */
    InputArgs inputArgs = { .buffer1 = &buffer1 };
    LineSeparatorArgs lsArgs = { .buffer1 = &buffer1, .buffer2 = &buffer2 };
    PlusSignArgs psArgs = { .buffer2 = &buffer2, .buffer3 = &buffer3 };
    OutputArgs outArgs = { .buffer3 = &buffer3 };

    /* Declare thread identifiers */
    pthread_t input_tid, ls_tid, ps_tid, out_tid;

    /* Create the Input Thread */
    if (pthread_create(&input_tid, NULL, input_thread, &inputArgs) != 0) {
        perror("Failed to create Input Thread");
        exit(EXIT_FAILURE);
    }

    /* Create the Line Separator Thread */
    if (pthread_create(&ls_tid, NULL, line_separator_thread, &lsArgs) != 0) {
        perror("Failed to create Line Separator Thread");
        exit(EXIT_FAILURE);
    }

    /* Create the Plus Sign Thread */
    if (pthread_create(&ps_tid, NULL, plus_sign_thread, &psArgs) != 0) {
        perror("Failed to create Plus Sign Thread");
        exit(EXIT_FAILURE);
    }

    /* Create the Output Thread */
    if (pthread_create(&out_tid, NULL, output_thread, &outArgs) != 0) {
        perror("Failed to create Output Thread");
        exit(EXIT_FAILURE);
    }

    /* Wait for all threads to finish execution */
    if (pthread_join(input_tid, NULL) != 0) {
        perror("Failed to join Input Thread");
        exit(EXIT_FAILURE);
    }
    if (pthread_join(ls_tid, NULL) != 0) {
        perror("Failed to join Line Separator Thread");
        exit(EXIT_FAILURE);
    }
    if (pthread_join(ps_tid, NULL) != 0) {
        perror("Failed to join Plus Sign Thread");
        exit(EXIT_FAILURE);
    }
    if (pthread_join(out_tid, NULL) != 0) {
        perror("Failed to join Output Thread");
        exit(EXIT_FAILURE);
    }

    /* Destroy the buffers to free resources */
    buffer_destroy(&buffer1);
    buffer_destroy(&buffer2);
    buffer_destroy(&buffer3);

    return 0; // Program exits successfully
}
