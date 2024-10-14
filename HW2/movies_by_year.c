#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>

/* Function Prototypes */
/*
 * Displays the main menu for the user to choose options.
 * No parameters are required.
 * No return value.
 */
void display_main_menu();

/*
 * Allows the user to select a file to process (largest, smallest, or specific file).
 * No parameters are required.
 * No return value.
 */
void select_file_to_process();

/*
 * Finds and processes the largest file with a specific prefix and extension.
 * No parameters are required.
 * No return value.
 */
void process_largest_file();

/*
 * Finds and processes the smallest file with a specific prefix and extension.
 * No parameters are required.
 * No return value.
 */
void process_smallest_file();

/*
 * Prompts the user to enter the name of a specific file to process.
 * No parameters are required.
 * No return value.
 */
void process_specific_file();

/*
 * Processes the specified file by creating a directory and parsing its contents.
 * Parameters:
 *   filename (const char *): The name of the file to process.
 * No return value.
 */
void process_file(const char *filename);

/*
 * Creates a directory with the specified name and permissions.
 * Parameters:
 *   dir_name (char *): The name of the directory to create.
 * No return value.
 */
void create_directory(char *dir_name);

/*
 * Parses the specified CSV file and creates year-based files in the specified directory.
 * Parameters:
 *   directory (const char *): The directory where year-based files will be created.
 *   filename (const char *): The CSV file to parse.
 * No return value.
 */
void create_year_files(const char *directory, const char *filename);

/* Main Function */
/*
 * Entry point of the program. Displays the main menu and handles user input.
 * No parameters are required.
 * Returns 0 upon successful execution.
 */
int main() {
    int choice;
    while (1) {
        display_main_menu();
        scanf("%d", &choice);
        switch (choice) {
            case 1:
                select_file_to_process();
                break;
            case 2:
                printf("Exiting the program.\n");
                exit(0);
            default:
                printf("Invalid choice. Please enter 1 or 2.\n");
        }
    }
    return 0;
}

/* Function Definitions */
/*
 * Displays the main menu for selecting options.
 * No parameters are required.
 * No return value.
 */
void display_main_menu() {
    printf("\n1. Select file to process\n");
    printf("2. Exit the program\n");
    printf("Enter a choice 1 or 2: ");
}

/*
 * Allows the user to select which file to process (largest, smallest, or specific file).
 * No parameters are required.
 * No return value.
 */
void select_file_to_process() {
    int choice;
    printf("\nWhich file you want to process?\n");
    printf("Enter 1 to pick the largest file\n");
    printf("Enter 2 to pick the smallest file\n");
    printf("Enter 3 to specify the name of a file\n");
    printf("Enter a choice from 1 to 3: ");
    scanf("%d", &choice);

    switch (choice) {
        case 1:
            process_largest_file();
            break;
        case 2:
            process_smallest_file();
            break;
        case 3:
            process_specific_file();
            break;
        default:
            printf("Invalid choice. Please enter a number from 1 to 3.\n");
            select_file_to_process();
    }
}

/*
 * Finds the largest file in the current directory with the prefix "movies_" and extension ".csv".
 * Calls process_file(filename) once the file is found.
 * No parameters are required.
 * No return value.
 */
void process_largest_file() {
    DIR* curr_dir = opendir(".");
    struct dirent* aDir;
    struct stat dirStat;
    off_t largestFileSize = 0;
    char largestFile[256];
    int i = 0;

    // Iterate through all files in the directory
    while ((aDir = readdir(curr_dir)) != NULL) {
        // Check if the file matches the prefix "movies_" and extension ".csv"
        if (strncmp(aDir->d_name, "movies_", 7) == 0 && strstr(aDir->d_name, ".csv") != NULL) {
            // Get file statistics to determine the size
            stat(aDir->d_name, &dirStat);
            // Update largest file if this file is larger than the previously found largest file
            if (i == 0 || dirStat.st_size > largestFileSize) {
                largestFileSize = dirStat.st_size;
                strcpy(largestFile, aDir->d_name);  // Copy the filename to largestFile
            }
            i++;
        }
    }

    closedir(curr_dir);

    // If no matching file was found, prompt the user again
    if (i == 0) {
        printf("No file found with the prefix 'movies_' and extension '.csv'\n");
        select_file_to_process();
    } else {
        // Process the largest file found
        process_file(largestFile);
    }
}

/*
 * Finds the smallest file in the current directory with the prefix "movies_" and extension ".csv".
 * Calls process_file(filename) once the file is found.
 * No parameters are required.
 * No return value.
 */
void process_smallest_file() {
    DIR* curr_dir = opendir(".");
    struct dirent* aDir;
    struct stat dirStat;
    off_t smallestFileSize = 0;
    char smallestFile[256];
    int i = 0;

    // Iterate through all files in the directory
    while ((aDir = readdir(curr_dir)) != NULL) {
        // Check if the file matches the prefix "movies_" and extension ".csv"
        if (strncmp(aDir->d_name, "movies_", 7) == 0 && strstr(aDir->d_name, ".csv") != NULL) {
            // Get file statistics to determine the size
            stat(aDir->d_name, &dirStat);
            // Update smallest file if this file is smaller than the previously found smallest file
            if (i == 0 || dirStat.st_size < smallestFileSize) {
                smallestFileSize = dirStat.st_size;
                strcpy(smallestFile, aDir->d_name);  // Copy the filename to smallestFile
            }
            i++;
        }
    }

    closedir(curr_dir);

    // If no matching file was found, prompt the user again
    if (i == 0) {
        printf("No file found with the prefix 'movies_' and extension '.csv'\n");
        select_file_to_process();
    } else {
        // Process the smallest file found
        process_file(smallestFile);
    }
}

/*
 * Prompts the user to enter the name of a specific file to process.
 * Verifies if the file exists, and if it does, processes it.
 * No parameters are required.
 * No return value.
 */
void process_specific_file() {
    char filename[256];
    printf("Enter the complete file name: ");
    scanf("%s", filename);
    
    // Check if the file exists in the current directory
    if (access(filename, F_OK) == 0) {
        // Process the specified file
        process_file(filename);
    } else {
        // If the file does not exist, prompt the user again
        printf("The file %s was not found. Try again.\n", filename);
        select_file_to_process();
    }
}

/*
 * Processes the specified file by creating a directory and parsing the file contents to create year-based files.
 * Parameters:
 *   filename (const char *): The name of the file to process.
 * No return value.
 */
void process_file(const char *filename) {
    printf("Now processing the chosen file named %s\n", filename);
    
    // Generate a random number to create a unique directory name
    char dir_name[256];
    srand(time(NULL));  // Seed the random number generator with the current time
    int random_number = rand() % 100000;  // Generate a random number between 0 and 99999
    sprintf(dir_name, "clinicke.movies.%d", random_number);  // Create the directory name
    
    // Create directory to store year-based files
    create_directory(dir_name);
    
    // Parse data from the file and create year-based files
    create_year_files(dir_name, filename);
}

/*
 * Creates a directory with the specified name and sets permissions to rwxr-x---.
 * Parameters:
 *   dir_name (char *): The name of the directory to create.
 * No return value.
 */
void create_directory(char *dir_name) {
    // Attempt to create the directory with the specified permissions
    if (mkdir(dir_name, 0750) == 0) {
        // Successfully created directory
        printf("Created directory with name %s\n", dir_name);
    } else {
        // Failed to create directory, print error message and exit
        perror("Failed to create directory");
        exit(1);
    }
}

/*
 * Parses the specified CSV file and creates year-based text files in the given directory.
 * Sets permissions for each file to rw-r-----.
 * Parameters:
 *   directory (const char *): The directory where year-based files will be created.
 *   filename (const char *): The CSV file to parse.
 * No return value.
 */
void create_year_files(const char *directory, const char *filename) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        // If the file could not be opened, print an error message and exit
        perror("Failed to open file");
        exit(1);
    }

    char line[1024];
    char* token;
    char* year;
    char* title;
    char* file_name;
    int is_first_line = 1;

    // Read each line from the CSV file
    while (fgets(line, 1024, file)) {
        if (is_first_line) {
            // Skip the header line if it is the first line
            is_first_line = 0;
            continue;
        }
        // Extract the title and year from the line
        token = strtok(line, ",");  // Get the title
        title = token;
        token = strtok(NULL, ",");  // Get the year
        year = token;

        if (year == NULL) {
            // If year is missing, skip this line
            continue;
        }

        if (strcmp(year, "year") == 0) {
            // Skip lines that contain the header "year"
            continue;
        }

        // Allocate memory for the file name based on the directory and year
        size_t dir_len = strlen(directory);
        size_t year_len = strlen(year);
        file_name = malloc(dir_len + 1 + year_len + 4 + 1); // "/" + ".txt" + '\0'
        if (file_name == NULL) {
            // If memory allocation fails, print an error message and exit
            perror("Failed to allocate memory for file_name");
            exit(1);
        }
        sprintf(file_name, "%s/%s.txt", directory, year);  // Create the file name

        // Open the year-based file in append mode
        FILE* year_file = fopen(file_name, "a");
        if (year_file == NULL) {
            // If the year file could not be opened, print an error message and exit
            perror("Failed to open year file");
            exit(1);
        }

        // Write the title to the year-based file
        fprintf(year_file, "%s\n", title);
        fclose(year_file);  // Close the year file
        free(file_name);  // Free the allocated memory for the file name
    }

    fclose(file);  // Close the CSV file

    printf("Data has been written to files in the directory %s\n", directory);

    // Set permissions for each file to rw-r-----
    DIR* dir = opendir(directory);
    struct dirent* aDir;
    struct stat dirStat;
    char file_path[1024];

    // Iterate through the files in the directory and set their permissions
    while ((aDir = readdir(dir)) != NULL) {
        if (aDir->d_type == DT_REG) { // Only process regular files
            snprintf(file_path, sizeof(file_path), "%s/%s", directory, aDir->d_name);  // Create the file path
            stat(file_path, &dirStat);  // Get file statistics
            chmod(file_path, S_IRUSR | S_IWUSR | S_IRGRP);  // Set permissions to rw-r-----
        }
    }

    closedir(dir);  // Close the directory

    printf("Permissions have been set for each file in the directory %s\n", directory);
}