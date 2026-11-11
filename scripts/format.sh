#!/usr/bin/env bash

set -Eeuo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

if ! command -v clang-format >/dev/null 2>&1; then
    echo "Error: clang-format not found."
    exit 1
fi

echo "Root: $ROOT_DIR"
clang-format --version

mapfile -d '' FILES < <(
    find "$ROOT_DIR" \
        -type d \( -name build -o -name .git \) -prune -o \
        -type f \( -name '*.c' -o -name '*.h' \) \
        -print0
)

echo "Found ${#FILES[@]} C files."

if (( ${#FILES[@]} == 0 )); then
    echo "Error: No .c or .h files found."
    exit 1
fi

for file in "${FILES[@]}"; do
    echo "Formatting: ${file#"$ROOT_DIR"/}"
    clang-format -i "$file"
done

echo "Formatting done."
