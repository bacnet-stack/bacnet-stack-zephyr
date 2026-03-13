#!/bin/bash
# Set the path to the twister executable
TWISTER_EXE=""

# Set workspace virtual environment if available
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
WORKSPACE_VENV="$SCRIPT_DIR/../.venv"
WORKSPACE_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
TWISTER_EXE="$WORKSPACE_DIR/zephyr/scripts/twister"

if [ -x "$WORKSPACE_VENV/bin/python3" ]; then
    VENV_SITE="$($WORKSPACE_VENV/bin/python3 -c 'import site; print(site.getsitepackages()[0])')"
    export PATH="$WORKSPACE_VENV/bin:$PATH"
    export PYTHONPATH="$VENV_SITE${PYTHONPATH:+:$PYTHONPATH}"
fi

# Set the path to the test cases directory
TEST_CASES_DIR="$SCRIPT_DIR/zephyr/tests"

# Set the output directory for test results
OUTPUT_DIR="$SCRIPT_DIR/twister-out.unit_testing"

# Set platform to unit testing to avoid building for ALL platforms
TWISTER_PLATFORM="unit_testing"

# Remove the output directory
rm -rf "$OUTPUT_DIR"

# Run twister with the specified test cases and output directory
"$TWISTER_EXE" -O "$OUTPUT_DIR" -p "$TWISTER_PLATFORM" -T "$TEST_CASES_DIR"

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
echo "Twister unit_testing output cleanup completed successfully."

# Check if twister ran successfully
if [ $? -eq 0 ]; then
    echo "Twister unit_testing completed successfully."
else
    echo "Twister unit_testing failed."
    exit 1
fi
