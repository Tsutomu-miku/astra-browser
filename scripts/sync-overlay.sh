#!/usr/bin/env bash
# Sync the Astra overlay into a Chromium source tree.
#
# Source:      chromium/astra/ (in this repo)
# Destination: <chromium-src>/astra/
#
# Usage:
#   ./scripts/sync-overlay.sh [options]
#
# Options:
#   --help                 Show this help message.
#   --dry-run              Show what would be copied without modifying anything.
#   --chromium-src <path>  Path to Chromium source tree.
#   --watch                Watch for changes and sync continuously
#                          (uses fswatch on macOS, inotifywait on Linux).
#   --check                Only verify overlay is in sync, exit non-zero if not.
#
# Environment:
#   PROJECT_ROOT   Optional override for the project root directory.
#   CHROMIUM_SRC   Path to the Chromium checkout (default: ./chromium/src).

set -euo pipefail

# ---------------------------------------------------------------------------
# Color output helpers
# ---------------------------------------------------------------------------
if [[ -t 1 ]]; then
  RED='\033[0;31m'
  GREEN='\033[0;32m'
  YELLOW='\033[1;33m'
  BLUE='\033[0;34m'
  BOLD='\033[1m'
  RESET='\033[0m'
else
  RED=''
  GREEN=''
  YELLOW=''
  BLUE=''
  BOLD=''
  RESET=''
fi

info()    { echo -e "${BLUE}[INFO]${RESET} $*"; }
warn()    { echo -e "${YELLOW}[WARN]${RESET} $*" >&2; }
error()   { echo -e "${RED}[ERROR]${RESET} $*" >&2; }
success() { echo -e "${GREEN}[OK]${RESET} $*"; }

# ---------------------------------------------------------------------------
# Paths & defaults
# ---------------------------------------------------------------------------
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="${PROJECT_ROOT:-$(cd "$SCRIPT_DIR/.." && pwd)}"
CHROMIUM_SRC="${CHROMIUM_SRC:-$PROJECT_ROOT/chromium/src}"
OVERLAY_SRC="$PROJECT_ROOT/chromium/astra"
OVERLAY_DEST="$CHROMIUM_SRC/astra"

DRY_RUN=false
WATCH_MODE=false
CHECK_ONLY=false

# ---------------------------------------------------------------------------
# Argument parsing
# ---------------------------------------------------------------------------
usage() {
  cat <<EOF
$(basename "$0") — Sync Astra overlay into Chromium source tree.

Usage:
  $(basename "$0") [options]

Options:
  --help                 Show this help message.
  --dry-run              Show what would be copied/removed, don't modify files.
  --chromium-src <path>  Path to Chromium source tree
                         (default: ./chromium/src).
  --watch                Watch for changes and sync continuously.
  --check                Only verify overlay is in sync, exit non-zero if not.

Environment variables:
  CHROMIUM_SRC  Path to Chromium checkout (default: ./chromium/src).
  PROJECT_ROOT  Override project root directory.

Platform support:
  macOS, Linux, Windows (Git Bash).

Examples:
  $(basename "$0")
  $(basename "$0") --dry-run
  $(basename "$0") --chromium-src /path/to/chromium/src
  $(basename "$0") --watch
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --help)
      usage
      exit 0
      ;;
    --dry-run)
      DRY_RUN=true
      shift
      ;;
    --chromium-src)
      if [[ $# -lt 2 ]]; then
        error "--chromium-src requires an argument."
        exit 1
      fi
      CHROMIUM_SRC="$2"
      OVERLAY_DEST="$CHROMIUM_SRC/astra"
      shift 2
      ;;
    --watch)
      WATCH_MODE=true
      shift
      ;;
    --check)
      CHECK_ONLY=true
      shift
      ;;
    *)
      error "Unknown option: $1"
      echo ""
      usage
      exit 1
      ;;
  esac
done

# ---------------------------------------------------------------------------
# Pre-flight checks
# ---------------------------------------------------------------------------
preflight() {
  if [[ ! -d "$OVERLAY_SRC" ]]; then
    error "Astra overlay not found at $OVERLAY_SRC"
    error "Make sure you're running from the project root."
    exit 1
  fi

  if [[ ! -d "$CHROMIUM_SRC" ]]; then
    error "Chromium checkout not found at $CHROMIUM_SRC"
    error "Run: ./scripts/chromium-bootstrap.sh"
    exit 1
  fi
}

# ---------------------------------------------------------------------------
# Detect copy method (rsync or cp)
# ---------------------------------------------------------------------------
detect_sync_method() {
  if command -v rsync >/dev/null 2>&1; then
    echo "rsync"
  else
    echo "cp"
  fi
}

SYNC_METHOD="$(detect_sync_method)"

# ---------------------------------------------------------------------------
# Perform sync and collect summary
# ---------------------------------------------------------------------------
do_sync() {
  local dry_run_flag=""
  local itemize_flag=""
  if $DRY_RUN || $CHECK_ONLY; then
    dry_run_flag="--dry-run"
    itemize_flag="--itemize-changes"
  fi

  mkdir -p "$OVERLAY_DEST"

  local -a changes=()
  local copied=0
  local removed=0
  local unchanged=0

  if [[ "$SYNC_METHOD" == "rsync" ]]; then
    # Use rsync for proper change detection.
    # -a: archive mode (recursive, preserve perms/times/etc.)
    # -c: use checksums instead of mtime/size
    # --delete: remove files in dest that don't exist in source
    # --exclude: skip .git, .DS_Store, etc.
    mapfile -t changes < <(
      rsync -ac --delete $dry_run_flag $itemize_flag \
        --exclude='.git' \
        --exclude='.DS_Store' \
        --exclude='*.swp' \
        --exclude='*~' \
        "$OVERLAY_SRC/" "$OVERLAY_DEST/" 2>/dev/null || true
    )

    # Count changes from itemized output.
    for change in "${changes[@]}"; do
      local code="${change:0:1}"
      case "$code" in
        '<'|'c'|'f') ((copied++)) ;;
        '*')
          if [[ "$change" == *deleting* ]]; then
            ((removed++))
          fi
          ;;
        'd')
          # Directory — check if it's a deletion or creation
          if [[ "$change" == *deleting* ]]; then
            ((removed++))
          fi
          ;;
        '.') ((unchanged++)) ;;
        *) ((copied++)) ;;
      esac
    done

    # When not dry-run, rsync doesn't show unchanged files in itemize.
    # We need a separate count for dry-run display purposes.
    if ! $DRY_RUN && ! $CHECK_ONLY; then
      # Count total source files for context.
      local total_src
      total_src="$(find "$OVERLAY_SRC" -type f -not -name '.DS_Store' | wc -l | tr -d ' ')"
      unchanged="$(( total_src - copied - removed ))"
    fi

  else
    # Fallback: cp-based sync (no rsync, e.g. minimal Windows Git Bash).
    # Copy all files, then remove orphaned files in dest.
    local total_src
    total_src="$(find "$OVERLAY_SRC" -type f -not -name '.DS_Store' | wc -l | tr -d ' ')"

    if $DRY_RUN || $CHECK_ONLY; then
      # For dry-run with cp, we do a simple diff-based comparison.
      local diff_output=""
      if command -v diff >/dev/null 2>&1; then
        diff_output="$(diff -rq "$OVERLAY_SRC" "$OVERLAY_DEST" 2>/dev/null || true)"
        if [[ -n "$diff_output" ]]; then
          changes=($(echo "$diff_output" | wc -l))
          copied="${#changes[@]}"
        else
          copied=0
        fi
      else
        warn "Neither rsync nor diff available — can't do dry-run check."
        warn "Will copy all files."
        copied="$total_src"
      fi
      removed=0
      unchanged=0
    else
      # Actual copy with cp.
      cp -R "$OVERLAY_SRC/"* "$OVERLAY_DEST/"
      # Preserve permissions on macOS/Linux.
      if command -v chmod >/dev/null 2>&1 && [[ "$(uname -s)" != MINGW* ]]; then
        # Attempt to mirror perms from source — best effort.
        find "$OVERLAY_SRC" -type f -exec sh -c '
          src="$1"; dest="$2/${1#$3}";
          if [ -f "$dest" ]; then
            chmod --reference="$src" "$dest" 2>/dev/null || true
          fi
        ' _ {} "$OVERLAY_DEST" "$OVERLAY_SRC" \; 2>/dev/null || true
      fi
      copied="$total_src"
      removed=0
      unchanged=0
    fi
  fi

  echo "$copied" "$removed" "$unchanged"
}

# ---------------------------------------------------------------------------
# Print summary
# ---------------------------------------------------------------------------
print_summary() {
  local copied="$1"
  local removed="$2"
  local unchanged="$3"

  echo ""
  echo -e "  ${BOLD}Source:${RESET}  $OVERLAY_SRC"
  echo -e "  ${BOLD}Dest:${RESET}    $OVERLAY_DEST"
  echo -e "  ${BOLD}Method:${RESET}  $SYNC_METHOD"
  echo ""
  echo -e "  ${GREEN}Copied/updated:${RESET}  $copied file(s)"
  echo -e "  ${RED}Removed:${RESET}         $removed file(s)"
  echo -e "  ${BLUE}Unchanged:${RESET}       $unchanged file(s)"
  echo ""
}

# ---------------------------------------------------------------------------
# Watch mode
# ---------------------------------------------------------------------------
run_watch() {
  info "Watch mode: watching $OVERLAY_SRC for changes..."
  info "Press Ctrl+C to stop."

  local watch_cmd=""
  local watch_args=()

  if command -v fswatch >/dev/null 2>&1; then
    # macOS: fswatch
    watch_cmd="fswatch"
    watch_args=(-o "$OVERLAY_SRC")
  elif command -v inotifywait >/dev/null 2>&1; then
    # Linux: inotifywait
    watch_cmd="inotifywait"
    watch_args=(-mrq -e modify,create,delete,move "$OVERLAY_SRC")
  else
    error "No file watcher found."
    error "  macOS:   brew install fswatch"
    error "  Linux:   apt-get install inotify-tools"
    error "  Windows: Not supported in watch mode."
    exit 1
  fi

  info "Using $watch_cmd for file watching."
  echo ""

  # Initial sync.
  local results
  results="$(do_sync)"
  local copied removed unchanged
  read -r copied removed unchanged <<< "$results"
  print_summary "$copied" "$removed" "$unchanged"
  success "Initial sync complete."

  # Watch loop.
  if [[ "$watch_cmd" == "fswatch" ]]; then
    "$watch_cmd" "${watch_args[@]}" | while read -r _event; do
      echo ""
      info "Change detected, re-syncing..."
      results="$(do_sync)"
      read -r copied removed unchanged <<< "$results"
      print_summary "$copied" "$removed" "$unchanged"
    done
  else
    # inotifywait mode: each line is an event.
    "$watch_cmd" "${watch_args[@]}" | while read -r _dir _action _file; do
      echo ""
      info "Change detected ($_action $_file), re-syncing..."
      results="$(do_sync)"
      read -r copied removed unchanged <<< "$results"
      print_summary "$copied" "$removed" "$unchanged"
    done
  fi
}

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------
preflight

if $WATCH_MODE; then
  run_watch
  exit 0
fi

# One-shot sync.
if $CHECK_ONLY; then
  info "Checking overlay sync status..."
elif $DRY_RUN; then
  info "Dry-run — no files will be modified."
else
  info "Syncing Astra overlay..."
fi

results="$(do_sync)"
copied="$(echo "$results" | awk '{print $1}')"
removed="$(echo "$results" | awk '{print $2}')"
unchanged="$(echo "$results" | awk '{print $3}')"

print_summary "$copied" "$removed" "$unchanged"

if $CHECK_ONLY; then
  local_changes=$(( copied + removed ))
  if [[ "$local_changes" -gt 0 ]]; then
    error "Overlay is OUT OF SYNC."
    error "Run: ./scripts/sync-overlay.sh"
    exit 1
  else
    success "Overlay is in sync."
    exit 0
  fi
fi

if $DRY_RUN; then
  info "Dry-run complete. No files modified."
else
  success "Overlay synced."
fi
