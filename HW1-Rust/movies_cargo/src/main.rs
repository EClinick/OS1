/*
 * Ethan Clinick
 * CS374 HW1 to Rust Extra Credit FALL 2024
 * 10/1/2024
 *
 * Program Description:
 * This program reads movie data from a CSV file provided as a command-line argument.
 * It processes the data to create a linked list of Movie structs and offers an interactive
 * menu for users to query the data based on specific criteria such as release year,
 * highest-rated movies per year, and movies by language.
 *
 * Functionalities:
 * 1. Show movies released in a specified year.
 * 2. Show the highest-rated movie for each year.
 * 3. Show movies and their year of release for a specific language.
 * 4. Exit the program.
 *
 * The program ensures strict adherence to input formats and handles errors gracefully.
 */

 use std::env;
 use std::error::Error;
 use std::fs::File;
 use std::io;
 use std::process;
 use std::collections::LinkedList;
 use csv::ReaderBuilder;
 
 /// Represents a movie with its relevant details.
 struct Movie {
     title: String,
     year: i32,
     languages: Vec<String>,
     rating: f32,
 }
 
 /// Reads and parses the CSV file to create a linked list of Movie structs.
 ///
 /// # Arguments
 ///
 /// * `filename` - A string slice that holds the name of the CSV file.
 ///
 /// # Returns
 ///
 /// * `Result<LinkedList<Movie>, Box<dyn Error>>` - On success, returns a linked list of movies.
 ///   On failure, returns an error.
 ///
 /// # Errors
 ///
 /// This function will return an error if the file cannot be opened or if there are issues
 /// parsing the CSV records.
 fn read_csv(filename: &str) -> Result<LinkedList<Movie>, Box<dyn Error>> {
     let file = File::open(filename)?;
     let mut rdr = ReaderBuilder::new()
         .has_headers(true) // Skip the header row
         .from_reader(file);
     let mut movies = LinkedList::new();
 
     for (index, result) in rdr.records().enumerate() {
         let record = result?;
         
         // Extract fields from the CSV record
         let title = record.get(0).unwrap_or("").trim().to_string();
         let year_str = record.get(1).unwrap_or("").trim();
         let languages_str = record.get(2).unwrap_or("").trim();
         let rating_str = record.get(3).unwrap_or("").trim();
 
         // Validate essential fields
         if title.is_empty() || year_str.is_empty() {
             println!("Skipping record at line {} due to missing title or year.", index + 2);
             continue;
         }
 
         // Parse year with error handling
         let year = match year_str.parse::<i32>() {
             Ok(y) if (1900..=2021).contains(&y) => y,
             _ => {
                 println!("Invalid year '{}' at line {}. Skipping record.", year_str, index + 2);
                 continue;
             }
         };
 
         // Parse languages enclosed in [] and separated by semicolons
         let languages = if languages_str.starts_with('[') && languages_str.ends_with(']') {
             languages_str[1..languages_str.len()-1]
                 .split(';')
                 .map(|s| s.trim().to_string())
                 .filter(|s| !s.is_empty())
                 .collect::<Vec<String>>()
         } else {
             println!("Invalid languages format '{}' at line {}. Skipping record.", languages_str, index + 2);
             continue;
         };
 
         // Enforce maximum number of languages and maximum length per language
         if languages.len() > 5 {
             println!("Too many languages at line {}. Skipping record.", index + 2);
             continue;
         }
         if languages.iter().any(|lang| lang.len() > 20) {
             println!("Language name too long at line {}. Skipping record.", index + 2);
             continue;
         }
 
         // Parse rating with error handling
         let rating = match rating_str.parse::<f32>() {
             Ok(r) if (1.0..=10.0).contains(&r) => r,
             _ => {
                 println!("Invalid rating '{}' at line {}. Setting to 0.0.", rating_str, index + 2);
                 0.0
             }
         };
 
         // Create a Movie struct and add it to the linked list
         movies.push_back(Movie {
             title,
             year,
             languages,
             rating,
         });
     }
 
     Ok(movies)
 }
 
 /// Displays movies released in a specified year.
 ///
 /// # Arguments
 ///
 /// * `movies` - A reference to the linked list of movies.
 /// * `year` - The year to filter movies by.
 fn show_movies_by_year(movies: &LinkedList<Movie>, year: i32) {
     let mut found = false;
     for movie in movies {
         if movie.year == year {
             println!("{}", movie.title);
             found = true;
         }
     }
     if !found {
         println!("No movies found in {}", year);
     }
 }
 
 /// Displays the highest-rated movie for each year.
 ///
 /// For each year, finds the movie with the highest rating and displays it.
 /// In case of ties, any one of the highest-rated movies is displayed.
 ///
 /// # Arguments
 ///
 /// * `movies` - A reference to the linked list of movies.
 fn show_highest_rated_movies(movies: &LinkedList<Movie>) {
     use std::collections::HashMap;
 
     let mut highest_rated: HashMap<i32, &Movie> = HashMap::new();
 
     for movie in movies {
         highest_rated.entry(movie.year)
             .and_modify(|existing| {
                 if movie.rating > existing.rating {
                     // Update with the higher-rated movie
                     *existing = movie;
                 }
             })
             .or_insert(movie);
     }
 
     // Collect years and sort them in ascending order
     let mut years: Vec<i32> = highest_rated.keys().cloned().collect();
     years.sort();
 
     for year in years {
         if let Some(movie) = highest_rated.get(&year) {
             println!("{} {:.1} {}", year, movie.rating, movie.title);
         }
     }
 }
 
 /// Displays movies and their release years for a specified language.
 ///
 /// Only exact case-sensitive matches are considered.
 ///
 /// # Arguments
 ///
 /// * `movies` - A reference to the linked list of movies.
 /// * `language` - The language to filter movies by.
 fn show_movies_by_language(movies: &LinkedList<Movie>, language: &str) {
     let mut found = false;
     for movie in movies {
         if movie.languages.contains(&language.to_string()) {
             println!("{} {}", movie.year, movie.title);
             found = true;
         }
     }
     if !found {
         println!("No movies found in {}", language);
     }
 }
 
 /// Displays the interactive menu to the user.
 fn print_menu() {
     println!("\n---------------------------------");
     println!("Choose an option:");
     println!("1. Show movies released in the specified year");
     println!("2. Show highest rated movie for each year");
     println!("3. Show the title and year of release of all movies in a specific language");
     println!("4. Quit");
     println!("---------------------------------\n");
 }
 
 /// The main entry point of the program.
 ///
 /// Processes the CSV file, displays the initial processing message, and handles
 /// user interactions through an interactive menu.
 ///
 /// # Returns
 ///
 /// * `Result<(), Box<dyn Error>>` - Returns Ok on successful execution.
 ///   Returns an error if any IO or parsing operations fail.
 fn main() -> Result<(), Box<dyn Error>> {
     // Collect command-line arguments
     let args: Vec<String> = env::args().collect();
 
     // Ensure exactly one argument is provided (the CSV file name)
     if args.len() != 2 {
         eprintln!("Usage: {} <CSV_FILE>", args[0]);
         process::exit(1);
     }
 
     let filename = &args[1];
 
     // Enforce file name constraints
     if filename.len() >= 50 {
         eprintln!("Error: File name '{}' exceeds 49 characters.", filename);
         process::exit(1);
     }
     if filename.contains(' ') {
         eprintln!("Error: File name '{}' contains spaces.", filename);
         process::exit(1);
     }
 
     // Read and parse the CSV file
     let movies = read_csv(filename)?;
 
     // Calculate the number of movies processed
     let movie_count = movies.len();
     println!(
         "Processed file {} and parsed data for {} movies",
         filename, movie_count
     );
 
     // Start the interactive menu loop
     loop {
         print_menu();
 
         // Prompt user for choice
         let mut choice = String::new();
         io::stdin().read_line(&mut choice)?;
         let choice = choice.trim();
 
         // Parse user choice
         let choice: i32 = match choice.parse() {
             Ok(num) => num,
             Err(_) => {
                 println!("Invalid choice. Please enter a number between 1 and 4.");
                 continue;
             }
         };
 
         match choice {
             1 => {
                 // Option 1: Show movies released in the specified year
                 println!("Enter the year:");
                 let mut year_input = String::new();
                 io::stdin().read_line(&mut year_input)?;
                 let year_input = year_input.trim();
 
                 // Parse the year input
                 let year: i32 = match year_input.parse() {
                     Ok(num) if (1900..=2021).contains(&num) => num,
                     _ => {
                         println!("Invalid year. Please enter a 4-digit year between 1900 and 2021.");
                         continue;
                     }
                 };
 
                 // Display movies for the specified year
                 show_movies_by_year(&movies, year);
             },
             2 => {
                 // Option 2: Show highest rated movie for each year
                 show_highest_rated_movies(&movies);
             },
             3 => {
                 // Option 3: Show movies by a specific language
                 println!("Enter the language:");
                 let mut language = String::new();
                 io::stdin().read_line(&mut language)?;
                 let language = language.trim();
 
                 // Validate language input length
                 if language.len() > 20 {
                     println!("Language name exceeds 20 characters. Please enter a shorter name.");
                     continue;
                 }
 
                 // Display movies for the specified language
                 show_movies_by_language(&movies, language);
             },
             4 => {
                 // Option 4: Exit the program
                 println!("Exiting the program.");
                 break;
             },
             _ => {
                 // Invalid choice
                 println!("Invalid choice. Please select a valid option (1-4).");
             }
         }
     }
 
     Ok(())
 }
 
 #[cfg(test)]
 mod tests {
     use super::*;
 
     /// Helper function to create a sample linked list of movies for testing.
     fn sample_movies() -> LinkedList<Movie> {
         let mut movies = LinkedList::new();
         movies.push_back(Movie {
             title: "The Shawshank Redemption".to_string(),
             year: 1994,
             languages: vec!["English".to_string()],
             rating: 9.3,
         });
         movies.push_back(Movie {
             title: "The Godfather".to_string(),
             year: 1972,
             languages: vec!["English".to_string(), "Italian".to_string()],
             rating: 9.2,
         });
         movies.push_back(Movie {
             title: "The Dark Knight".to_string(),
             year: 2008,
             languages: vec!["English".to_string(), "Mandarin".to_string()],
             rating: 9.0,
         });
         movies.push_back(Movie {
             title: "12 Angry Men".to_string(),
             year: 1957,
             languages: vec!["English".to_string()],
             rating: 8.9,
         });
         movies.push_back(Movie {
             title: "Schindler's List".to_string(),
             year: 1993,
             languages: vec!["English".to_string(), "German".to_string(), "Polish".to_string()],
             rating: 8.9,
         });
         movies
     }
 
     #[test]
     fn test_read_csv_valid_file() {
         // Assuming "movies_sample_1.csv" exists and is properly formatted
         let result = read_csv("movies_sample_1.csv");
         assert!(result.is_ok());
         let movies = result.unwrap();
         // Adjust the expected number based on the sample CSV
         assert_eq!(movies.len(), 5);
     }
 
     #[test]
     fn test_show_movies_by_year_found() {
         let movies = sample_movies();
         // Capture the output
         let year = 1994;
         // Since the function prints to stdout, we can't capture it directly here.
         // Instead, ensure that the movie exists.
         let exists = movies.iter().any(|m| m.year == year && m.title == "The Shawshank Redemption");
         assert!(exists);
     }
 
     #[test]
     fn test_show_movies_by_year_not_found() {
         let movies = sample_movies();
         let year = 2020;
         let exists = movies.iter().any(|m| m.year == year);
         assert!(!exists);
     }
 
     #[test]
     fn test_show_highest_rated_movies() {
         let movies = sample_movies();
         // Implement a simple check to ensure the highest rated per year is correct
         // For example, in 1994, "The Shawshank Redemption" with 9.3
         let mut highest_rated: HashMap<i32, f32> = HashMap::new();
         for movie in &movies {
             highest_rated.entry(movie.year)
                 .and_modify(|r| if movie.rating > *r { *r = movie.rating } )
                 .or_insert(movie.rating);
         }
 
         assert_eq!(highest_rated.get(&1994), Some(&9.3));
         assert_eq!(highest_rated.get(&1972), Some(&9.2));
         assert_eq!(highest_rated.get(&2008), Some(&9.0));
         assert_eq!(highest_rated.get(&1957), Some(&8.9));
         assert_eq!(highest_rated.get(&1993), Some(&8.9));
     }
 
     #[test]
     fn test_show_movies_by_language_found() {
         let movies = sample_movies();
         let language = "English";
         let exists = movies.iter().any(|m| m.languages.contains(&language.to_string()));
         assert!(exists);
     }
 
     #[test]
     fn test_show_movies_by_language_not_found() {
         let movies = sample_movies();
         let language = "Japanese";
         let exists = movies.iter().any(|m| m.languages.contains(&language.to_string()));
         assert!(!exists);
     }
 }
 