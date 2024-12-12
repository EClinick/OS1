# OS1
CS374-OS1

## Table of Contents
1. [Overview](#overview)
2. [Assignments](#assignments)
    - [HW1: Movie Data Processing](#hw1-movie-data-processing)
        - C Implementation
        - Rust Implementation
        - Features
            * Movie Year Search
            * Rating Analysis
            * Language Search
    - [HW2: File Directory Processing](#hw2-file-directory-processing)
        - C Implementation
        - Rust Implementation
        - File Organization System
    - [HW3: Small Shell](#hw3-small-shell-smallsh)
        - Built-in Commands
        - Process Management
        - Signal Handling
        - Variable Expansion
    - [HW4: Multi-threaded Line Processor](#hw4-multi-threaded-line-processor)
        - Thread Pipeline
        - Buffer Management
        - Producer-Consumer Implementation
    - [HW5: One-Time Pad System](#hw5-one-time-pad-encryption)
        - Encryption Server/Client
        - Decryption Server/Client
        - Key Generation
        - Network Programming

## Overview
This repository contains 5 programming assignments (HW1-HW5) implementing various system programming concepts in C and Rust.

## Assignments

### HW1: Movie Data Processing
#### Implementation Details:
- **C Version:**
  - Parses CSV files using custom string manipulation
  - Implements linked list data structure for movie records
  - Features menu-driven interface for searching/filtering movies
- **Rust Version:**
  - Uses `csv` crate for file parsing
  - Leverages HashMap for organizing movie data
  - Implements same functionality with Rust's memory safety features

### HW2: File Directory Processing
#### Implementation Details:
- **C Version:**
  - Uses system calls (`mkdir`, `chmod`, `stat`)
  - Creates directory structure by year
  - Sets file permissions to rw-r----- (640)
  - Handles dynamic memory for file names and paths
- **Rust Version:**
  - Uses `std::fs` for file operations
  - Implements same functionality with Rust's file system APIs
  - Handles permissions using Rust's cross-platform abstractions

### HW3: Small Shell (smallsh)
#### Key Features:
- Implements built-in commands (cd, status, exit)
- Handles foreground/background processes using `fork()` and `exec()`
- Implements signal handling (SIGINT, SIGTSTP)
- Supports I/O redirection using `dup2()`
- Variable expansion for $$ to PID
- Uses linked list to track background processes

### HW4: Multi-threaded Line Processor
#### Thread Pipeline:
1. Input Thread: Reads stdin until "STOP"
2. Line Separator Thread: Replaces \n with spaces
3. Plus Sign Thread: Converts ++ to ^
4. Output Thread: Formats 80-char lines

#### Synchronization:
- Uses pthread mutex and condition variables
- Implements producer-consumer pattern
- Three shared buffers between threads
- Thread-safe memory management

### HW5: One-Time Pad Encryption
#### Components:
- `enc_server`: Handles encryption requests
- `enc_client`: Sends plaintext for encryption
- `dec_server`: Handles decryption requests
- `dec_client`: Sends ciphertext for decryption
- `keygen`: Generates random keys

#### Features:
- Socket programming for client-server communication
- One-time pad encryption algorithm
- Error handling for bad characters/connections
- Key validation and length checking
- Concurrent server design

## Build Instructions
See individual homework directories for specific compilation and usage instructions.