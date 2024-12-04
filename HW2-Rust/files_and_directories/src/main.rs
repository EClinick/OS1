/**
 * movies_processor.rs
 *
 * Description:
 * ------------
 * This Rust program processes CSV files containing movie data within the current directory.
 * It provides a user-friendly interface to select a CSV file based on specific criteria
 * (largest, smallest, or user-specified) and organizes the movies by their release year.
 * For each year, the program creates a text file listing all movie titles released that year.
 *
 * Features:
 * ---------
 * - **Directory Operations**: Reads directory entries to identify relevant CSV files.
 * - **File Selection**: Allows users to select the largest or smallest CSV file with
 *   the prefix `movies_` or specify a file by name.
 * - **CSV Parsing**: Utilizes the `csv` crate to parse CSV files and extract movie information.
 * - **Data Processing**: Organizes movies by their release year and creates corresponding text files.
 * - **Directory and File Creation**: Creates new directories and files with specific naming conventions
 *   and sets appropriate permissions using Unix-style permissions.
 * - **Error Handling**: Implements robust error handling to manage file access, parsing, and permission issues.
 *
 * Compilation:
 * ------------
 * To compile the program, ensure that Rust is installed on your system. Then, navigate to the project directory and run:
 *
 *     cargo build --release
 *
 * This will compile the program with optimizations enabled.
 *
 * Usage:
 * ------
 * Run the program from the directory containing the CSV files:
 *
 *     cargo run
 *
 * The program will present a menu-driven interface with the following options:
 *
 * 1. **Select file to process**: Choose a file based on size or specify a file name.
 * 2. **Exit the program**: Terminate the program.
 *
 * After selecting a file to process, the program will:
 * - Create a new directory named `<your_onid>.movies.<random_number>` with permissions `rwxr-x---`.
 * - Parse the selected CSV file to extract movie titles and their release years.
 * - For each release year, create a text file named `YYYY.txt` containing the titles of movies released that year,
 *   with permissions `rw-r-----`.
 *
 * Example Interaction:
 * --------------------
 * ```
 * 1. Select file to process
 * 2. Exit the program
 *
 * Enter a choice 1 or 2: 1
 *
 * Which file you want to process?
 * Enter 1 to pick the largest file
 * Enter 2 to pick the smallest file
 * Enter 3 to specify the name of a file
 *
 * Enter a choice from 1 to 3: 1
 * Now processing the chosen file named movies_1.csv
 * Created directory with name your_onid.movies.83465
 *
 * 1. Select file to process
 * 2. Exit the program
 *
 * Enter a choice 1 or 2: 2
 * Exiting the program.
 * ```
 *
 * Author:
 * -------
 * Ethan Clinick
 * CS374 HW5 FALL 2024
 * 12/4/2024
 */

use csv::ReaderBuilder; // For reading and parsing CSV files
use rand::Rng; // For generating random numbers
use std::collections::HashMap; // For storing movies organized by year
use std::env; // For accessing environment variables and current directory
use std::fs::{self, File, OpenOptions}; // For file and directory operations
use std::io::{self, Write}; // For input/output operations
use std::path::Path; // For handling filesystem paths
use std::process; // For exiting the program
use std::os::unix::fs::PermissionsExt; // For setting file and directory permissions

// Define a constant for the user's ONID (replace "clinicke" with your actual ONID)
const ONID: &str = "clinicke";

/// The main function serves as the entry point of the program.
/// It presents a menu to the user to either select a file to process or exit the program.
/// The program continues to loop until the user chooses to exit.
fn main() {
    loop {
        // Display the main menu options
        println!("1. Select file to process");
        println!("2. Exit the program\n");

        // Prompt the user to enter their choice
        print!("Enter a choice 1 or 2: ");
        io::stdout().flush().unwrap(); // Ensure the prompt is displayed immediately

        // Read the user's input
        let choice = read_user_input();

        // Handle the user's choice using a match statement
        match choice.as_str() {
            "1" => {
                // If the user chooses to select a file, attempt to select and process it
                if let Some(file_name) = select_file() {
                    println!("Now processing the chosen file named {}", file_name);
                    // Attempt to process the selected file and handle any errors
                    if let Err(e) = process_file(&file_name) {
                        eprintln!("Error processing file: {}", e);
                    }
                }
            }
            "2" => {
                // If the user chooses to exit, print a message and terminate the program
                println!("Exiting the program.");
                process::exit(0);
            }
            _ => {
                // If the user enters an invalid choice, display an error message
                println!("Invalid choice. Please enter 1 or 2.\n");
            }
        }
    }
}

/// Reads a line of input from the standard input (stdin),
/// trims any leading/trailing whitespace, and returns it as a String.
/// 
/// # Returns
/// 
/// A `String` containing the user's input.
fn read_user_input() -> String {
    let mut input = String::new(); // Initialize a mutable String to store user input
    io::stdin()
        .read_line(&mut input) // Read a line from stdin and store it in `input`
        .expect("Failed to read line"); // Panic with an error message if reading fails
    input.trim().to_string() // Trim whitespace and convert to String
}

/// Presents a submenu to the user for selecting a file to process.
/// The user can choose to pick the largest CSV file with the prefix `movies_`,
/// the smallest such file, or specify a file by name.
/// 
/// # Returns
/// 
/// An `Option<String>` containing the name of the selected file if successful.
fn select_file() -> Option<String> {
    loop {
        // Display the file selection menu options
        println!("\nWhich file you want to process?");
        println!("Enter 1 to pick the largest file");
        println!("Enter 2 to pick the smallest file");
        println!("Enter 3 to specify the name of a file\n");

        // Prompt the user to enter their choice
        print!("Enter a choice from 1 to 3: ");
        io::stdout().flush().unwrap(); // Ensure the prompt is displayed immediately

        // Read the user's input
        let choice = read_user_input();

        // Handle the user's choice using a match statement
        match choice.as_str() {
            "1" => {
                // If the user chooses to pick the largest file
                if let Some(file) = find_largest_csv() {
                    return Some(file); // Return the largest file's name
                } else {
                    // If no matching files are found, display an error message
                    println!("No files matching the criteria were found.\n");
                }
            }
            "2" => {
                // If the user chooses to pick the smallest file
                if let Some(file) = find_smallest_csv() {
                    return Some(file); // Return the smallest file's name
                } else {
                    // If no matching files are found, display an error message
                    println!("No files matching the criteria were found.\n");
                }
            }
            "3" => {
                // If the user chooses to specify a file by name
                print!("Enter the complete file name: ");
                io::stdout().flush().unwrap(); // Ensure the prompt is displayed immediately
                let file_name = read_user_input(); // Read the file name input

                // Check if the specified file exists in the current directory
                if Path::new(&file_name).exists() {
                    return Some(file_name); // Return the specified file's name
                } else {
                    // If the file does not exist, display an error message and loop again
                    println!("The file {} was not found. Try again\n", file_name);
                }
            }
            _ => {
                // If the user enters an invalid choice, display an error message
                println!("Invalid choice. Please enter a number from 1 to 3.\n");
            }
        }
    }
}

/// Finds the largest CSV file in the current directory that starts with the prefix `movies_`.
/// In case of a tie (multiple files with the same largest size), any one of them is returned.
/// 
/// # Returns
/// 
/// An `Option<String>` containing the name of the largest matching file if found.
fn find_largest_csv() -> Option<String> {
    let current_dir = env::current_dir().expect("Cannot access current directory"); // Get the current directory
    let mut largest_file: Option<(String, u64)> = None; // Initialize a variable to keep track of the largest file

    // Iterate over each entry in the current directory
    for entry in fs::read_dir(current_dir).expect("Cannot read directory") {
        if let Ok(entry) = entry {
            let path = entry.path(); // Get the path of the directory entry
            if path.is_file() {
                // Check if the entry is a file
                if let Some(file_name) = path.file_name().and_then(|n| n.to_str()) {
                    // Convert the file name to a string slice
                    if file_name.starts_with("movies_") && file_name.ends_with(".csv") {
                        // Check if the file name matches the required prefix and extension
                        if let Ok(metadata) = fs::metadata(&path) {
                            let size = metadata.len(); // Get the file size in bytes
                            match &largest_file {
                                Some((_, current_max)) => {
                                    // If a largest file is already tracked, compare sizes
                                    if size > *current_max {
                                        largest_file = Some((file_name.to_string(), size)); // Update if current file is larger
                                    }
                                }
                                None => {
                                    // If no largest file is tracked yet, set the current file as largest
                                    largest_file = Some((file_name.to_string(), size));
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // If a largest file is found, print a message and return its name
    largest_file.map(|(name, _)| {
        println!("Now processing the chosen file named {}", name);
        name
    })
}

/// Finds the smallest CSV file in the current directory that starts with the prefix `movies_`.
/// In case of a tie (multiple files with the same smallest size), any one of them is returned.
/// 
/// # Returns
/// 
/// An `Option<String>` containing the name of the smallest matching file if found.
fn find_smallest_csv() -> Option<String> {
    let current_dir = env::current_dir().expect("Cannot access current directory"); // Get the current directory
    let mut smallest_file: Option<(String, u64)> = None; // Initialize a variable to keep track of the smallest file

    // Iterate over each entry in the current directory
    for entry in fs::read_dir(current_dir).expect("Cannot read directory") {
        if let Ok(entry) = entry {
            let path = entry.path(); // Get the path of the directory entry
            if path.is_file() {
                // Check if the entry is a file
                if let Some(file_name) = path.file_name().and_then(|n| n.to_str()) {
                    // Convert the file name to a string slice
                    if file_name.starts_with("movies_") && file_name.ends_with(".csv") {
                        // Check if the file name matches the required prefix and extension
                        if let Ok(metadata) = fs::metadata(&path) {
                            let size = metadata.len(); // Get the file size in bytes
                            match &smallest_file {
                                Some((_, current_min)) => {
                                    // If a smallest file is already tracked, compare sizes
                                    if size < *current_min {
                                        smallest_file = Some((file_name.to_string(), size)); // Update if current file is smaller
                                    }
                                }
                                None => {
                                    // If no smallest file is tracked yet, set the current file as smallest
                                    smallest_file = Some((file_name.to_string(), size));
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // If a smallest file is found, print a message and return its name
    smallest_file.map(|(name, _)| {
        println!("Now processing the chosen file named {}", name);
        name
    })
}

/// Processes the specified CSV file by performing the following operations:
/// 
/// 1. Creates a new directory named `your_onid.movies.random` with permissions `rwxr-x---`.
/// 2. Parses the CSV file to organize movies by their release year.
/// 3. Creates a `.txt` file for each year containing the titles of movies released that year,
///    with permissions `rw-r-----`.
/// 
/// After processing, the program returns to the main menu.
/// 
/// # Arguments
/// 
/// * `file_name` - A string slice that holds the name of the file to process.
/// 
/// # Returns
/// 
/// A `Result` which is:
/// 
/// - `Ok(())` if the file was processed successfully.
/// - An error of type `Box<dyn std::error::Error>` if an error occurred during processing.
fn process_file(file_name: &str) -> Result<(), Box<dyn std::error::Error>> {
    // Generate a random number between 0 and 99999 inclusive for the directory name
    let random_number = rand::thread_rng().gen_range(0..=99999);
    // Format the directory name using the user's ONID and the random number
    let dir_name = format!("{}.movies.{}", ONID, random_number);
    fs::create_dir(&dir_name)?; // Create the new directory

    // Set permissions to rwxr-x--- (owner: read, write, execute; group: read, execute; others: none)
    let mut perms = fs::metadata(&dir_name)?.permissions(); // Get current permissions
    perms.set_mode(0o750); // Set the desired permissions using octal notation
    fs::set_permissions(&dir_name, perms)?; // Apply the new permissions to the directory

    println!("Created directory with name {}\n", dir_name); // Inform the user about the created directory

    // Open the specified CSV file for reading
    let file = File::open(file_name)?;
    // Initialize a CSV reader with headers
    let mut rdr = ReaderBuilder::new()
        .has_headers(true)
        .from_reader(file);

    // Initialize a HashMap to store movie titles organized by their release year
    let mut movies_by_year: HashMap<String, Vec<String>> = HashMap::new();

    // Iterate over each record (row) in the CSV file
    for result in rdr.records() {
        let record = result?; // Unwrap the result or return an error

        // Extract the 'Title' and 'Year' fields from the record
        let title = record.get(0).unwrap_or("").to_string(); // Get the first column (Title)
        let year = record.get(1).unwrap_or("").to_string(); // Get the second column (Year)

        // If both title and year are present, add the title to the corresponding year's list
        if !title.is_empty() && !year.is_empty() {
            movies_by_year.entry(year).or_insert_with(Vec::new).push(title);
        }
    }

    // Iterate over each year and its corresponding list of movie titles
    for (year, titles) in movies_by_year {
        // Define the path for the year's text file within the new directory
        let year_file_path = format!("{}/{}.txt", dir_name, year);
        // Open the year's text file for writing, creating it if it doesn't exist
        let mut file = OpenOptions::new()
            .write(true) // Enable writing
            .create(true) // Create the file if it doesn't exist
            .truncate(true) // Truncate the file to zero length if it exists
            .open(&year_file_path)?; // Open the file

        // Write each movie title to the year's text file, one per line
        for title in titles {
            writeln!(file, "{}", title)?; // Write the title followed by a newline
        }

        // Set permissions to rw-r----- (owner: read, write; group: read; others: none)
        let mut perms = fs::metadata(&year_file_path)?.permissions(); // Get current permissions
        perms.set_mode(0o640); // Set the desired permissions using octal notation
        fs::set_permissions(&year_file_path, perms)?; // Apply the new permissions to the file
    }

    Ok(()) // Indicate that the file was processed successfully
}
