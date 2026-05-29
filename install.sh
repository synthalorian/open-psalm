#!/usr/bin/env bash
# ──────────────────────────────────────────────
# Open Psalm — Install Script
# Builds the project and installs the global
# 'open-psalm' command so it works from anywhere.
# ──────────────────────────────────────────────
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BIN_DIR="${HOME}/.local/bin"
BINARY="${SCRIPT_DIR}/build/open-psalm"
WRAPPER="${BIN_DIR}/open-psalm"

echo "  ╔════════════════════════════════════════╗"
echo "  ║      ◈  OPEN PSALM  ◈                 ║"
echo "  ║  The Word, blazing in your terminal    ║"
echo "  ╚════════════════════════════════════════╝"
echo ""

# ── Dependencies ──────────────────────────
echo "→ Checking dependencies..."
MISSING=""
for cmd in cmake g++ sqlite3; do
    if ! command -v "$cmd" &>/dev/null; then
        echo "  ✗ $cmd not found"
        MISSING="$MISSING $cmd"
    else
        echo "  ✓ $cmd"
    fi
done

if [ -n "$MISSING" ]; then
    echo ""
    echo "Missing dependencies:$MISSING"
    echo ""
    echo "Install them:"
    echo "  Arch:   sudo pacman -S cmake gcc sqlite3 nlohmann-json"
    echo "  Debian: sudo apt install cmake g++ libsqlite3-dev nlohmann-json3-dev"
    echo "  Fedora: sudo dnf install cmake gcc-c++ sqlite-devel nlohmann-json-devel"
    exit 1
fi

# ── Build ────────────────────────────────
echo ""
echo "→ Building Open Psalm..."

cd "$SCRIPT_DIR"
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j"$(nproc)"

echo "  ✓ Build complete"

# ── Install wrapper ──────────────────────
echo ""
echo "→ Installing 'open-psalm' command..."

mkdir -p "$BIN_DIR"

cat > "$WRAPPER" << 'WRAPPER_EOF'
#!/usr/bin/env bash
# ──────────────────────────────────────────────
# open-psalm — "The Word, blazing in your terminal"
# Launcher wrapper — sets CWD to project root so
# the data/ directory is always found.
# ──────────────────────────────────────────────

PROJECT_DIR="PLACEHOLDER_PROJECT_DIR"
BINARY="$PROJECT_DIR/build/open-psalm"

# Handle "help" as a positional arg
for arg in "$@"; do
    if [ "$arg" = "help" ]; then
        exec "$BINARY" --help
    fi
done

if [ -f "$BINARY" ]; then
    cd "$PROJECT_DIR" && exec "$BINARY" "$@"
else
    echo "open-psalm: binary not found at $BINARY"
    echo "Re-run install.sh from the open-psalm project directory."
    exit 1
fi
WRAPPER_EOF

sed -i "s|PLACEHOLDER_PROJECT_DIR|$SCRIPT_DIR|" "$WRAPPER"
chmod +x "$WRAPPER"

echo "  ✓ Installed to $WRAPPER"

# ── PATH check ───────────────────────────
if [[ ":$PATH:" != *":$BIN_DIR:"* ]]; then
    echo ""
    echo "⚠  Add $BIN_DIR to your PATH:"
    echo "   echo 'export PATH=\"\$HOME/.local/bin:\$PATH\"' >> ~/.bashrc"
    echo "   source ~/.bashrc"
fi

echo ""
echo "  ╔════════════════════════════════════════╗"
echo "  ║     Installation complete!             ║"
echo "  ║     Type 'open-psalm' to begin.        ║"
echo "  ╚════════════════════════════════════════╝"
echo ""
