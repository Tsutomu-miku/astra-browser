#!/usr/bin/env bash
#
# setup-cef.sh — Download and set up CEF (Chromium Embedded Framework)
#
# Usage: ./scripts/setup-cef.sh [version] [arch]
#

set -e

CEF_VERSION="${1:-144.0.27}"
CEF_ARCH="${2:-macosarm64}"
CEF_COMMIT="3fae261"
CHROMIUM_VERSION="144.0.7559.254"

PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
THIRD_PARTY_DIR="$PROJECT_ROOT/third_party"
CEF_DIR="$THIRD_PARTY_DIR/cef"

# Construct download URL (Spotify CDN builds)
CEF_FILENAME="cef_binary_${CEF_VERSION}+g${CEF_COMMIT}+chromium-${CHROMIUM_VERSION}_${CEF_ARCH}"
CEF_ARCHIVE="${CEF_FILENAME}.tar.bz2"
CEF_URL="https://cef-builds.spotifycdn.com/${CEF_ARCHIVE}"

echo "=== Astra Browser — CEF Setup ==="
echo "  Version:  ${CEF_VERSION}"
echo "  Arch:     ${CEF_ARCH}"
echo "  URL:      ${CEF_URL}"
echo "  Target:   ${CEF_DIR}"
echo ""

# Create directories
mkdir -p "$THIRD_PARTY_DIR"

# Download if not already present
if [ ! -d "$CEF_DIR" ]; then
    if [ ! -f "$THIRD_PARTY_DIR/$CEF_ARCHIVE" ]; then
        echo "Downloading CEF..."
        curl -L --progress-bar -o "$THIRD_PARTY_DIR/$CEF_ARCHIVE" "$CEF_URL"
    else
        echo "Archive already exists, skipping download."
    fi

    echo "Extracting CEF..."
    tar -xjf "$THIRD_PARTY_DIR/$CEF_ARCHIVE" -C "$THIRD_PARTY_DIR"
    mv "$THIRD_PARTY_DIR/$CEF_FILENAME" "$CEF_DIR"
    echo "Extracted to $CEF_DIR"
else
    echo "CEF already set up at $CEF_DIR"
fi

# Verify
if [ -f "$CEF_DIR/CMakeLists.txt" ]; then
    echo ""
    echo "✓ CEF setup complete."
    echo "  Build with: cmake -B build -G Xcode && cmake --build build --config Release"
else
    echo ""
    echo "✗ CEF setup failed — CMakeLists.txt not found"
    exit 1
fi
