#!/bin/bash

# Create generated-output directory if it doesn't exist
mkdir -p ./tests/generated-output

# Run the three test cases
./lineprocessor < ./tests/input1-1.txt > ./tests/generated-output/output1.txt
./lineprocessor < ./tests/input2-1.txt > ./tests/generated-output/output2.txt
./lineprocessor < ./tests/input3-1.txt > ./tests/generated-output/output3.txt

# Function to compare and display outputs
compare_outputs() {
    local test_num=$1
    local generated_file="./tests/generated-output/output${test_num}.txt"
    local expected_file="./tests/output${test_num}-1.txt"
    
    echo "============= Test ${test_num} ============="
    echo "----- Generated output: -----"
    cat "$generated_file"
    echo
    echo "----- Expected output: -----"
    cat "$expected_file"
    echo
    
    if diff "$generated_file" "$expected_file" >/dev/null ; then
        echo "Test ${test_num}: PASSED"
    else
        echo "Test ${test_num}: FAILED"
        echo "----- Differences: -----"
        diff "$generated_file" "$expected_file"
    fi
    echo "====================================="
    echo
}

# Compare all outputs
compare_outputs 1
compare_outputs 2
compare_outputs 3