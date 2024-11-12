/*
* Ethan Clinick
* CS374 HW3 FALL 2024
* 11/3/2024
*/


#define _POSIX_C_SOURCE 200809L // Enable certain features (strdup) in strdup
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <ctype.h>

// Constants defining maximum allowed input characters and arguments
#define MAX_CHARS 2048
#define MAX_ARGS 512

// Global Variables
// 'fgOnly' flag indicates whether the shell is in foreground-only mode.
// 'volatile sig_atomic_t' is used to ensure safe access in signal handlers.
volatile sig_atomic_t fgOnly = 0;

// 'lastStatus' stores the exit status of the last foreground process.
// 'lastSignal' stores the signal number that terminated the last foreground process, if any.
int lastStatus = 0;
int lastSignal = 0;

// Structure to represent a background process.
// It contains the process ID and a pointer to the next background process in the list.
typedef struct bgProcess {
    pid_t pid;                // Process ID of the background process
    struct bgProcess* next;   // Pointer to the next background process
} bgProcess;

// Head pointer for the linked list of background processes.
bgProcess* bgList = NULL;

// Structure to represent a parsed command.
// It contains arguments, input/output redirection files, and a flag for background execution.
typedef struct Command {
    char* args[MAX_ARGS + 1]; // Array of argument strings; +1 for the NULL terminator required by exec functions
    char* inputFile;           // Input redirection file, if any
    char* outputFile;          // Output redirection file, if any
    int background;            // Flag indicating if the command should run in the background
} Command;

// Function Prototypes
void handleSIGTSTP(int signo);            // Handler for SIGTSTP (Ctrl-Z) signal
void setupSignalHandlers();               // Sets up the shell's signal handlers
char* getInput();                         // Retrieves user input from the command line
int isBlankOrComment(char* input);        // Checks if the input line is blank or a comment
char* expandPID(char* input);             // Expands instances of "$$" to the shell's PID
int handleBuiltIn(Command* cmd);          // Processes built-in commands (exit, cd, status)
void handleRedirection(Command* cmd, int background); // Handles input/output redirection for commands
void executeCommand(Command* cmd);        // Executes non-built-in commands
void addBgProcess(pid_t pid);             // Adds a process to the background process list
void removeBgProcess(pid_t pid);          // Removes a process from the background process list
void checkBgProcesses();                  // Checks the status of background processes and updates the list

// Signal Handlers

/**
 * handleSIGTSTP - Custom handler for SIGTSTP (Ctrl-Z) signal.
 * Toggles the shell between normal mode and foreground-only mode.
 * When entering foreground-only mode, background execution is disabled.
 * When exiting, background execution is enabled.
 *
 * @signo: The signal number received.
 */
void handleSIGTSTP(int signo) {
    if (fgOnly == 0) {
        // Inform the user that the shell is entering foreground-only mode
        char* message = "\nEntering foreground-only mode (& is now ignored)\n: ";
        write(STDOUT_FILENO, message, strlen(message));
        fgOnly = 1; // Set flag to indicate foreground-only mode
    }
    else {
        // Inform the user that the shell is exiting foreground-only mode
        char* message = "\nExiting foreground-only mode\n: ";
        write(STDOUT_FILENO, message, strlen(message));
        fgOnly = 0; // Reset flag to normal mode
    }
}

/**
 * setupSignalHandlers - Configures the shell's signal handling behavior.
 * - Ignores SIGINT (Ctrl-C) to prevent the shell itself from being terminated.
 * - Sets a custom handler for SIGTSTP (Ctrl-Z) to toggle foreground-only mode.
 */
void setupSignalHandlers() {
    // Define structures to specify actions for SIGINT and SIGTSTP
    struct sigaction SIGINT_action = {0}, SIGTSTP_action = {0};

    // Configure SIGINT to be ignored by the shell
    SIGINT_action.sa_handler = SIG_IGN; // Ignore SIGINT
    sigfillset(&SIGINT_action.sa_mask);  // Block all signals during handler
    SIGINT_action.sa_flags = 0;          // No special flags
    sigaction(SIGINT, &SIGINT_action, NULL); // Apply the action

    // Configure SIGTSTP to use the custom handler 'handleSIGTSTP'
    SIGTSTP_action.sa_handler = handleSIGTSTP; // Custom handler
    sigfillset(&SIGTSTP_action.sa_mask);        // Block all signals during handler
    SIGTSTP_action.sa_flags = SA_RESTART;       // Restart interrupted system calls
    sigaction(SIGTSTP, &SIGTSTP_action, NULL);   // Apply the action
}

/**
 * getInput - Prompts the user and retrieves a line of input from the command line.
 *
 * Returns:
 *   A dynamically allocated string containing the user's input.
 *   Returns NULL if an error occurs or EOF is encountered.
 */
char* getInput() {
    char* input = NULL;    // Pointer to store the input string
    size_t buffer = 0;     // Size of the buffer (managed by getline)
    printf(": ");          // Display the command prompt
    fflush(stdout);       // Ensure the prompt is displayed immediately

    // Use getline to read an entire line from stdin, allocating memory as needed
    ssize_t nread = getline(&input, &buffer, stdin);
    if (nread == -1) {
        // Handle EOF or read error
        clearerr(stdin); // Clear the error indicator
        free(input);     // Free any allocated memory
        return NULL;     // Return NULL to indicate no input
    }
    return input; // Return the user's input
}

/**
 * isBlankOrComment - Determines if the input line is either blank or a comment.
 *
 * @input: The input string to check.
 *
 * Returns:
 *   1 if the line is blank or a comment, 0 otherwise.
 */
int isBlankOrComment(char* input) {
    // Trim leading whitespace characters
    while (isspace(*input)) input++;

    // Check if the trimmed input is empty or starts with '#'
    return (*input == '\0' || *input == '#' || *input == '\n');
}

/**
 * expandPID - Replaces all instances of "$$" in the input string with the shell's PID.
 *
 * @input: The original input string containing potential "$$" sequences.
 *
 * Returns:
 *   A new dynamically allocated string with "$$" expanded to the PID.
 *   The caller is responsible for freeing the returned string.
 */
char* expandPID(char* input) {
    char* ptr = input;                             // Pointer to traverse the input string
    char* result = malloc(MAX_CHARS);              // Allocate memory for the expanded string
    if (!result) {                                 // Check for successful allocation
        perror("malloc");                          // Print error message if allocation fails
        exit(1);                                   // Terminate the program
    }
    result[0] = '\0';                               // Initialize the result string as empty

    // Loop through the input string, searching for "$$"
    while ((ptr = strstr(ptr, "$$")) != NULL) {
        // Concatenate the segment before "$$" to the result
        strncat(result, input, ptr - input);

        // Convert the PID to a string and concatenate it to the result
        char pidStr[20];
        sprintf(pidStr, "%d", getpid());
        strcat(result, pidStr);

        ptr += 2; // Move past the "$$" sequence
        input = ptr; // Update the input pointer to the new position
    }

    // Concatenate any remaining part of the input string after the last "$$"
    strcat(result, input);
    return result; // Return the expanded string
}

/**
 * handleBuiltIn - Processes built-in shell commands: exit, cd, and status.
 *
 * @cmd: Pointer to the Command structure containing the parsed command.
 *
 * Returns:
 *   1 if a built-in command was handled, 0 otherwise.
 */
int handleBuiltIn(Command* cmd) {
    // Handle the 'exit' built-in command
    if (strcmp(cmd->args[0], "exit") == 0) {
        // Iterate through the background process list and terminate each process
        bgProcess* current = bgList;
        while (current != NULL) {
            kill(current->pid, SIGTERM); // Send SIGTERM to terminate the process
            current = current->next;     // Move to the next process
        }

        // Free the linked list of background processes
        while (bgList != NULL) {
            bgProcess* temp = bgList;     // Temporary pointer to current process
            bgList = bgList->next;        // Move the head to the next process
            free(temp);                   // Free the memory of the terminated process
        }

        exit(0); // Terminate the shell
    }
    // Handle the 'cd' built-in command
    else if (strcmp(cmd->args[0], "cd") == 0) {
        if (cmd->args[1] == NULL) {
            // If no argument is provided, change to the HOME directory
            if (chdir(getenv("HOME")) != 0) { // Attempt to change directory
                perror("chdir");               // Print error if chdir fails
                lastStatus = 1;                // Set lastStatus to indicate failure
            }
        }
        else {
            // Change to the specified directory (supports both absolute and relative paths)
            if (chdir(cmd->args[1]) != 0) {    // Attempt to change directory
                perror("chdir");               // Print error if chdir fails
                lastStatus = 1;                // Set lastStatus to indicate failure
            }
        }
        return 1; // Indicate that a built-in command was handled
    }
    // Handle the 'status' built-in command
    else if (strcmp(cmd->args[0], "status") == 0) {
        if (lastSignal != 0) {
            // If the last foreground process was terminated by a signal, display it
            printf("terminated by signal %d\n", lastSignal);
        }
        else {
            // Otherwise, display the exit status of the last foreground process
            printf("exit value %d\n", lastStatus);
        }
        fflush(stdout); // Ensure the output is displayed immediately
        return 1;        // Indicate that a built-in command was handled
    }
    return 0; // Indicate that no built-in command was handled
}

/**
 * handleRedirection - Sets up input and output redirection for a command.
 *
 * @cmd: Pointer to the Command structure containing the parsed command.
 * @background: Flag indicating if the command is to be executed in the background.
 *
 * This function uses dup2() to redirect stdin and stdout to the specified files.
 * If the command is a background process and no redirection is specified,
 * it redirects stdin and/or stdout to /dev/null.
 */
void handleRedirection(Command* cmd, int background) {
    // Handle Input Redirection
    if (cmd->inputFile != NULL) {
        // Open the input file for reading only
        int inputFD = open(cmd->inputFile, O_RDONLY);
        if (inputFD == -1) {
            // If opening fails, print an error and exit the child process
            fprintf(stderr, "cannot open %s for input\n", cmd->inputFile);
            exit(1);
        }
        // Duplicate the input file descriptor to stdin (file descriptor 0)
        if (dup2(inputFD, 0) == -1) {
            perror("dup2"); // Print error if dup2 fails
            exit(1);
        }
        close(inputFD); // Close the original file descriptor as it's no longer needed
    }
    else if (background) {
        // If in background and no input redirection, redirect stdin to /dev/null
        int devNull = open("/dev/null", O_RDONLY);
        if (devNull == -1) {
            perror("open /dev/null"); // Print error if opening fails
            exit(1);
        }
        if (dup2(devNull, 0) == -1) {
            perror("dup2"); // Print error if dup2 fails
            exit(1);
        }
        close(devNull); // Close the original file descriptor
    }

    // Handle Output Redirection
    if (cmd->outputFile != NULL) {
        // Open the output file for writing only, create it if it doesn't exist, truncate if it does
        int outputFD = open(cmd->outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (outputFD == -1) {
            // If opening fails, print an error and exit the child process
            fprintf(stderr, "cannot open %s for output\n", cmd->outputFile);
            exit(1);
        }
        // Duplicate the output file descriptor to stdout (file descriptor 1)
        if (dup2(outputFD, 1) == -1) {
            perror("dup2"); // Print error if dup2 fails
            exit(1);
        }
        close(outputFD); // Close the original file descriptor
    }
    else if (background) {
        // If in background and no output redirection, redirect stdout to /dev/null
        int devNull = open("/dev/null", O_WRONLY);
        if (devNull == -1) {
            perror("open /dev/null"); // Print error if opening fails
            exit(1);
        }
        if (dup2(devNull, 1) == -1) {
            perror("dup2"); // Print error if dup2 fails
            exit(1);
        }
        close(devNull); // Close the original file descriptor
    }
}

/**
 * addBgProcess - Adds a new background process to the background process list.
 *
 * @pid: The process ID of the background process to add.
 */
void addBgProcess(pid_t pid) {
    // Allocate memory for a new bgProcess node
    bgProcess* newNode = malloc(sizeof(bgProcess));
    newNode->pid = pid;       // Set the PID
    newNode->next = bgList;   // Insert at the beginning of the list
    bgList = newNode;         // Update the head of the list
}

/**
 * removeBgProcess - Removes a background process from the background process list.
 *
 * @pid: The process ID of the background process to remove.
 *
 * This function traverses the linked list to find and remove the specified process.
 */
void removeBgProcess(pid_t pid) {
    bgProcess** current = &bgList; // Pointer to the current node's pointer
    while (*current) {              // Traverse the list
        if ((*current)->pid == pid) { // Check if the current node matches the PID
            bgProcess* temp = *current; // Temporary pointer to the current node
            *current = (*current)->next; // Remove the node from the list
            free(temp);                   // Free the memory of the removed node
            return;                       // Exit after removal
        }
        current = &((*current)->next); // Move to the next node
    }
}

/**
 * checkBgProcesses - Checks the status of all background processes.
 *
 * This function iterates through the background process list and uses waitpid()
 * with the WNOHANG option to check if any background processes have completed.
 * If a process has completed, it prints a message and removes the process from the list.
 */
void checkBgProcesses() {
    bgProcess* current = bgList; // Start from the head of the list
    bgProcess* prev = NULL;      // Pointer to the previous node

    while (current != NULL) {
        // Check if the background process has terminated without blocking
        pid_t result = waitpid(current->pid, NULL, WNOHANG);
        if (result > 0) {
            // If the process has terminated, inform the user
            printf("background pid %d is done: exit value 0\n", current->pid);
            fflush(stdout); // Ensure the message is displayed immediately

            // Remove the process from the list
            if (prev == NULL) {
                // If it's the first node, update the head
                bgList = current->next;
                free(current); // Free the memory of the terminated process
                current = bgList; // Move to the next process
            }
            else {
                // If it's not the first node, bypass the terminated node
                prev->next = current->next;
                free(current);      // Free the memory of the terminated process
                current = prev->next; // Move to the next process
            }
        }
        else {
            // If the process is still running, move to the next one
            prev = current;
            current = current->next;
        }
    }
}

/**
 * executeCommand - Executes non-built-in commands by forking a child process.
 *
 * @cmd: Pointer to the Command structure containing the parsed command.
 *
 * This function handles the creation of child processes, setting up signal
 * handling in children, performing I/O redirection, and executing the command.
 * It also manages foreground and background execution behavior.
 */
void executeCommand(Command* cmd) {
    pid_t spawnPid = fork(); // Create a new child process
    int childStatus;         // Variable to store the child's exit status

    switch(spawnPid) {
        case -1:
            // If fork() fails, print an error and terminate
            perror("fork");
            exit(1);
            break;
        case 0:
            // In the child process

            // Handle Signals in the child process
            if (cmd->background) {
                // For background processes, ignore SIGINT (Ctrl-C)
                struct sigaction SIGINT_child = {0};
                SIGINT_child.sa_handler = SIG_IGN; // Ignore SIGINT
                sigfillset(&SIGINT_child.sa_mask);  // Block all signals during handler
                SIGINT_child.sa_flags = 0;          // No special flags
                sigaction(SIGINT, &SIGINT_child, NULL); // Apply the action
            }
            else {
                // For foreground processes, use the default SIGINT behavior
                struct sigaction SIGINT_child = {0};
                SIGINT_child.sa_handler = SIG_DFL;  // Default handler
                sigfillset(&SIGINT_child.sa_mask);   // Block all signals during handler
                SIGINT_child.sa_flags = 0;           // No special flags
                sigaction(SIGINT, &SIGINT_child, NULL); // Apply the action
            }

            // Ignore SIGTSTP (Ctrl-Z) in child processes
            struct sigaction SIGTSTP_child = {0};
            SIGTSTP_child.sa_handler = SIG_IGN; // Ignore SIGTSTP
            sigfillset(&SIGTSTP_child.sa_mask);  // Block all signals during handler
            SIGTSTP_child.sa_flags = 0;          // No special flags
            sigaction(SIGTSTP, &SIGTSTP_child, NULL); // Apply the action

            // Handle input and output redirection
            handleRedirection(cmd, cmd->background);

            // Execute the command using execvp, which searches the PATH environment variable
            if (execvp(cmd->args[0], cmd->args) == -1) {
                // If execvp fails, print an error and terminate the child process
                perror(cmd->args[0]);
                exit(1);
            }
            break;
        default:
            // In the parent process
            if (!cmd->background) {
                // If the command is to be run in the foreground, wait for it to complete
                pid_t childPid = waitpid(spawnPid, &childStatus, 0); // Wait for the specific child
                if (WIFEXITED(childStatus)) {
                    // If the child exited normally, store the exit status
                    lastStatus = WEXITSTATUS(childStatus);
                    lastSignal = 0; // No signal termination
                }
                else if (WIFSIGNALED(childStatus)) {
                    // If the child was terminated by a signal, store the signal number
                    lastSignal = WTERMSIG(childStatus);
                    printf("terminated by signal %d\n", lastSignal); // Inform the user
                    fflush(stdout); // Ensure the message is displayed immediately
                }
            }
            else {
                // If the command is to be run in the background, do not wait
                printf("background pid is %d\n", spawnPid); // Inform the user of the background PID
                fflush(stdout); // Ensure the message is displayed immediately
                addBgProcess(spawnPid); // Add the background process to the list
            }
            break;
    }
}

int main() {
    // Initialize the shell by setting up signal handlers
    setupSignalHandlers();

    while (1) { // Main loop to continuously prompt for and execute commands
        // Check if any background processes have completed
        checkBgProcesses();

        // Retrieve user input
        char* input = getInput();
        if (input == NULL) continue; // If no input, prompt again

        // Handle blank lines and comments by ignoring them
        if (isBlankOrComment(input)) {
            free(input); // Free the memory allocated by getline
            continue;    // Skip to the next iteration of the loop
        }

        // Expand instances of "$$" to the shell's PID
        char* expandedInput = expandPID(input);
        free(input); // Free the original input string as it's no longer needed

        // Parse the expanded input into the Command structure
        Command cmd = {0}; // Initialize the Command structure to zero
        char* saveptr;      // Pointer for strtok_r's state
        char* token = strtok_r(expandedInput, " \n", &saveptr); // Tokenize the input by spaces and newlines
        int argCount = 0;   // Counter for the number of arguments

        while (token != NULL && argCount < MAX_ARGS) { // Loop through each token
            if (strcmp(token, "<") == 0) { // Input redirection detected
                token = strtok_r(NULL, " \n", &saveptr); // Get the filename for input redirection
                if (token != NULL) {
                    cmd.inputFile = strdup(token); // Duplicate the token and store as inputFile
                }
            }
            else if (strcmp(token, ">") == 0) { // Output redirection detected
                token = strtok_r(NULL, " \n", &saveptr); // Get the filename for output redirection
                if (token != NULL) {
                    cmd.outputFile = strdup(token); // Duplicate the token and store as outputFile
                }
            }
            else {
                // Regular argument; store it in the args array
                cmd.args[argCount++] = strdup(token); // Duplicate the token and add to args
            }
            token = strtok_r(NULL, " \n", &saveptr); // Get the next token
        }

        // Check if the last argument is "&" indicating background execution
        if (argCount > 0 && strcmp(cmd.args[argCount - 1], "&") == 0) {
            cmd.background = 1;             // Set the background flag
            free(cmd.args[argCount - 1]);  // Free the memory allocated for "&"
            cmd.args[argCount - 1] = NULL;  // Remove "&" from the args array
            argCount--;                      // Decrement the argument count
        }

        // If the shell is in foreground-only mode, ignore the background flag
        if (fgOnly) {
            cmd.background = 0;
        }

        cmd.args[argCount] = NULL; // Null-terminate the args array as required by exec functions

        // Handle built-in commands; if handled, skip executing as external command
        if (handleBuiltIn(&cmd)) {
            // Built-in command was handled; no further action needed
        }
        else {
            // Execute non-built-in command
            executeCommand(&cmd);
        }

        // Free dynamically allocated memory for arguments and redirection files
        
        for (int i = 0; i < argCount; i++) {
            free(cmd.args[i]); // Free each argument string
        }
        if (cmd.inputFile) free(cmd.inputFile);   // Free inputFile if it was set
        if (cmd.outputFile) free(cmd.outputFile); // Free outputFile if it was set
        free(expandedInput); // Free the expanded input string
    }

    return 0; // This line is technically unreachable due to the infinite loop
}
