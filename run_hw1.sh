#!/bin/bash

# Set the file name and output executable
FILE="hw1.cpp"
OUTPUT="hw1"

# Set the number of accounts and iterations
NUM_ACCOUNTS=3
NUM_ITERATIONS=100000

# Check if the file exists
if [[ ! -f "$FILE" ]]; then
  echo "Error: $FILE not found!"
  exit 1
fi

# Load the correct GCC module
module load gcc-11.2.0

# Verify GCC version
GCC_VERSION=$(gcc --version | head -n 1)
if [[ ! "$GCC_VERSION" =~ "11.2.0" ]]; then
  echo "Error: Failed to switch to GCC 11.2.0. Current version is: $GCC_VERSION"
  exit 1
fi
echo "Using compiler: $GCC_VERSION"

# Compile the C++ program with threading support and optimization
g++ -std=c++17 -pthread -O3 "$FILE" -o "$OUTPUT"
if [[ $? -ne 0 ]]; then
  echo "Compilation failed!"
  exit 1
fi

# Run the compiled program with different NUM_THREADS values
./"$OUTPUT" "$NUM_ACCOUNTS" 1 "$NUM_ITERATIONS"
./"$OUTPUT" "$NUM_ACCOUNTS" 2 "$NUM_ITERATIONS"
./"$OUTPUT" "$NUM_ACCOUNTS" 4 "$NUM_ITERATIONS"
./"$OUTPUT" "$NUM_ACCOUNTS" 8 "$NUM_ITERATIONS"
./"$OUTPUT" "$NUM_ACCOUNTS" 16 "$NUM_ITERATIONS"