#!/bin/bash

# Set the path to the twister executable
TWISTER_EXE="../zephyr/scripts/twister"

# Set the path to the test cases directory
TEST_CASES_DIR="zephyr/samples"

# Set the output directory for test results
OUTPUT_DIR="twister-out.samples"

# Remove the output directory
rm -rf "$OUTPUT_DIR"

# Run twister with the specified test cases and output directory
"$TWISTER_EXE" -O "$OUTPUT_DIR" -T "$TEST_CASES_DIR"

# twister output directory cleanup
find $OUTPUT_DIR -name 'modules' -exec rm -rf {} \;
find $OUTPUT_DIR -name 'app' -exec rm -rf \
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
    {}/../CMakeCache.txt' \;
find $OUTPUT_DIR -name 'app' -exec rm -rf '{}' \;
echo "Twister output cleanup completed successfully."

# Check if twister ran successfully
if [ $? -eq 0 ]; then
    echo "Twister testing completed successfully."
else
    echo "Twister testing failed."
    exit 1
fi
