#!/usr/bin/env bash

set -Eeuo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

MODE="${1:-debug}"
TARGET="${2:-all}"
INSTALL_PREFIX="${INSTALL_PREFIX:-/usr/local}"

usage() {
    cat <<EOF
Usage: $0 [debug|release] [all|module]

Examples:
  $0
  $0 debug all
  $0 release all
  $0 debug cli
  $0 release compiler

Environment:
  INSTALL_PREFIX  Installation prefix (default: /usr/local)
EOF
}

if [[ $# -gt 2 ]]; then
    usage
    exit 1
fi

case "$MODE" in
    debug|release)
        ;;
    *)
        echo "Error: invalid build mode '$MODE'."
        usage
        exit 1
        ;;
esac

if [[ ! "$TARGET" =~ ^[a-zA-Z0-9_-]+$ ]]; then
    echo "Error: invalid module name '$TARGET'."
    exit 1
fi

BUILD_DIR="$ROOT_DIR/build/$MODE"

if [[ ! -f "$BUILD_DIR/cmake_install.cmake" ]]; then
    echo "Error: no install configuration found for '$MODE'."
    echo "Expected: $BUILD_DIR/cmake_install.cmake"
    echo "Configure the project first."
    exit 1
fi

INSTALL_ARGS=(
    --install "$BUILD_DIR"
    --prefix "$INSTALL_PREFIX"
)

if [[ "$TARGET" != "all" ]]; then
    MODULE_DIR="$ROOT_DIR/$TARGET"

    if [[ ! -f "$MODULE_DIR/CMakeLists.txt" ]]; then
        echo "Error: module '$TARGET' not found."
        exit 1
    fi

    INSTALL_ARGS+=(--component "$TARGET")
fi

echo "==> Installing $TARGET ($MODE)"
echo "==> Prefix: $INSTALL_PREFIX"

if [[ "$EUID" -eq 0 || -w "$INSTALL_PREFIX" ]]; then
    cmake "${INSTALL_ARGS[@]}"
elif command -v sudo >/dev/null 2>&1; then
    sudo cmake "${INSTALL_ARGS[@]}"
else
    echo "Error: insufficient permissions for '$INSTALL_PREFIX'."
    echo "Set INSTALL_PREFIX to a writable directory."
    exit 1
fi

echo "==> Installation completed successfully."
