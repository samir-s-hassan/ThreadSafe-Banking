#!/bin/bash

# Set the file name
FILE="hw1.cpp"
OUTPUT="hw1"

# Check if the file exists
if [[ ! -f "$FILE" ]]; then
  echo "Error: $FILE not found!"
  exit 1
fi

# Load the correct GCC module
echo "Loading GCC 11.2.0 module..."
module load gcc-11.2.0

# Verify GCC version
GCC_VERSION=$(gcc --version | head -n 1)
if [[ ! "$GCC_VERSION" =~ "11.2.0" ]]; then
  echo "Error: Failed to switch to GCC 11.2.0. Current version is: $GCC_VERSION"
  exit 1
fi
echo "Using compiler: $GCC_VERSION"

# Compile the C++ program with threading support and optimization
echo "Compiling $FILE with -O3 optimization..."
g++ -std=c++17 -pthread -O3 "$FILE" -o "$OUTPUT"
if [[ $? -ne 0 ]]; then
  echo "Compilation failed!"
  exit 1
fi

# Run the compiled program
echo "Running $OUTPUT..."
./"$OUTPUT"

# Check if the program ran successfully
if [[ $? -eq 0 ]]; then
  echo "Program executed successfully!"
else
  echo "Program execution failed!"
  exit 1
fi