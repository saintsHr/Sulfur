#!/usr/bin/env bash

set -euo pipefail

PREFIX="${PREFIX:-/usr/local}"

BINDIR="$PREFIX/bin"
LIBDIR="$PREFIX/lib/sulfur"
DATADIR="$PREFIX/share/sulfur"

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"

echo "Sulfur Installer"
echo

for file in \
    "$SCRIPT_DIR/bin/sulfur" \
    "$SCRIPT_DIR/bin/sulfur-compiler" \
    "$SCRIPT_DIR/lib/libsulfurstd.a" \
    "$SCRIPT_DIR/lib/libsulfurrt.a" \
    "$SCRIPT_DIR/share/std"
do
    if [[ ! -e "$file" ]]; then
        echo "Error: Missing required file: $file" >&2
        exit 1
    fi
done

if [[ ! -w "$PREFIX" ]]; then
    echo "Error: Cannot write to $PREFIX."
    echo "Try running with sudo:"
    echo "  sudo ./install.sh"
    exit 1
fi

echo "[1/4] Creating directories..."

install -d "$BINDIR"
install -d "$LIBDIR"
install -d "$DATADIR"

echo "[2/4] Installing binaries..."

install -m 755 "$SCRIPT_DIR/bin/sulfur" \
    "$BINDIR/sulfur"

install -m 755 "$SCRIPT_DIR/bin/sulfur-compiler" \
    "$BINDIR/sulfur-compiler"

echo "[3/4] Installing libraries..."

install -m 644 "$SCRIPT_DIR/lib/libsulfurstd.a" \
    "$LIBDIR/libsulfurstd.a"

install -m 644 "$SCRIPT_DIR/lib/libsulfurrt.a" \
    "$LIBDIR/libsulfurrt.a"

echo "[4/4] Installing standard library..."

rm -rf "$DATADIR/std"
cp -r "$SCRIPT_DIR/share/std" "$DATADIR/std"

echo
echo "Sulfur installed successfully!"
echo
echo "Installation paths:"
echo "    Binaries: $BINDIR"
echo "    Library:  $LIBDIR"
echo "    Share:    $DATADIR"
echo
