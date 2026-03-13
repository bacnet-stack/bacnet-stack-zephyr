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
TEST_CASES_DIR="$SCRIPT_DIR/zephyr/tests/subsys"

# Set the output directory for test results
OUTPUT_DIR="$SCRIPT_DIR/twister-out.subsys"

# Set platform to native_sim to run specific simulation tests
TWISTER_PLATFORM="native_sim"

cleanup_twister_output_dir() {
    local output_dir="$1"

    find "$output_dir" -name 'CMakeFiles' -exec rm -rf {} \; 2>/dev/null
    find "$output_dir" -name 'modules' -exec rm -rf {} \; 2>/dev/null
    find "$output_dir" -name 'app' -exec rm -rf \
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
    find "$output_dir" -name 'app' -exec rm -rf '{}' \; 2>/dev/null
}

# Remove the output directory
rm -rf "$OUTPUT_DIR"

# Run twister for native_sim-only functional subsys tests
"$TWISTER_EXE" -O "$OUTPUT_DIR" -p "$TWISTER_PLATFORM" -T "$TEST_CASES_DIR"
TWISTER_RC=$?

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
echo "Twister native_sim output cleanup completed successfully."

# Check if twister ran successfully
if [ $TWISTER_RC -eq 0 ]; then
    echo "Twister native_sim completed successfully."
else
    echo "Twister native_sim failed."
    exit 1
fi
