#!/usr/bin/env bash
# Build Astra-branded Chromium from a direct Chromium checkout.
#
# Usage: ./scripts/build-chromium.sh [Debug|Release]
#
# Environment:
#   DEPOT_TOOLS_DIR   Path to depot_tools (default: ./third_party/depot_tools).
#   CHROMIUM_SRC      Path to Chromium checkout (default: ./chromium/src).
#   OUT_DIR_NAME      Output directory name (default: astra_Debug / astra_Release).
#   TARGET            Ninja target to build (default: chrome).

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
DEPOT_TOOLS_DIR="${DEPOT_TOOLS_DIR:-$PROJECT_ROOT/third_party/depot_tools}"
CHROMIUM_SRC="${CHROMIUM_SRC:-$PROJECT_ROOT/chromium/src}"

BUILD_TYPE="${1:-Debug}"
TARGET="${TARGET:-chrome}"

case "$BUILD_TYPE" in
  Debug)
    OUT_DIR_NAME="${OUT_DIR_NAME:-astra_Debug}"
    ;;
  Release)
    OUT_DIR_NAME="${OUT_DIR_NAME:-astra_Release}"
    ;;
  *)
    echo "ERROR: Unknown build type: $BUILD_TYPE" >&2
    echo "       Usage: $0 [Debug|Release]" >&2
    exit 1
    ;;
esac

OUT_DIR="$CHROMIUM_SRC/out/$OUT_DIR_NAME"

echo "=== Astra Chromium build ==="
echo "  build type:  $BUILD_TYPE"
echo "  out dir:     $OUT_DIR"
echo "  target:      $TARGET"
echo "  chromium:    $CHROMIUM_SRC"
echo ""

# ---------------------------------------------------------------------------
# Pre-flight checks
# ---------------------------------------------------------------------------

if [[ ! -d "$CHROMIUM_SRC" ]]; then
  echo "ERROR: Chromium checkout not found at $CHROMIUM_SRC" >&2
  echo "       Run: ./scripts/chromium-bootstrap.sh" >&2
  exit 1
fi

export PATH="$DEPOT_TOOLS_DIR:$PATH"

if ! command -v gn >/dev/null 2>&1; then
  echo "ERROR: gn not found. Is depot_tools in PATH?" >&2
  echo "       DEPOT_TOOLS_DIR=$DEPOT_TOOLS_DIR" >&2
  exit 1
fi

# Validate the overlay is synced before building.
echo "[1/3] Checking Astra overlay sync..."
"$SCRIPT_DIR/sync-chromium-overlay.sh" --check || {
  echo ""
  echo "ERROR: Overlay out of sync. Run ./scripts/sync-chromium-overlay.sh first." >&2
  exit 1
}

# ---------------------------------------------------------------------------
# GN args
# ---------------------------------------------------------------------------

if [[ "$BUILD_TYPE" == "Release" ]]; then
  GN_ARGS=(
    'is_debug=false'
    'is_component_build=false'
    'symbol_level=1'
    'proprietary_codecs=true'
    'ffmpeg_branding="Chrome"'
    'is_astra_branded=true'
  )
else
  GN_ARGS=(
    'is_debug=true'
    'is_component_build=true'
    'symbol_level=1'
    'enable_nacl=false'
    'is_astra_branded=true'
  )
fi

# ---------------------------------------------------------------------------
# gn gen
# ---------------------------------------------------------------------------

echo "[2/3] Running gn gen..."
(
  cd "$CHROMIUM_SRC"
  gn gen "$OUT_DIR" --args="${GN_ARGS[*]}"
)

# ---------------------------------------------------------------------------
# autoninja
# ---------------------------------------------------------------------------

echo "[3/3] Building with autoninja..."
(
  cd "$CHROMIUM_SRC"
  autoninja -C "$OUT_DIR" "$TARGET"
)

echo ""
echo "=== Build complete ==="
echo "  Output: $OUT_DIR/$TARGET"
echo ""
