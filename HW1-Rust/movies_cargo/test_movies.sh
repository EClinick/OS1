#!/bin/bash

# Set test file path (using relative path)
TEST_FILE="movies_sample_1.csv"

# Copy test file to current directory if needed
if [ ! -f "$TEST_FILE" ]; then
    cp "/nfs/stak/users/clinicke/OS1/HW1-Rust/movies_cargo/movies_sample_1.csv" ./ 2>/dev/null || {
        echo "Error: Could not copy test file"
        exit 1
    }
fi

# Function to run test case with timeout
run_test() {
    local test_name=$1
    local input=$2
    local expected=$3
    
    echo "Running test: $test_name"
    actual=$(timeout 5s sh -c "printf '$input' | cargo run -- $TEST_FILE" 2>&1)
    
    if [ $? -eq 124 ]; then
        echo "✗ Test timed out"
        return 1
    fi
    
    if echo "$actual" | grep -q "$expected"; then
        echo "✓ Test passed"
    else
        echo "✗ Test failed"
        echo "Expected output to contain: $expected"
        echo "Actual output: $actual"
    fi
    echo "------------------------"
}

# Test cases
run_test "Initial Load" "4\n" "Processed file movies_sample_1.csv and parsed data for 24 movies"

run_test "No Movies in 1999" "1\n1999\n4\n" "No data about movies released in the year 1999"

run_test "Movies in 2012" "1\n2012\n4\n" "The Avengers\nRise of the Guardians\nAnna Karenina"

run_test "Highest Rated Movies" "2\n4\n" "2018 8.5 Avengers: Infinity War"

run_test "English Movies" "3\nEnglish\n4\n" "2008 The Incredible Hulk"

run_test "No Punjabi Movies" "3\nPunjabi\n4\n" "No data about movies released in Punjabi"

run_test "Invalid Choice" "5\n4\n" "You entered an incorrect choice. Try again."