#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
WORKSPACE_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

# setup our build environment
cd "$WORKSPACE_DIR"

echo "$PWD"
ls -al "$SCRIPT_DIR"
west --version
west init -l --mf west-ci.yml "$SCRIPT_DIR"
west update > /dev/null 2>&1
