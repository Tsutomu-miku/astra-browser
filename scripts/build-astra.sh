#!/usr/bin/env bash
# Build the Astra Chromium target.
#
# Generates GN args, runs gn gen, and builds with autoninja.
#
# Usage:
#   ./scripts/build-astra.sh [options]
#
# Options:
#   --help              Show this help message.
#   --release           Build release (default: debug).
#   --target <target>   Ninja target to build (default: chrome).
#   --jobs <N>          Number of parallel jobs (default: auto).
#   --clean             Clean the output directory before building.
#   --chromium-src <p>  Path to Chromium source tree.
#   --out-dir <name>    Output directory name
#                       (default: astra_Debug / astra_Release).
#   --args <str>        Extra GN args appended to the default set.
#   --no-patches        Skip the patch verification step.
#
# Environment:
#   DEPOT_TOOLS_DIR   Path to depot_tools (default: ./third_party/depot_tools).
#   CHROMIUM_SRC      Path to Chromium checkout (default: ./chromium/src).

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
step()    { echo -e "\n${BOLD}[${1}/4] ${2}${RESET}"; }

# ---------------------------------------------------------------------------
# Paths & defaults
# ---------------------------------------------------------------------------
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
DEPOT_TOOLS_DIR="${DEPOT_TOOLS_DIR:-$PROJECT_ROOT/third_party/depot_tools}"
CHROMIUM_SRC="${CHROMIUM_SRC:-$PROJECT_ROOT/chromium/src}"

BUILD_TYPE="Debug"
TARGET="chrome"
JOBS=""
CLEAN=false
OUT_DIR_NAME=""
EXTRA_GN_ARGS=""
SKIP_PATCHES=false

# ---------------------------------------------------------------------------
# Argument parsing
# ---------------------------------------------------------------------------
usage() {
  cat <<EOF
$(basename "$0") — Build Astra-branded Chromium.

Usage:
  $(basename "$0") [options]

Options:
  --help              Show this help message.
  --release           Build release (default: debug).
  --target <target>   Ninja target to build (default: chrome).
  --jobs <N>          Number of parallel jobs (default: auto-detect).
  --clean             Clean the output directory before building.
  --chromium-src <p>  Path to Chromium source tree (default: ./chromium/src).
  --out-dir <name>    Output directory name
                      (default: astra_Debug / astra_Release).
  --args <str>        Extra GN args appended to the defaults.
  --no-patches        Skip the patch verification step.

Environment variables:
  DEPOT_TOOLS_DIR  Path to depot_tools (default: ./third_party/depot_tools).
  CHROMIUM_SRC     Path to Chromium checkout (default: ./chromium/src).

Output locations:
  Debug:    out/astra_Debug/
  Release:  out/astra_Release/

Platform notes:
  macOS:    Output is Chrome.app bundle.
  Linux:    Output is chrome binary.
  Windows:  Output is chrome.exe (use Git Bash or WSL).

Examples:
  $(basename "$0")                              # Debug build of chrome
  $(basename "$0") --release                    # Release build
  $(basename "$0") --target chrome --jobs 8     # Specific target and job count
  $(basename "$0") --clean --release            # Clean release build
  $(basename "$0") --args 'is_component_build=true'
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --help)
      usage
      exit 0
      ;;
    --release)
      BUILD_TYPE="Release"
      shift
      ;;
    --target)
      if [[ $# -lt 2 ]]; then
        error "--target requires an argument."
        exit 1
      fi
      TARGET="$2"
      shift 2
      ;;
    --jobs)
      if [[ $# -lt 2 ]]; then
        error "--jobs requires an argument."
        exit 1
      fi
      JOBS="$2"
      shift 2
      ;;
    --clean)
      CLEAN=true
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
    --out-dir)
      if [[ $# -lt 2 ]]; then
        error "--out-dir requires an argument."
        exit 1
      fi
      OUT_DIR_NAME="$2"
      shift 2
      ;;
    --args)
      if [[ $# -lt 2 ]]; then
        error "--args requires an argument."
        exit 1
      fi
      EXTRA_GN_ARGS="$2"
      shift 2
      ;;
    --no-patches)
      SKIP_PATCHES=true
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
# Resolve output directory
# ---------------------------------------------------------------------------
if [[ -z "$OUT_DIR_NAME" ]]; then
  case "$BUILD_TYPE" in
    Debug)   OUT_DIR_NAME="astra_Debug" ;;
    Release) OUT_DIR_NAME="astra_Release" ;;
  esac
fi

OUT_DIR="$CHROMIUM_SRC/out/$OUT_DIR_NAME"

# ---------------------------------------------------------------------------
# Platform detection
# ---------------------------------------------------------------------------
detect_os() {
  case "$(uname -s)" in
    Darwin*) echo "macos" ;;
    Linux*)
      if grep -q microsoft /proc/version 2>/dev/null; then
        echo "wsl"
      else
        echo "linux"
      fi
      ;;
    MINGW*|MSYS*|CYGWIN*) echo "windows" ;;
    *) echo "unknown" ;;
  esac
}

OS="$(detect_os)"

# ---------------------------------------------------------------------------
# Print build header
# ---------------------------------------------------------------------------
print_header() {
  echo -e "${BOLD}========================================${RESET}"
  echo -e "${BOLD}  Astra Chromium Build${RESET}"
  echo -e "${BOLD}========================================${RESET}"
  echo ""
  echo -e "  ${BOLD}Build type:${RESET}  $BUILD_TYPE"
  echo -e "  ${BOLD}Target:${RESET}      $TARGET"
  echo -e "  ${BOLD}Output:${RESET}      out/$OUT_DIR_NAME"
  echo -e "  ${BOLD}Source:${RESET}      $CHROMIUM_SRC"
  echo -e "  ${BOLD}Platform:${RESET}    $OS"
  if [[ -n "$JOBS" ]]; then
    echo -e "  ${BOLD}Jobs:${RESET}        $JOBS"
  fi
  if [[ -n "$EXTRA_GN_ARGS" ]]; then
    echo -e "  ${BOLD}Extra args:${RESET}  $EXTRA_GN_ARGS"
  fi
  echo ""
}

# ---------------------------------------------------------------------------
# Step 1: Pre-flight checks
# ---------------------------------------------------------------------------
step_preflight() {
  step "1" "Pre-flight checks"

  # Check Chromium source exists.
  if [[ ! -d "$CHROMIUM_SRC" ]]; then
    error "Chromium checkout not found at $CHROMIUM_SRC"
    error "Run: ./scripts/chromium-bootstrap.sh"
    exit 1
  fi

  # Check depot_tools.
  export PATH="$DEPOT_TOOLS_DIR:$PATH"

  if ! command -v gn >/dev/null 2>&1; then
    error "gn not found. Is depot_tools in PATH?"
    error "  DEPOT_TOOLS_DIR=$DEPOT_TOOLS_DIR"
    error "Run: ./scripts/chromium-bootstrap.sh"
    exit 1
  fi

  if ! command -v autoninja >/dev/null 2>&1 && ! command -v ninja >/dev/null 2>&1; then
    error "neither autoninja nor ninja found."
    error "  DEPOT_TOOLS_DIR=$DEPOT_TOOLS_DIR"
    exit 1
  fi

  # Verify overlay is synced.
  info "Checking Astra overlay sync..."
  if ! "$SCRIPT_DIR/sync-overlay.sh" --check >/dev/null 2>&1; then
    error "Astra overlay is out of sync."
    error "Run: ./scripts/sync-overlay.sh"
    exit 1
  fi

  # Verify patches are applied.
  if ! $SKIP_PATCHES; then
    info "Checking patch status..."
    if [[ ! -f "$CHROMIUM_SRC/.astra_patches_applied" ]]; then
      warn "No Astra patches marked as applied."
      warn "Build may fail if patches are required for the target."
      warn "Run: ./scripts/apply-patches.sh"
    fi
  fi

  # Platform-specific checks.
  case "$OS" in
    macos)
      if ! xcode-select -p >/dev/null 2>&1; then
        error "Xcode Command Line Tools not installed."
        error "Run: xcode-select --install"
        exit 1
      fi
      ;;
    windows)
      warn "Windows builds via Git Bash are best-effort."
      warn "For official Windows builds, use depot_tools from cmd.exe"
      warn "with Visual Studio 2022 (17.0+) and the Windows SDK."
      ;;
  esac

  success "Pre-flight checks passed."
}

# ---------------------------------------------------------------------------
# Step 2: GN args
# ---------------------------------------------------------------------------
build_gn_args() {
  local -a args=()

  if [[ "$BUILD_TYPE" == "Release" ]]; then
    args+=(
      'is_debug=false'
      'is_component_build=false'
      'symbol_level=1'
      'is_official_build=false'
      'proprietary_codecs=false'
    )
  else
    args+=(
      'is_debug=true'
      'is_component_build=true'
      'symbol_level=1'
      'enable_nacl=false'
    )
  fi

  # Common args.
  args+=(
    'is_astra_branded=true'
    'enable_dremel=false'
    'enable_hangout_services_extension=false'
    'enable_widevine=false'
    'fatal_linker_warnings=false'
  )

  # Platform-specific args.
  case "$OS" in
    macos)
      args+=('use_remoteexec=false')
      ;;
    linux)
      # Common Linux defaults.
      ;;
    windows)
      args+=('is_win_fastlink=true')
      ;;
  esac

  # Append extra args if provided.
  if [[ -n "$EXTRA_GN_ARGS" ]]; then
    # Split extra args by space (simple approach).
    # This handles quoted args via bash word splitting of $EXTRA_GN_ARGS.
    # Note: this is best-effort for simple extra args.
    for extra_arg in $EXTRA_GN_ARGS; do
      args+=("$extra_arg")
    done
  fi

  echo "${args[@]}"
}

# ---------------------------------------------------------------------------
# Step 3: gn gen
# ---------------------------------------------------------------------------
step_gn_gen() {
  step "2" "Running gn gen"

  # Clean output dir if requested.
  if $CLEAN && [[ -d "$OUT_DIR" ]]; then
    info "Cleaning output directory: $OUT_DIR"
    rm -rf "$OUT_DIR"
  fi

  mkdir -p "$OUT_DIR"

  local gn_args
  gn_args="$(build_gn_args)"

  info "GN args:"
  for arg in $gn_args; do
    echo "    $arg"
  done

  (
    cd "$CHROMIUM_SRC"
    gn gen "out/$OUT_DIR_NAME" --args="$gn_args"
  )

  success "gn gen complete."
}

# ---------------------------------------------------------------------------
# Step 4: Build
# ---------------------------------------------------------------------------
step_build() {
  step "3" "Building with ninja"

  local ninja_cmd="autoninja"
  if ! command -v autoninja >/dev/null 2>&1; then
    ninja_cmd="ninja"
  fi

  local job_flags=()
  if [[ -n "$JOBS" ]]; then
    job_flags=("-j" "$JOBS")
  fi

  info "Running: $ninja_cmd ${job_flags[*]:-} -C out/$OUT_DIR_NAME $TARGET"

  local start_time
  start_time="$(date +%s)"

  (
    cd "$CHROMIUM_SRC"
    if [[ "$ninja_cmd" == "autoninja" ]]; then
      # autoninja handles its own job count.
      if [[ -n "$JOBS" ]]; then
        autoninja -j "$JOBS" -C "out/$OUT_DIR_NAME" "$TARGET"
      else
        autoninja -C "out/$OUT_DIR_NAME" "$TARGET"
      fi
    else
      ninja "${job_flags[@]}" -C "out/$OUT_DIR_NAME" "$TARGET"
    fi
  )

  local end_time
  end_time="$(date +%s)"
  local duration=$(( end_time - start_time ))

  local minutes=$(( duration / 60 ))
  local seconds=$(( duration % 60 ))

  success "Build complete in ${minutes}m ${seconds}s."
}

# ---------------------------------------------------------------------------
# Step 5: Summary
# ---------------------------------------------------------------------------
step_summary() {
  step "4" "Build summary"

  # Determine artifact path.
  local artifact=""
  case "$OS" in
    macos)
      artifact="out/$OUT_DIR_NAME/Chromium.app"
      if [[ -d "$CHROMIUM_SRC/$artifact" ]]; then
        :
      else
        artifact="out/$OUT_DIR_NAME/$TARGET.app"
      fi
      ;;
    linux)
      artifact="out/$OUT_DIR_NAME/$TARGET"
      ;;
    windows)
      artifact="out/$OUT_DIR_NAME/$TARGET.exe"
      ;;
    *)
      artifact="out/$OUT_DIR_NAME/$TARGET"
      ;;
  esac

  echo -e "  ${BOLD}Build type:${RESET}    $BUILD_TYPE"
  echo -e "  ${BOLD}Target:${RESET}        $TARGET"
  echo -e "  ${BOLD}Output dir:${RESET}    out/$OUT_DIR_NAME"
  echo -e "  ${BOLD}Artifact:${RESET}      $artifact"
  echo ""
  echo -e "  ${BOLD}Useful commands:${RESET}"
  echo "    Run:  $CHROMIUM_SRC/$artifact"
  echo "    Test: autoninja -C out/$OUT_DIR_NAME unit_tests"
  echo ""

  if [[ -x "$CHROMIUM_SRC/$artifact" ]] || [[ -d "$CHROMIUM_SRC/$artifact" ]]; then
    success "Artifact ready at $CHROMIUM_SRC/$artifact"
  else
    warn "Artifact not found at expected path: $artifact"
    warn "The target may produce a different output file."
  fi
}

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------
print_header
step_preflight
step_gn_gen
step_build
step_summary
