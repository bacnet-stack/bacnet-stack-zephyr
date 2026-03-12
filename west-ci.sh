#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
WORKSPACE_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

source "$SCRIPT_DIR/ci-debug-modules.sh"
register_ci_module_debug_trap "west-ci.sh" "$SCRIPT_DIR" "$WORKSPACE_DIR"

# setup our build environment
cd "$WORKSPACE_DIR"

echo "$PWD"
ls -al "$SCRIPT_DIR"
west --version
west init -l --mf "$SCRIPT_DIR/west-ci.yml" .
west update > /dev/null 2>&1
