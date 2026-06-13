#!/usr/bin/env bash
# Bootstrap a direct Chromium checkout for Astra development.
#
# What this does:
#   1. Checks for depot_tools (clones it if missing).
#   2. Clones or updates a Chromium checkout at CHROMIUM_SRC.
#   3. Syncs the Astra overlay (chromium/astra/) into chromium/src/astra/.
#   4. Prints patch-point instructions (manual — patches are not auto-applied).
#
# Usage: ./scripts/chromium-bootstrap.sh
#
# Environment:
#   DEPOT_TOOLS_DIR   Path to depot_tools (default: ./third_party/depot_tools).
#   CHROMIUM_SRC      Path to Chromium checkout (default: ./chromium/src).
#   CHROMIUM_BRANCH   Optional branch to check out (default: main).

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
DEPOT_TOOLS_DIR="${DEPOT_TOOLS_DIR:-$PROJECT_ROOT/third_party/depot_tools}"
CHROMIUM_SRC="${CHROMIUM_SRC:-$PROJECT_ROOT/chromium/src}"
CHROMIUM_PARENT="$(dirname "$CHROMIUM_SRC")"
CHROMIUM_BRANCH="${CHROMIUM_BRANCH:-main}"

echo "=== Astra direct Chromium bootstrap ==="
echo "depot_tools:   $DEPOT_TOOLS_DIR"
echo "chromium src:  $CHROMIUM_SRC"
echo "chromium ref:  $CHROMIUM_BRANCH"
echo ""

# ---------------------------------------------------------------------------
# 1. depot_tools
# ---------------------------------------------------------------------------
ensure_depot_tools() {
  if [[ -d "$DEPOT_TOOLS_DIR" ]]; then
    echo "[1/4] depot_tools found, updating..."
    (
      cd "$DEPOT_TOOLS_DIR"
      git pull --ff-only origin main 2>/dev/null || true
    )
  else
    echo "[1/4] Cloning depot_tools..."
    mkdir -p "$(dirname "$DEPOT_TOOLS_DIR")"
    git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git \
      "$DEPOT_TOOLS_DIR"
  fi
  export PATH="$DEPOT_TOOLS_DIR:$PATH"

  # Verify depot_tools commands are available.
  if ! command -v gclient >/dev/null 2>&1; then
    echo "ERROR: gclient not found after depot_tools setup." >&2
    exit 1
  fi
  echo "      depot_tools ready."
}

# ---------------------------------------------------------------------------
# 2. Chromium checkout
# ---------------------------------------------------------------------------
ensure_chromium_checkout() {
  mkdir -p "$CHROMIUM_PARENT"

  if [[ -d "$CHROMIUM_SRC" ]]; then
    echo "[2/4] Chromium checkout found, syncing..."

    # Verify .gclient exists in the parent directory.  gclient needs this
    # file to manage dependencies.  If the checkout was created by
    # `fetch chromium` it will be present; a bare git clone won't have it.
    if [[ ! -f "$CHROMIUM_PARENT/.gclient" ]]; then
      echo "WARNING: No .gclient file found in $CHROMIUM_PARENT" >&2
      echo "         gclient sync may fail.  If your checkout was created" >&2
      echo "         by 'fetch chromium', .gclient should be present." >&2
      echo "         To fix: run 'gclient config' or re-bootstrap with fetch." >&2
      echo "" >&2
    fi

    (
      cd "$CHROMIUM_SRC"
      git checkout "$CHROMIUM_BRANCH" 2>/dev/null || \
        echo "      (using current branch, $CHROMIUM_BRANCH not found locally)"
    )
  else
    echo "[2/4] Cloning Chromium (this may take a while)..."
    (
      cd "$CHROMIUM_PARENT"
      fetch --nohooks chromium
    )
    (
      cd "$CHROMIUM_SRC"
      git checkout "$CHROMIUM_BRANCH"
    )
  fi

  # Run gclient sync to pull dependencies.
  echo "      Running gclient sync..."
  (
    cd "$CHROMIUM_SRC"
    gclient sync
  )
  echo "      Chromium checkout ready."
}

# ---------------------------------------------------------------------------
# 3. Astra overlay
# ---------------------------------------------------------------------------
sync_overlay() {
  echo "[3/4] Syncing Astra overlay..."
  "$SCRIPT_DIR/sync-chromium-overlay.sh"
}

# ---------------------------------------------------------------------------
# 4. Patch point instructions
# ---------------------------------------------------------------------------
print_patch_instructions() {
  echo "[4/4] Chromium patch points (manual):"
  echo ""
  echo "  The Astra overlay is at chromium/src/astra/. To build it, you need to"
  echo "  register a few entry points in the Chromium source tree. Detailed"
  echo "  instructions live in chromium/src/astra/patches/."
  echo ""
  echo "  Summary of required patches:"
  echo ""
  echo "  1. chrome/browser/chrome_browser_main.cc"
  echo "     -> Register AstraBrowserMainExtraParts"
  echo "        File: astra/patches/0001-browser-main-extra-parts.md"
  echo ""
  echo "  2. chrome/browser/ui/views/frame/browser_view.cc"
  echo "     -> Install AstraBrowserView after BrowserView construction"
  echo "        File: astra/patches/0002-browser-view-install.md"
  echo ""
  echo "  3. chrome/browser/ui/browser_command_controller.cc"
  echo "     -> Forward Astra-only command IDs to AstraCommandDelegate"
  echo "        File: astra/patches/0003-command-forwarding.md"
  echo ""
  echo "  4. BUILD.gn / build graph"
  echo "     -> Include //astra in the build (e.g. via chrome/browser deps)"
  echo "        File: astra/patches/0004-build-gn-include.md"
  echo ""
  echo "  Patches are intentionally minimal and delegate to //astra code."
  echo "  Product logic must never live inside Chromium source files."
  echo ""
}

# ---------------------------------------------------------------------------
# Run
# ---------------------------------------------------------------------------
ensure_depot_tools
ensure_chromium_checkout
sync_overlay
print_patch_instructions

echo ""
echo "=== Bootstrap complete ==="
echo ""
echo "Next steps:"
echo "  1. Apply the Chromium source patches listed above."
echo "  2. Build: ./scripts/build-chromium.sh Debug"
echo ""
