/*
* Ethan Clinick
* CS374 HW1 FALL 2024
* 10/1/2024
*/


/*
Summary: This program reads a file of movies and allows the user to select from a menu of options to display movies based on the year, rating, or language.
Paramaters:
    - The program takes in a file of movies as a command line argument
    - The program reads the file and creates a linked list of movie structs
    - The program displays a menu of options to the user
    - The user can select from the following options:
        1. Show movies released in the specified year
        2. Show highest rated movie for each year
        3. Show the title and year of release of all movies in a specific language
        4. Exit from the program
    - The program will continue to display the menu until the user selects the exit option
    - The program will free the linked list of movie structs before exiting
Returns:
    - The program will return EXIT_SUCCESS if it completes successfully
    - The program will return EXIT_FAILURE if there is an error

*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct movie {
    char *title; // The title of the movie
    int year; // The year the movie was released
    char *languages[5]; // The languages the movie is available in
    int num_languages; // The number of languages the movie is available in
    double rating; // The rating value of the movie
    struct movie *next; // Pointer to the next movie in the linked list
};


/*
Summary: Function to create a new movie struct from the current line
Parameters: char *currLine - the current line from the file
Returns: struct movie * - the new movie struct
*/

// Function to create a new movie struct from the current line
struct movie *createMovie(char *currLine) {
    struct movie *currMovie = malloc(sizeof(struct movie));
    currMovie->next = NULL;
    currMovie->num_languages = 0;

    char *saveptr;

    // The first token is the Title
    char *token = strtok_r(currLine, ",", &saveptr);
    currMovie->title = calloc(strlen(token) + 1, sizeof(char));
    strcpy(currMovie->title, token);

    // The next token is the Year
    token = strtok_r(NULL, ",", &saveptr);
    currMovie->year = atoi(token);

    // The next token is the Languages
    token = strtok_r(NULL, ",", &saveptr);
    // Remove the '[' and ']' from the token
    char *langStr = calloc(strlen(token) + 1, sizeof(char));
    strcpy(langStr, token);
    if (langStr[0] == '[') {
        memmove(langStr, langStr+1, strlen(langStr));
    }
    if (langStr[strlen(langStr)-1] == ']') {
        langStr[strlen(langStr)-1] = '\0';
    }
    // Split the languages by ';'
    char *langSavePtr;
    char *langToken = strtok_r(langStr, ";", &langSavePtr);
    int langCount = 0;
    while (langToken != NULL && langCount < 5) {
        currMovie->languages[langCount] = calloc(strlen(langToken) + 1, sizeof(char));
        strcpy(currMovie->languages[langCount], langToken);
        langCount++;
        langToken = strtok_r(NULL, ";", &langSavePtr);
    }
    currMovie->num_languages = langCount;
    free(langStr);

    // The next token is the Rating Value
    token = strtok_r(NULL, ",", &saveptr);
    currMovie->rating = strtod(token, NULL);

    return currMovie;
}

/*
Summary: Function to process the file and return the linked list of movies
Parameters: char *filePath - the path to the file to process
            int *movieCount - a pointer to the variable to store the number of movies
Returns: struct movie * - the head of the linked list of movies
*/

// Function to process the file and return the linked list of movies
struct movie *processFile(char *filePath, int *movieCount) {
    FILE *movieFile = fopen(filePath, "r");
    if (!movieFile) {
        fprintf(stderr, "Could not open file %s\n", filePath);
        exit(1);
    }

    char *currLine = NULL;
    size_t len = 0;
    ssize_t nread;
    struct movie *head = NULL;
    struct movie *tail = NULL;
    *movieCount = 0;

    // Skip the header line
    getline(&currLine, &len, movieFile);

    // Read the file line by line
    while ((nread = getline(&currLine, &len, movieFile)) != -1) {
        // Remove the newline character at the end of currLine
        if (currLine[nread - 1] == '\n') {
            currLine[nread - 1] = '\0';
        }
        struct movie *newMovie = createMovie(currLine);
        (*movieCount)++;

        // Add to the linked list
        if (head == NULL) {
            head = newMovie;
            tail = newMovie;
        } else {
            tail->next = newMovie;
            tail = newMovie;
        }
    }

    free(currLine);
    fclose(movieFile);
    return head;
}

// Function to print the menu options

/*
Summary: Function to print the menu options
Parameters: None
Returns: None

*/

void printMenu() {
    printf("\n1. Show movies released in the specified year\n");
    printf("2. Show highest rated movie for each year\n");
    printf("3. Show the title and year of release of all movies in a specific language\n");
    printf("4. Exit from the program\n\n");
}


/*
Summary: Function to show movies released in the specified year
Parameters: struct movie *list - the head of the linked list of movies
Returns: None


*/

// Function for choice 1
void showMoviesByYear(struct movie *list) {
    printf("Enter the year for which you want to see movies: ");
    int year;
    scanf("%d", &year);
    struct movie *curr = list;
    int found = 0;
    while (curr != NULL) {
        if (curr->year == year) {
            printf("%s\n", curr->title);
            found = 1;
        }
        curr = curr->next;
    }
    if (!found) {
        printf("No data about movies released in the year %d\n", year);
    }
}

/*
Summary: Function to show the highest rated movie for each year
Parameters: struct movie *list - the head of the linked list of movies
Returns: None

*/

// Function for choice 2
void showHighestRatedMovies(struct movie *list) {
    // We need to find the highest rated movie for each year
    // Since years are between 1900 and 2021, we can use an array indexed by year - 1900
    struct movie *highestRated[122]; // From 1900 to 2021 inclusive
    for (int i = 0; i < 122; i++) {
        highestRated[i] = NULL;
    }
    struct movie *curr = list;
    while (curr != NULL) {
        int index = curr->year - 1900;
        if (highestRated[index] == NULL || curr->rating > highestRated[index]->rating) {
            highestRated[index] = curr;
        }
        curr = curr->next;
    }
    // Now print the highest rated movies
    for (int i = 121; i >=0; i--) {
        if (highestRated[i] != NULL) {
            printf("%d %.1f %s\n", highestRated[i]->year, highestRated[i]->rating, highestRated[i]->title);
        }
    }
}

/*
Summary: Function to show the title and year of release of all movies in a specific language
Parameters: struct movie *list - the head of the linked list of movies
Returns: None
*/

// Function for choice 3
void showMoviesByLanguage(struct movie *list) {
    printf("Enter the language for which you want to see movies: ");
    char language[21];
    scanf("%20s", language); // Limit input to 20 characters
    struct movie *curr = list;
    int found = 0;
    while (curr != NULL) {
        for (int i = 0; i < curr->num_languages; i++) {
            if (strcmp(curr->languages[i], language) == 0) {
                printf("%d %s\n", curr->year, curr->title);
                found = 1;
                break;
            }
        }
        curr = curr->next;
    }
    if (!found) {
        printf("No data about movies released in %s\n", language);
    }
}
/*
Summary: Function to free the linked list
Parameters: struct movie *list - the head of the linked list of movies
Returns: None
*/


// Function to free the linked list
void freeMovieList(struct movie *list) {
    struct movie *curr = list;
    while (curr != NULL) {
        struct movie *temp = curr;
        curr = curr->next;
        // Free the allocated strings
        free(temp->title);
        for (int i = 0; i < temp->num_languages; i++) {
            free(temp->languages[i]);
        }
        free(temp);
    }
}

/*
Summary: Main function to process the file and show the menu
Parameters: int argc - the number of command line arguments
            char *argv[] - the array of command line arguments
Returns: int - the exit status of the program

*/


int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("You must provide the name of the file to process\n");
        printf("Example: %s movies.csv\n", argv[0]);
        return EXIT_FAILURE;
    }

    int movieCount = 0;
    struct movie *list = processFile(argv[1], &movieCount);
    printf("Processed file %s and parsed data for %d movies\n", argv[1], movieCount);

    int choice = 0;
    while (1) {
        printMenu();
        printf("Enter a choice from 1 to 4: ");
        scanf("%d", &choice);
        if (choice == 1) {
            showMoviesByYear(list);
        } else if (choice == 2) {
            showHighestRatedMovies(list);
        } else if (choice == 3) {
            showMoviesByLanguage(list);
        } else if (choice == 4) {
            break;
        } else {
            printf("You entered an incorrect choice. Try again.\n");
        }
    }

    // Free the linked list
    freeMovieList(list);

    return EXIT_SUCCESS;
}