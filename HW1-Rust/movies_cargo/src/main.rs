/*
* Ethan Clinick
* CS374 HW1 to Rust Extra Credit FALL 2024
* 10/1/2024
*/

use std::io;
use std::error::Error;
use std::fs::File;
use csv::ReaderBuilder;

struct Movie {
    title: String,
    year: i32,
    languages: Vec<String>,
    // num_languages: i32,
    rating: f32,
}

fn read_csv(filename: &str) -> Result<Vec<Movie>, Box<dyn Error>> {
    let file = File::open(filename)?;
    let mut rdr = ReaderBuilder::new().from_reader(file);
    let mut movies = Vec::new();
    for result in rdr.records() {
        let record = result?;
        let title = record.get(0).unwrap_or("").trim().to_string();
        let year_str = record.get(1).unwrap_or("").trim();
        let languages_str = record.get(2).unwrap_or("");
        // let num_languages_str = languages_str.len().to_string();
        let rating_str = record.get(3).unwrap_or("").trim();

        // Skip the record if essential fields are missing
        if title.is_empty() || year_str.is_empty() {
            println!("Skipping record due to missing title or year.");
            continue;
        }

        // Parsing with error handling
        let year = match year_str.parse::<i32>() {
            Ok(y) => y,
            Err(_) => {
                println!("Invalid year '{}'. Skipping record.", year_str);
                continue;
            }
        };

        let languages: Vec<String> = languages_str
            .split(',')
            .map(|s| s.trim().to_string())
            .collect();

        // let num_languages = match num_languages_str.parse::<i32>() {
        //     Ok(n) => n,
        //     Err(_) => {
        //         println!("Invalid num_languages '{}'. Setting to 0.", num_languages_str);
        //         0 // Default value
        //     }
        // };

        let rating = match rating_str.parse::<f32>() {
            Ok(r) => r,
            Err(_) => {
                println!("Invalid rating '{}'. Setting to 0.0.", rating_str);
                0.0 // Default value
            }
        };

        movies.push(Movie {
            title,
            year,
            
            languages,
            rating,
        });
    }
    Ok(movies)
}

fn show_movies_by_year(movies: &Vec<Movie>, year: i32) {
    let mut found = false;
    for movie in movies {
        if movie.year == year {
            println!("{} ({})", movie.title, movie.year);
            found = true;
        }
    }
    if !found {
        println!("No movies found in {}", year);
    }
}

fn show_highest_rated_movies(movies: &Vec<Movie>) {
    let mut highest_rated: Vec<Movie> = Vec::new();
    for movie in movies {
        let mut found = false;
        for hr_movie in &mut highest_rated {
            if hr_movie.year == movie.year {
                if hr_movie.rating < movie.rating {
                    hr_movie.rating = movie.rating;
                    hr_movie.title = movie.title.clone();
                }
                found = true;
                break;
            }
        }
        if !found {
            highest_rated.push(Movie { title: movie.title.clone(), year: movie.year, languages: movie.languages.clone(), rating: movie.rating });
        }
    }
    //Sorting from year oldest to newest
    highest_rated.sort_by(|a, b| a.year.cmp(&b.year));

    for movie in highest_rated {
        println!("{} ({}) - {}", movie.title, movie.year, movie.rating);
    }
}

fn show_movies_by_language(movies: &Vec<Movie>, language: &str) {
    let mut found = false;
    for movie in movies {
        if movie.languages.contains(&language.to_string()) {
            println!("{} ({})", movie.title, movie.year);
            found = true;
        }
    }
    if !found {
        println!("No movies found in {}", language);
    }
}

fn parse_user_csv_input(input: &str) -> Vec<String> {
    input.split(',').map(|s| s.trim().to_string()).collect()
}

fn print_menu() {
    println!("\n---------------------------------");
    println!("Choose an option:");
    println!("1. Show movies released in the specified year");
    println!("2. Show highest rated movie for each year");
    println!("3. Show the title and year of release of all movies in a specific language");
    println!("4. Quit");
    println!("---------------------------------\n");
}


fn main() -> Result<(), Box<dyn Error>> {
    // Declare movies outside the loop so it's in scope
    let mut movies: Vec<Movie> = Vec::new();
    
    loop {
        if movies.is_empty() {
            println!("Enter the name of the CSV file:");
            let mut filename = String::new();
            io::stdin().read_line(&mut filename)?;
            let filename = filename.trim();
            
            // Attempt to read the CSV file
            match read_csv(filename) {
                Ok(loaded_movies) => {
                    if loaded_movies.is_empty() {
                        println!("No movies found in the file. Please try again.");
                        continue; // Ask for the filename again
                    } else {
                        println!("{} movies loaded successfully.", loaded_movies.len());
                        movies = loaded_movies; // Update the movies variable
                    }
                },
                Err(e) => {
                    println!("Error reading file: {}. Please try again.", e);
                    continue; // Ask for the filename again
                },
            }
        }
        
        // Display the menu
        print_menu();
        
        let mut choice = String::new();
        io::stdin().read_line(&mut choice)?;
        let choice: i32 = match choice.trim().parse() {
            Ok(num) => num,
            Err(_) => {
                println!("Invalid choice. Please enter a number.");
                continue;
            }
        };
        
        match choice {
            1 => {
                println!("Enter the year:");
                let mut year_input = String::new();
                io::stdin().read_line(&mut year_input)?;
                let year: i32 = match year_input.trim().parse() {
                    Ok(num) => num,
                    Err(_) => {
                        println!("Invalid year. Please enter a valid number.");
                        continue;
                    }
                };
                show_movies_by_year(&movies, year);
            },
            2 => {
                show_highest_rated_movies(&movies);
            },
            3 => {
                println!("Enter the language:");
                let mut language = String::new();
                io::stdin().read_line(&mut language)?;
                let language = language.trim();
                show_movies_by_language(&movies, language);
            },
            4 => {
                println!("Exiting the program.");
                return Ok(());
            },
            _ => {
                println!("Invalid choice. Please select a valid option.");
            }
        }
    }
}

#[cfg(test)]
//To run tests: cargo test
mod tests {
    use super::*;

    #[test]
    fn test_read_csv() {
        let movies = read_csv("movies_sample_1.csv").unwrap();
        assert_eq!(movies.len(), 5);
        assert_eq!(movies[0].title, "The Shawshank Redemption");
        assert_eq!(movies[0].year, 1994);
        assert_eq!(movies[0].languages, vec!["English".to_string()]);
        //assert_eq!(movies[0].num_languages, 1);
        assert_eq!(movies[0].rating, 9.3);
    }

    #[test]
    fn test_show_movies_by_year() {
        let movies = read_csv("movies_sample_1.csv").unwrap();
        show_movies_by_year(&movies, 1994);
    }

    #[test]
    fn test_show_highest_rated_movies() {
        let movies = read_csv("movies_sample_1.csv").unwrap();
        show_highest_rated_movies(&movies);
    }

    #[test]
    fn test_show_movies_by_language() {
        let movies = read_csv("movies_sample_1.csv").unwrap();
        show_movies_by_language(&movies, "English");
    }
}