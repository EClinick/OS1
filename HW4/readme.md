# Line Processor

## Description
This program processes input lines using a multi-threaded pipeline consisting of four threads:
1. **Input Thread:** Reads lines from standard input.
2. **Line Separator Thread:** Replaces line separators (`\n`) with spaces.
3. **Plus Sign Thread:** Replaces every pair of plus signs (`++`) with a caret (`^`).
4. **Output Thread:** Writes processed data as lines of exactly 80 characters to standard output.

The threads communicate using producer-consumer buffers with mutual exclusion and condition variables.

## Compilation Instructions

To compile the program, ensure you have `gcc` installed on your system. Use the following command:

```bash
gcc -pthread -o line_processor line_processor.c
```