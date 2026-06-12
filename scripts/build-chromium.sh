#!/usr/bin/env bash
set -euo pipefail

BUILD_TYPE="${1:-Debug}"
PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
DEPOT_TOOLS_DIR="${DEPOT_TOOLS_DIR:-$PROJECT_ROOT/third_party/depot_tools}"
CHROMIUM_SRC="${CHROMIUM_SRC:-$PROJECT_ROOT/chromium/src}"
OUT_DIR="$CHROMIUM_SRC/out/astra_$BUILD_TYPE"

if [ ! -d "$CHROMIUM_SRC" ]; then
  echo "Chromium checkout not found at $CHROMIUM_SRC"
  echo "Run: ./scripts/chromium-bootstrap.sh"
  exit 1
fi

export PATH="$DEPOT_TOOLS_DIR:$PATH"

rsync -a --delete "$PROJECT_ROOT/chromium/astra/" "$CHROMIUM_SRC/astra/"

if [ "$BUILD_TYPE" = "Release" ]; then
  GN_ARGS='is_debug=false is_component_build=false symbol_level=1 proprietary_codecs=true ffmpeg_branding="Chrome"'
else
  GN_ARGS='is_debug=true is_component_build=true symbol_level=1 enable_nacl=false'
fi

(
  cd "$CHROMIUM_SRC"
  gn gen "$OUT_DIR" --args="$GN_ARGS"
  autoninja -C "$OUT_DIR" chrome
)
