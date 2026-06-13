#!/usr/bin/env bash
# Apply Astra patches to a Chromium source tree.
#
# Patches are read from chromium/astra/patches/*.patch and applied in
# alphanumeric order.  A marker file (.astra_patches_applied) tracks which
# patches have been applied so --reverse can cleanly unapply them.
#
# Usage:
#   ./scripts/apply-patches.sh [options]
#
# Options:
#   --help              Show this help message.
#   --reverse           Unapply patches instead of applying them.
#   --check             Dry-run — verify patches would apply cleanly.
#   --list              List all patches with their current status.
#   --chromium-src <p>  Path to Chromium source tree.
#   --patch-dir <p>     Path to patch directory (default: chromium/astra/patches).
#   --force             Apply even if already applied marker exists.
#
# Environment:
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
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
CHROMIUM_SRC="${CHROMIUM_SRC:-$PROJECT_ROOT/chromium/src}"
PATCH_DIR_DEFAULT="$PROJECT_ROOT/chromium/astra/patches"
PATCH_DIR="$PATCH_DIR_DEFAULT"
MARKER_FILE=".astra_patches_applied"

REVERSE=false
CHECK_ONLY=false
LIST_ONLY=false
FORCE=false

# ---------------------------------------------------------------------------
# Argument parsing
# ---------------------------------------------------------------------------
usage() {
  cat <<EOF
$(basename "$0") — Apply or unapply Astra patches to Chromium source.

Usage:
  $(basename "$0") [options]

Options:
  --help              Show this help message.
  --reverse           Unapply patches instead of applying them.
  --check             Dry-run — verify patches would apply cleanly.
  --list              List all patches with current status.
  --chromium-src <p>  Path to Chromium source tree (default: ./chromium/src).
  --patch-dir <p>     Path to patch directory
                      (default: chromium/astra/patches).
  --force             Apply even if already applied marker exists.

Environment variables:
  CHROMIUM_SRC  Path to Chromium checkout (default: ./chromium/src).

Exit codes:
  0  All patches applied/unapplied successfully.
  1  Error occurred during patch operation.
  2  Some patches already applied (with --check).

Platform support:
  macOS, Linux, Windows (Git Bash / WSL).

Examples:
  $(basename "$0")                     # Apply all patches
  $(basename "$0") --reverse           # Unapply all patches
  $(basename "$0") --check             # Verify patches apply cleanly
  $(basename "$0") --list              # List patches with status
  $(basename "$0") --force             # Force re-apply
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --help)
      usage
      exit 0
      ;;
    --reverse)
      REVERSE=true
      shift
      ;;
    --check)
      CHECK_ONLY=true
      shift
      ;;
    --list)
      LIST_ONLY=true
      shift
      ;;
    --chromium-src)
      if [[ $# -lt 2 ]]; then
        error "--chromium-src requires an argument."
        exit 1
      fi
      CHROMIUM_SRC="$2"
      shift 2
      ;;
    --patch-dir)
      if [[ $# -lt 2 ]]; then
        error "--patch-dir requires an argument."
        exit 1
      fi
      PATCH_DIR="$2"
      shift 2
      ;;
    --force)
      FORCE=true
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
  if [[ ! -d "$CHROMIUM_SRC" ]]; then
    error "Chromium checkout not found at $CHROMIUM_SRC"
    error "Run: ./scripts/chromium-bootstrap.sh"
    exit 1
  fi

  if [[ ! -d "$PATCH_DIR" ]]; then
    error "Patch directory not found at $PATCH_DIR"
    exit 1
  fi

  if ! ls "$PATCH_DIR"/*.patch >/dev/null 2>&1; then
    warn "No .patch files found in $PATCH_DIR"
  fi
}

# ---------------------------------------------------------------------------
# Collect patches
# ---------------------------------------------------------------------------
collect_patches() {
  # Collect patch files in alphanumeric order.
  local patches=()
  for f in "$PATCH_DIR"/*.patch; do
    [[ -f "$f" ]] && patches+=("$(basename "$f")")
  done
  # Sort them.
  IFS=$'\n' sorted=($(sort <<<"${patches[*]}"))
  unset IFS
  echo "${sorted[@]}"
}

# ---------------------------------------------------------------------------
# Read applied patches from marker file
# ---------------------------------------------------------------------------
read_applied() {
  local marker="$CHROMIUM_SRC/$MARKER_FILE"
  if [[ ! -f "$marker" ]]; then
    echo ""
    return
  fi
  # Read lines into array, strip empty/comment lines.
  local -a applied=()
  while IFS= read -r line; do
    [[ -z "$line" || "$line" == \#* ]] && continue
    applied+=("$line")
  done < "$marker"
  echo "${applied[@]}"
}

# ---------------------------------------------------------------------------
# Write applied patches to marker file
# ---------------------------------------------------------------------------
write_applied() {
  local marker="$CHROMIUM_SRC/$MARKER_FILE"
  local -a patches=("$@")

  # Create the marker file header.
  cat > "$marker" <<EOF
# Astra patches applied to this Chromium checkout.
# Managed by scripts/apply-patches.sh — do not edit manually.
# Created: $(date -u +"%Y-%m-%dT%H:%M:%SZ")
EOF

  for p in "${patches[@]}"; do
    echo "$p" >> "$marker"
  done
}

# ---------------------------------------------------------------------------
# Check if a patch is in the applied list
# ---------------------------------------------------------------------------
is_applied() {
  local patch="$1"
  shift
  local -a applied=("$@")
  for p in "${applied[@]}"; do
    [[ "$p" == "$patch" ]] && return 0
  done
  return 1
}

# ---------------------------------------------------------------------------
# Apply a single patch
# ---------------------------------------------------------------------------
apply_single() {
  local patch_name="$1"
  local patch_path="$PATCH_DIR/$patch_name"
  local check_flag=""
  if $CHECK_ONLY; then
    check_flag="--check"
  fi

  # Try git am first (for proper git patch format).
  local failed=false
  local output=""

  if command -v git >/dev/null 2>&1 && [[ -d "$CHROMIUM_SRC/.git" ]]; then
    # Use git am if the patch is a git-format patch.
    if head -1 "$patch_path" | grep -q '^From '; then
      output="$(cd "$CHROMIUM_SRC" && git am $check_flag "$patch_path" 2>&1)" || failed=true

      if $failed && ! $CHECK_ONLY; then
        # Abort the failed git am so the tree is clean.
        (cd "$CHROMIUM_SRC" && git am --abort 2>/dev/null) || true
      fi
    else
      # Not a git-format patch, try patch command with git's pwd.
      output="$(cd "$CHROMIUM_SRC" && patch -p1 $check_flag < "$patch_path" 2>&1)" || failed=true
    fi
  else
    # Fall back to plain patch command.
    output="$(cd "$CHROMIUM_SRC" && patch -p1 $check_flag < "$patch_path" 2>&1)" || failed=true
  fi

  if $failed; then
    echo "FAILED"
    echo "$output"
    return 1
  fi

  echo "OK"
  return 0
}

# ---------------------------------------------------------------------------
# Reverse a single patch
# ---------------------------------------------------------------------------
reverse_single() {
  local patch_name="$1"
  local patch_path="$PATCH_DIR/$patch_name"
  local check_flag=""
  if $CHECK_ONLY; then
    check_flag="--check"
  fi

  local failed=false
  local output=""

  if command -v git >/dev/null 2>&1 && [[ -d "$CHROMIUM_SRC/.git" ]]; then
    # Try git apply --reverse for git-format patches.
    if head -1 "$patch_path" | grep -q '^From '; then
      output="$(cd "$CHROMIUM_SRC" && git apply --reverse $check_flag "$patch_path" 2>&1)" || failed=true
    else
      output="$(cd "$CHROMIUM_SRC" && patch -p1 -R $check_flag < "$patch_path" 2>&1)" || failed=true
    fi
  else
    output="$(cd "$CHROMIUM_SRC" && patch -p1 -R $check_flag < "$patch_path" 2>&1)" || failed=true
  fi

  if $failed; then
    echo "FAILED"
    echo "$output"
    return 1
  fi

  echo "OK"
  return 0
}

# ---------------------------------------------------------------------------
# List patches with status
# ---------------------------------------------------------------------------
list_patches() {
  echo -e "${BOLD}Astra patches${RESET}"
  echo ""
  printf "  %-4s  %-40s  %-10s  %s\n" "#" "Patch" "Status" "Path"
  printf "  %-4s  %-40s  %-10s  %s\n" "---" "----------------------------------------" "----------" "----"

  local -a patches
  read -ra patches <<< "$(collect_patches)"

  local -a applied
  read -ra applied <<< "$(read_applied)"

  local i=0
  for patch in "${patches[@]}"; do
    ((i++))
    local status=""
    local color=""
    if is_applied "$patch" "${applied[@]}"; then
      status="applied"
      color="$GREEN"
    else
      status="not applied"
      color="$YELLOW"
    fi

    # Get the target file from the patch.
    local target=""
    target="$(head -5 "$PATCH_DIR/$patch" | grep '^+++ ' | head -1 | sed 's/^+++ [ab]\///;s/^+++ //')"

    printf "  %-4s  %-40s  ${color}%-10s${RESET}  %s\n" "$i" "$patch" "$status" "$target"
  done

  echo ""
  echo "  Total: ${#patches[@]} patch(es)"
  echo "  Applied: $(echo "${applied[@]}" | wc -w | tr -d ' ')"
  echo ""
}

# ---------------------------------------------------------------------------
# Apply all patches
# ---------------------------------------------------------------------------
apply_all() {
  local -a patches
  read -ra patches <<< "$(collect_patches)"

  if [[ ${#patches[@]} -eq 0 ]]; then
    warn "No patches to apply."
    return 0
  fi

  local -a applied_before
  read -ra applied_before <<< "$(read_applied)"

  # Check if patches are already applied.
  if [[ ${#applied_before[@]} -gt 0 ]] && ! $FORCE && ! $CHECK_ONLY; then
    warn "${#applied_before[@]} patch(es) already marked as applied."
    warn "Use --force to re-apply, or --reverse to unapply first."
    return 1
  fi

  if $CHECK_ONLY; then
    info "Checking ${#patches[@]} patch(es)..."
  else
    info "Applying ${#patches[@]} patch(es)..."
  fi

  local -a newly_applied=()
  local -a failed=()

  for patch in "${patches[@]}"; do
    printf "  %-40s  " "$patch"

    if is_applied "$patch" "${applied_before[@]}" && ! $FORCE; then
      echo -e "${YELLOW}SKIP (already applied)${RESET}"
      continue
    fi

    local result
    result="$(apply_single "$patch")" || true

    if [[ "$result" == OK* ]]; then
      newly_applied+=("$patch")
      echo -e "${GREEN}OK${RESET}"
    else
      failed+=("$patch")
      echo -e "${RED}FAILED${RESET}"
      # Print the error output indented.
      local err_output
      err_output="$(echo "$result" | tail -n +2)"
      if [[ -n "$err_output" ]]; then
        echo "$err_output" | while IFS= read -r line; do
          echo -e "    ${RED}${line}${RESET}"
        done
      fi
    fi
  done

  echo ""

  # Update marker file on successful non-check run.
  if ! $CHECK_ONLY && [[ ${#failed[@]} -eq 0 ]]; then
    local -a all_applied=("${applied_before[@]}" "${newly_applied[@]}")
    # Re-sort to keep order.
    IFS=$'\n' sorted=($(sort <<<"${all_applied[*]}"))
    unset IFS
    write_applied "${sorted[@]}"
  fi

  # Summary.
  local applied_count=${#newly_applied[@]}
  local failed_count=${#failed[@]}

  if $CHECK_ONLY; then
    echo -e "  ${BLUE}Check complete:${RESET} $applied_count would apply, $failed_count would fail"
    if [[ $failed_count -gt 0 ]]; then
      error "$failed_count patch(es) would NOT apply cleanly."
      return 2
    fi
    success "All ${#patches[@]} patch(es) would apply cleanly."
    return 0
  fi

  echo -e "  ${BLUE}Applied:${RESET}    $applied_count patch(es)"
  echo -e "  ${RED}Failed:${RESET}     $failed_count patch(es)"

  if [[ $failed_count -gt 0 ]]; then
    error "$failed_count patch(es) failed to apply."
    error "Fix the conflicts and re-run, or use --reverse to unapply."
    return 1
  fi

  success "All ${#patches[@]} patch(es) applied successfully."
  return 0
}

# ---------------------------------------------------------------------------
# Reverse all patches
# ---------------------------------------------------------------------------
reverse_all() {
  local -a applied_before
  read -ra applied_before <<< "$(read_applied)"

  if [[ ${#applied_before[@]} -eq 0 ]]; then
    info "No patches marked as applied. Nothing to reverse."
    return 0
  fi

  # Reverse in reverse order.
  local -a reversed_order=()
  for (( idx=${#applied_before[@]}-1 ; idx>=0 ; idx-- )); do
    reversed_order+=("${applied_before[idx]}")
  done

  if $CHECK_ONLY; then
    info "Checking reverse of ${#reversed_order[@]} patch(es)..."
  else
    info "Reversing ${#reversed_order[@]} patch(es)..."
  fi

  local -a reversed=()
  local -a failed=()

  for patch in "${reversed_order[@]}"; do
    printf "  %-40s  " "$patch"

    # Verify the patch file exists.
    if [[ ! -f "$PATCH_DIR/$patch" ]]; then
      echo -e "${YELLOW}SKIP (patch file missing)${RESET}"
      warn "  Patch file not found: $PATCH_DIR/$patch"
      continue
    fi

    local result
    result="$(reverse_single "$patch")" || true

    if [[ "$result" == OK* ]]; then
      reversed+=("$patch")
      echo -e "${GREEN}OK${RESET}"
    else
      failed+=("$patch")
      echo -e "${RED}FAILED${RESET}"
      local err_output
      err_output="$(echo "$result" | tail -n +2)"
      if [[ -n "$err_output" ]]; then
        echo "$err_output" | while IFS= read -r line; do
          echo -e "    ${RED}${line}${RESET}"
        done
      fi
    fi
  done

  echo ""

  # Update marker file on successful non-check run.
  if ! $CHECK_ONLY; then
    # Remove reversed patches from the applied list.
    local -a remaining=()
    for p in "${applied_before[@]}"; do
      local was_reversed=false
      for r in "${reversed[@]}"; do
        [[ "$p" == "$r" ]] && was_reversed=true && break
      done
      $was_reversed || remaining+=("$p")
    done

    if [[ ${#remaining[@]} -eq 0 ]]; then
      # All reversed — remove marker file.
      rm -f "$CHROMIUM_SRC/$MARKER_FILE"
    else
      write_applied "${remaining[@]}"
    fi
  fi

  local reversed_count=${#reversed[@]}
  local failed_count=${#failed[@]}

  echo -e "  ${BLUE}Reversed:${RESET}   $reversed_count patch(es)"
  echo -e "  ${RED}Failed:${RESET}     $failed_count patch(es)"

  if [[ $failed_count -gt 0 ]]; then
    error "$failed_count patch(es) failed to reverse."
    return 1
  fi

  success "All reversed successfully."
  return 0
}

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------
preflight

if $LIST_ONLY; then
  list_patches
  exit 0
fi

if $REVERSE; then
  reverse_all
  exit $?
else
  apply_all
  exit $?
fi
