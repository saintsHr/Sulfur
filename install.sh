#!/bin/bash
set -e

ICON_DIR="$HOME/.local/share/icons/hicolor/48x48/mimetypes"
MIME_DIR="$HOME/.local/share/mime/packages"
BIN_DIR="$HOME/.local/bin"
INDEX_THEME="$HOME/.local/share/icons/hicolor/index.theme"

mkdir -p "$ICON_DIR"
mkdir -p "$MIME_DIR"
mkdir -p "$BIN_DIR"

if [ ! -f assets/sulfur.png ]; then
    echo "Error: icon assets/sulfur.png not found!"
    exit 1
fi

cat > "$MIME_DIR/slfr.xml" <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<mime-info xmlns="http://www.freedesktop.org/standards/shared-mime-info">
  <mime-type type="image/x-slfr">
    <comment>Sulfur source file</comment>
    <glob pattern="*.slfr"/>
  </mime-type>
</mime-info>
EOF

update-mime-database "$HOME/.local/share/mime"

cp assets/sulfur.png "$ICON_DIR/image-x-slfr.png"

if [ ! -f "$INDEX_THEME" ]; then
    cat > "$INDEX_THEME" <<EOF
[Icon Theme]
Name=Hicolor
Comment=Fallback icon theme
Directories=48x48/mimetypes

[48x48/mimetypes]
Size=48
Context=Mimetypes
Type=Fixed
EOF
fi

gtk-update-icon-cache "$HOME/.local/share/icons/hicolor/"

if [ ! -f slfr ]; then
    echo "Error: program 'slfr' not found in the current directory!"
    exit 1
fi

cp slfr "$BIN_DIR/"
chmod +x "$BIN_DIR/slfr"

if ! echo "$PATH" | grep -q "$HOME/.local/bin"; then
    SHELL_RC=""
    if [ -n "$BASH_VERSION" ]; then
        SHELL_RC="$HOME/.bashrc"
    elif [ -n "$ZSH_VERSION" ]; then
        SHELL_RC="$HOME/.zshrc"
    fi

    if [ -n "$SHELL_RC" ]; then
        echo 'export PATH="$HOME/.local/bin:$PATH"' >> "$SHELL_RC"
        echo "Added ~/.local/bin to PATH. Please restart the terminal."
    else
        echo "Could not detect your shell. Please add ~/.local/bin to PATH manually."
    fi
fi

echo "Installation complete! .slfr files now have an icon and 'slfr' is in your PATH."
