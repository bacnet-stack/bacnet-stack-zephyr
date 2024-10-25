#!/bin/bash

# Set the path to the twister executable
TWISTER_EXE="../zephyr/scripts/twister"

# Set the path to the test cases directory
TEST_CASES_DIR="zephyr/tests"

# Set the output directory for test results
OUTPUT_DIR="twister-out.unit_testing"

TWISTER_PLATFORM="unit_testing"

# Remove the output directory
rm -rf "$OUTPUT_DIR"

# Run twister with the specified test cases and output directory
"$TWISTER_EXE" -O "$OUTPUT_DIR" -p "$TWISTER_PLATFORM" -T "$TEST_CASES_DIR"

# Check if twister ran successfully
if [ $? -eq 0 ]; then
    echo "Twister testing completed successfully."
else
    echo "Twister testing failed."
    exit 1
fi
