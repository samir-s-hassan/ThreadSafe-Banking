# Set the file name
FILE="hw1.cpp"
OUTPUT="hw1"

# Check if the file exists
if [[ ! -f "$FILE" ]]; then
  echo "Error: $FILE not found!"
  exit 1
fi

# Compile the C++ program
echo "Compiling $FILE..."
g++ -std=c++17 "$FILE" -o "$OUTPUT"
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
