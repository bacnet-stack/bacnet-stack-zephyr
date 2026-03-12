#!/bin/bash

ci_module_debug_on_failure() {
    local exit_code=$?
    if [ "$exit_code" -eq 0 ]; then
        return
    fi

    echo "== CI debug: ${CI_DEBUG_SCRIPT_NAME:-ci-script} failure context =="
    echo "exit_code=$exit_code"
    echo "pwd=$PWD"
    echo "script_dir=${CI_DEBUG_SCRIPT_DIR:-$PWD}"
    echo "workspace_dir=${CI_DEBUG_WORKSPACE_DIR:-$PWD}"
    echo "ZEPHYR_MODULES=${ZEPHYR_MODULES-<unset>}"
    echo "ZEPHYR_EXTRA_MODULES=${ZEPHYR_EXTRA_MODULES-<unset>}"

    if command -v west >/dev/null 2>&1; then
        echo "== west topdir =="
        west topdir || true
        echo "== west list (name:path) =="
        west list -f '{name}:{path}' || true
    fi

    echo "== zephyr/module.yml files with name fields =="
    find "${CI_DEBUG_WORKSPACE_DIR:-$PWD}" -path '*/zephyr/module.yml' -type f -print0 2>/dev/null | while IFS= read -r -d '' module_file; do
        module_name=$(grep -E '^[[:space:]]*name:[[:space:]]*' "$module_file" | head -n1 | sed -E 's/^[[:space:]]*name:[[:space:]]*//') || true
        echo "$module_file -> ${module_name:-<no-name-field>}"
    done
}

register_ci_module_debug_trap() {
    CI_DEBUG_SCRIPT_NAME="${1:-ci-script}"
    CI_DEBUG_SCRIPT_DIR="${2:-$PWD}"
    CI_DEBUG_WORKSPACE_DIR="${3:-$PWD}"
    trap ci_module_debug_on_failure EXIT
}
