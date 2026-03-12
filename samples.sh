#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
WORKSPACE_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

# Set the path to the twister executable
TWISTER_EXE="$WORKSPACE_DIR/zephyr/scripts/twister"

# Set the path to the test cases directory
TEST_CASES_DIR="$SCRIPT_DIR/zephyr/samples"

# Set the output directory for test results
OUTPUT_DIR="$SCRIPT_DIR/twister-out.samples"

# Remove the output directory
rm -rf "$OUTPUT_DIR"

# Run twister with the specified test cases and output directory
"$TWISTER_EXE" -O "$OUTPUT_DIR" -T "$TEST_CASES_DIR"

# twister output directory cleanup files we do not archive
find "$OUTPUT_DIR" -name 'CMakeFiles' -exec rm -rf {} \; 2>/dev/null
find "$OUTPUT_DIR" -name 'modules' -exec rm -rf {} \; 2>/dev/null
find "$OUTPUT_DIR" -name 'app' -exec rm -rf \
    '{}/../zephyr/arch
    {}/../zephyr/boards
    {}/../zephyr/cmake
    {}/../zephyr/CMakeFiles
    {}/../zephyr/dev_graph.dot
    {}/../zephyr/drivers
    {}/../zephyr/dts.cmake
    {}/../zephyr/edt.pickle
    {}/../zephyr/include
    {}/../zephyr/isrList.bin
    {}/../zephyr/kconfig
    {}/../zephyr/kernel
    {}/../zephyr/lib
    {}/../zephyr/libzephyr.a
    {}/../zephyr/misc
    {}/../zephyr/modules
    {}/../zephyr/soc
    {}/../zephyr/subsys
    {}/../Makefile
    {}/../Kconfig
    {}/../cmake_install.cmake
    {}/../CMakeCache.txt' \; 2>/dev/null
find "$OUTPUT_DIR" -name 'app' -exec rm -rf '{}' \; 2>/dev/null
echo "Twister output cleanup completed successfully."

# Check if twister ran successfully
if [ $? -eq 0 ]; then
    echo "Twister testing completed successfully."
else
    echo "Twister testing failed."
    exit 1
fi
