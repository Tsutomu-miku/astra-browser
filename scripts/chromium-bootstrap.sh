#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
DEPOT_TOOLS_DIR="${DEPOT_TOOLS_DIR:-$PROJECT_ROOT/third_party/depot_tools}"
CHROMIUM_PARENT="${CHROMIUM_PARENT:-$PROJECT_ROOT/chromium}"
CHROMIUM_SRC="$CHROMIUM_PARENT/src"

echo "=== Astra direct Chromium bootstrap ==="
echo "depot_tools:  $DEPOT_TOOLS_DIR"
echo "chromium src: $CHROMIUM_SRC"

if [ ! -d "$DEPOT_TOOLS_DIR" ]; then
  git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git "$DEPOT_TOOLS_DIR"
fi

export PATH="$DEPOT_TOOLS_DIR:$PATH"
mkdir -p "$CHROMIUM_PARENT"

if [ ! -d "$CHROMIUM_SRC" ]; then
  (
    cd "$CHROMIUM_PARENT"
    fetch --nohooks chromium
  )
fi

(
  cd "$CHROMIUM_SRC"
  gclient sync
)

echo "Chromium checkout ready."
echo "Next: ./scripts/build-chromium.sh Debug"
