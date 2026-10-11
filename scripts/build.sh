#!/usr/bin/env bash

set -e

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

MODE="debug"
TARGET="all"

usage() {
    echo "Usage: $0 [debug|release] [all|module]"
    echo
    echo "Examples:"
    echo "  $0"
    echo "  $0 debug all"
    echo "  $0 release all"
    echo "  $0 debug compiler"
    echo "  $0 release compiler"
    exit 1
}

if [[ $# -gt 2 ]]; then
    usage
fi

if [[ $# -ge 1 ]]; then
    MODE="$1"
fi

if [[ $# -ge 2 ]]; then
    TARGET="$2"
fi

case "$MODE" in
    debug)
        BUILD_TYPE="Debug"
        SANITIZERS="TRUE"
        ;;
    release)
        BUILD_TYPE="Release"
        SANITIZERS="FALSE"
        ;;
    *)
        echo "Error: invalid build mode '$MODE'."
        usage
        ;;
esac

BUILD_DIR="$ROOT_DIR/build/$MODE"

if [[ "$TARGET" == "all" ]]; then
    echo "==> Configuring all modules ($MODE)"

    cmake -S "$ROOT_DIR" -B "$BUILD_DIR" \
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
        -DENABLE_SANITIZERS="$SANITIZERS"

    echo "==> Building all modules"

    cmake --build "$BUILD_DIR" --parallel

    echo "==> Build completed successfully."
    exit 0
fi

if [[ ! "$TARGET" =~ ^[a-zA-Z0-9_-]+$ ]]; then
    echo "Error: invalid module name '$TARGET'."
    exit 1
fi

MODULE_DIR="$ROOT_DIR/$TARGET"
MODULE_BUILD_DIR="$BUILD_DIR/$TARGET"

if [[ ! -f "$MODULE_DIR/CMakeLists.txt" ]]; then
    echo "Error: module '$TARGET' not found."
    echo "Expected: $MODULE_DIR/CMakeLists.txt"
    exit 1
fi

echo "==> Configuring module: $TARGET ($MODE)"

cmake -S "$MODULE_DIR" -B "$MODULE_BUILD_DIR"
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
    -DENABLE_SANITIZERS="$SANITIZERS"

echo "==> Building module: $TARGET"

cmake --build "$MODULE_BUILD_DIR" --parallel

echo "==> Build completed successfully."
