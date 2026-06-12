#!/usr/bin/env bash
#
# build-native.sh — Build Astra Browser (Native UI + CEF)
#
# Usage: ./scripts/build-native.sh [Debug|Release]
#

set -e

BUILD_TYPE="${1:-Release}"
PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build/native"

echo "=== Astra Browser — Native Build ==="
echo "  Build type: $BUILD_TYPE"
echo "  Build dir:  $BUILD_DIR"
echo ""

# Check for CEF
if [ ! -d "$PROJECT_ROOT/third_party/cef" ]; then
    echo "CEF not found. Run: ./scripts/setup-cef.sh"
    exit 1
fi

# Configure
echo "Configuring..."
cmake -S "$PROJECT_ROOT" \
      -B "$BUILD_DIR" \
      -G Xcode \
      -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
      -DCEF_ROOT="$PROJECT_ROOT/third_party/cef" \
      -Wno-dev

echo ""
echo "Building..."
cmake --build "$BUILD_DIR" \
      --config "$BUILD_TYPE" \
      -j$(sysctl -n hw.ncpu)

echo ""
APP_PATH="$BUILD_DIR/Release/Astra.app"
if [ -d "$APP_PATH" ]; then
    echo "✓ Build complete: $APP_PATH"
    echo ""
    echo "Run with: open $APP_PATH"
    echo "Or: $APP_PATH/Contents/MacOS/Astra"
else
    echo "Build finished (check build output above for app location)"
fi
