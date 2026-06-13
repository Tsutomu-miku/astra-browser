#!/usr/bin/env bash
# Sync the Astra overlay (chromium/astra/) into a Chromium checkout.
#
# Usage: ./scripts/sync-chromium-overlay.sh [--check]
#   --check   Only verify that the overlay is in sync, do not modify the tree.
#
# Environment:
#   PROJECT_ROOT   Optional override for the project root directory.
#   CHROMIUM_SRC   Path to the Chromium checkout (default: ./chromium/src).

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="${PROJECT_ROOT:-$(cd "$SCRIPT_DIR/.." && pwd)}"
CHROMIUM_SRC="${CHROMIUM_SRC:-$PROJECT_ROOT/chromium/src}"
OVERLAY_SRC="$PROJECT_ROOT/chromium/astra"
OVERLAY_DEST="$CHROMIUM_SRC/astra"

CHECK_ONLY=false
if [[ "${1:-}" == "--check" ]]; then
  CHECK_ONLY=true
fi

if [[ ! -d "$OVERLAY_SRC" ]]; then
  echo "ERROR: Astra overlay not found at $OVERLAY_SRC" >&2
  exit 1
fi

if [[ ! -d "$CHROMIUM_SRC" ]]; then
  echo "ERROR: Chromium checkout not found at $CHROMIUM_SRC" >&2
  echo "       Run: ./scripts/chromium-bootstrap.sh" >&2
  exit 1
fi

if $CHECK_ONLY; then
  # Use rsync dry-run + --itemize-changes to detect drift.
  # -a: archive mode (recursive, preserve perms/times/etc.)
  # -c: use checksums instead of mtime/size
  # -n: dry run
  # -i: itemize changes
  # --delete: also report files that would be deleted
  mapfile -t changes < <(rsync -acni --delete "$OVERLAY_SRC/" "$OVERLAY_DEST/" || true)

  if [[ ${#changes[@]} -eq 0 ]]; then
    echo "Overlay is in sync."
    exit 0
  fi

  echo "ERROR: Astra overlay is out of sync with Chromium checkout." >&2
  echo "       Run: ./scripts/sync-chromium-overlay.sh" >&2
  echo "" >&2
  echo "  Files that differ:" >&2
  for change in "${changes[@]}"; do
    echo "    $change" >&2
  done
  exit 1
fi

echo "=== Syncing Astra overlay ==="
echo "  source: $OVERLAY_SRC"
echo "  dest:   $OVERLAY_DEST"

mkdir -p "$OVERLAY_DEST"

rsync -ac --delete \
  --exclude='.git' \
  --exclude='.DS_Store' \
  "$OVERLAY_SRC/" "$OVERLAY_DEST/"

echo "Overlay synced."
