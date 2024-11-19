
# Makefile for HW4

# Compiler
CC = gcc

# Compiler flags
CFLAGS = -pthread -Wall -Wextra -Werror

# Target executable
TARGET = line_processor

# Source files
SRCS = line_processor.c

# Object files
OBJS = $(SRCS:.c=.o)

# Default target
all: $(TARGET)

# Link object files to create the executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Compile source files to object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up object files and the executable
clean:
	rm -f $(OBJS) $(TARGET)

# Create a zip file for submission
zip:
	zip -r HW4_clinicke.zip line_processor.c makefile tests/ README.txt