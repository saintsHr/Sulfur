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
    echo "  $0 debug cli"
    echo "  $0 release cli"
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
    debug|release) ;;
    *)
        echo "Error: invalid build mode '$MODE'."
        usage
        ;;
esac

BUILD_DIR="$ROOT_DIR/build/$MODE"

if [[ "$TARGET" == "all" ]]; then
    if [[ -d "$BUILD_DIR" ]]; then
        echo "==> Cleaning all ($MODE)"
        rm -rf -- "$BUILD_DIR"
    else
        echo "==> Nothing to clean ($MODE)"
    fi
else
    if [[ ! "$TARGET" =~ ^[a-zA-Z0-9_-]+$ ]]; then
        echo "Error: invalid module name '$TARGET'."
        exit 1
    fi

    MODULE_BUILD_DIR="$BUILD_DIR/$TARGET"

    if [[ -d "$MODULE_BUILD_DIR" ]]; then
        echo "==> Cleaning module: $TARGET ($MODE)"
        rm -rf -- "$MODULE_BUILD_DIR"
    else
        echo "==> Nothing to clean for module '$TARGET' ($MODE)"
    fi
fi

echo "==> Clean completed."
