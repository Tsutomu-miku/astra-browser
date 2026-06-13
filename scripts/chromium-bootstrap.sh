#!/usr/bin/env bash
# Bootstrap a direct Chromium checkout for Astra development.
#
# What this does:
#   1. Checks for depot_tools (installs/clones if missing).
#   2. Sets up gclient config.
#   3. Fetches Chromium (or uses existing checkout).
#   4. Checks out a specific Chromium revision.
#   5. Prints next steps and status.
#
# Usage:
#   ./scripts/chromium-bootstrap.sh [options]
#
# Options:
#   --help              Show this help message.
#   --force             Force re-bootstrap even if checkout exists.
#   --revision <rev>    Chromium revision/tag to check out
#                       (default: CHROMIUM_REVISION from script).
#   --no-sync           Skip gclient sync after checkout.
#
# Environment:
#   DEPOT_TOOLS_DIR     Path to depot_tools
#                       (default: ./third_party/depot_tools).
#   CHROMIUM_SRC        Path to Chromium checkout
#                       (default: ./chromium/src).
#   CHROMIUM_REVISION   Default Chromium revision/tag to use.

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
banner()  { echo -e "\n${BOLD}== $* ==${RESET}\n"; }

# ---------------------------------------------------------------------------
# Paths & defaults
# ---------------------------------------------------------------------------
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
DEPOT_TOOLS_DIR="${DEPOT_TOOLS_DIR:-$PROJECT_ROOT/third_party/depot_tools}"
CHROMIUM_SRC="${CHROMIUM_SRC:-$PROJECT_ROOT/chromium/src}"
CHROMIUM_PARENT="$(dirname "$CHROMIUM_SRC")"
CHROMIUM_REVISION="${CHROMIUM_REVISION:-128.0.6613.119}"

FORCE=false
NO_SYNC=false

# ---------------------------------------------------------------------------
# Argument parsing
# ---------------------------------------------------------------------------
usage() {
  cat <<EOF
$(basename "$0") — Bootstrap a Chromium checkout for Astra development.

Usage:
  $(basename "$0") [options]

Options:
  --help              Show this help message.
  --force             Force re-bootstrap even if checkout exists.
  --revision <rev>    Chromium revision/tag to check out
                      (default: ${CHROMIUM_REVISION}).
  --no-sync           Skip gclient sync after checkout.

Environment variables:
  DEPOT_TOOLS_DIR     Path to depot_tools (default: ./third_party/depot_tools).
  CHROMIUM_SRC        Path to Chromium checkout (default: ./chromium/src).
  CHROMIUM_REVISION   Default Chromium revision to use.

Platform support:
  macOS, Linux, Windows (Git Bash / WSL).

Examples:
  $(basename "$0")
  $(basename "$0") --revision 128.0.6613.119
  $(basename "$0") --force --no-sync
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --help)
      usage
      exit 0
      ;;
    --force)
      FORCE=true
      shift
      ;;
    --revision)
      if [[ $# -lt 2 ]]; then
        error "--revision requires an argument."
        exit 1
      fi
      CHROMIUM_REVISION="$2"
      shift 2
      ;;
    --no-sync)
      NO_SYNC=true
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
# Step 1: depot_tools
# ---------------------------------------------------------------------------
ensure_depot_tools() {
  banner "1/4 — depot_tools"

  if [[ -d "$DEPOT_TOOLS_DIR" ]]; then
    if $FORCE; then
      info "Forcing re-clone of depot_tools..."
      rm -rf "$DEPOT_TOOLS_DIR"
    else
      info "depot_tools found at $DEPOT_TOOLS_DIR"
      info "Updating depot_tools..."
      (
        cd "$DEPOT_TOOLS_DIR"
        git pull --ff-only origin main 2>/dev/null || warn "Could not update depot_tools (offline?)"
      )
      export PATH="$DEPOT_TOOLS_DIR:$PATH"
      if command -v gclient >/dev/null 2>&1; then
        success "depot_tools ready."
        return
      fi
    fi
  fi

  info "Cloning depot_tools..."
  mkdir -p "$(dirname "$DEPOT_TOOLS_DIR")"

  if ! command -v git >/dev/null 2>&1; then
    error "git is required but not found."
    case "$OS" in
      macos)
        error "Install Xcode Command Line Tools: xcode-select --install"
        ;;
      linux)
        error "Install git: sudo apt-get install git"
        ;;
      windows|wsl)
        error "Install Git for Windows: https://git-scm.com/download/win"
        ;;
    esac
    exit 1
  fi

  git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git \
    "$DEPOT_TOOLS_DIR"

  export PATH="$DEPOT_TOOLS_DIR:$PATH"

  # Verify depot_tools commands are available.
  if ! command -v gclient >/dev/null 2>&1; then
    error "gclient not found after depot_tools setup."
    exit 1
  fi

  success "depot_tools installed."
}

# ---------------------------------------------------------------------------
# Step 2: gclient config
# ---------------------------------------------------------------------------
setup_gclient() {
  banner "2/4 — gclient config"

  mkdir -p "$CHROMIUM_PARENT"

  local gclient_file="$CHROMIUM_PARENT/.gclient"

  if [[ -f "$gclient_file" ]] && ! $FORCE; then
    info ".gclient already exists at $gclient_file"
    return
  fi

  info "Creating .gclient config..."

  cat > "$gclient_file" <<EOF
solutions = [
  {
    "name": "src",
    "url": "https://chromium.googlesource.com/chromium/src.git",
    "deps_file": "DEPS",
    "managed": False,
    "custom_deps": {},
    "custom_vars": {},
  },
]
EOF

  success ".gclient created at $gclient_file"
}

# ---------------------------------------------------------------------------
# Step 3: Chromium checkout
# ---------------------------------------------------------------------------
ensure_chromium_checkout() {
  banner "3/4 — Chromium checkout"

  if [[ -d "$CHROMIUM_SRC" ]]; then
    if $FORCE; then
      warn "Forcing re-fetch. This will remove existing checkout at $CHROMIUM_SRC"
      read -p "Continue? [y/N] " -n 1 -r
      echo
      if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        error "Aborted by user."
        exit 1
      fi
      rm -rf "$CHROMIUM_SRC"
    else
      info "Chromium checkout found at $CHROMIUM_SRC"

      # Verify it's a valid git repo.
      if [[ ! -d "$CHROMIUM_SRC/.git" ]]; then
        error "$CHROMIUM_SRC exists but is not a git repository."
        error "Use --force to re-bootstrap."
        exit 1
      fi

      # Check if the requested revision exists.
      local has_rev=false
      if (cd "$CHROMIUM_SRC" && git rev-parse --verify --quiet "$CHROMIUM_REVISION" >/dev/null 2>&1); then
        has_rev=true
      elif (cd "$CHROMIUM_SRC" && git tag -l "$CHROMIUM_REVISION" | grep -q .); then
        has_rev=true
      fi

      if $has_rev; then
        info "Requested revision $CHROMIUM_REVISION available."
      else
        warn "Revision $CHROMIUM_REVISION not found locally. Will fetch..."
        (
          cd "$CHROMIUM_SRC"
          git fetch origin "refs/tags/$CHROMIUM_REVISION:refs/tags/$CHROMIUM_REVISION" 2>/dev/null || \
            git fetch origin "$CHROMIUM_REVISION" 2>/dev/null || \
            warn "Could not fetch revision. Check network or specify a local ref."
        )
      fi

      # Checkout the requested revision.
      info "Checking out $CHROMIUM_REVISION..."
      (
        cd "$CHROMIUM_SRC"
        git checkout "$CHROMIUM_REVISION"
      )
      success "Chromium at revision $CHROMIUM_REVISION"

      if ! $NO_SYNC; then
        info "Running gclient sync..."
        (
          cd "$CHROMIUM_SRC"
          gclient sync
        )
        success "gclient sync complete."
      fi

      return
    fi
  fi

  # Fresh checkout.
  info "Fetching Chromium (this may take a while — 30+ GB download)..."
  info "Platform: $OS"

  case "$OS" in
    macos)
      info "Note: On macOS, you need Xcode and the 10.15+ SDK."
      ;;
    linux)
      info "Note: On Linux, you may need to install build dependencies first."
      info "      See: https://chromium.googlesource.com/chromium/src/+/main/docs/linux/build_instructions.md"
      ;;
    windows)
      warn "Windows support is best-effort via Git Bash."
      warn "For official Windows builds, use depot_tools from cmd.exe."
      ;;
    wsl)
      warn "Building Chromium in WSL is possible but may have performance issues."
      warn "Consider a native Linux build or Windows build."
      ;;
  esac

  info "Running 'fetch chromium'..."
  (
    cd "$CHROMIUM_PARENT"
    fetch --nohooks chromium
  )

  # Checkout the specific revision.
  info "Checking out $CHROMIUM_REVISION..."
  (
    cd "$CHROMIUM_SRC"
    git checkout "$CHROMIUM_REVISION"
  )

  if ! $NO_SYNC; then
    info "Running gclient sync..."
    (
      cd "$CHROMIUM_SRC"
      gclient sync
    )
  fi

  success "Chromium checkout ready."
}

# ---------------------------------------------------------------------------
# Step 4: Status & next steps
# ---------------------------------------------------------------------------
print_status_and_next_steps() {
  banner "4/4 — Status & next steps"

  echo -e "  ${BOLD}Chromium revision:${RESET}  $CHROMIUM_REVISION"
  echo -e "  ${BOLD}Source path:${RESET}        $CHROMIUM_SRC"
  echo -e "  ${BOLD}depot_tools:${RESET}        $DEPOT_TOOLS_DIR"
  echo ""
  echo -e "  ${BOLD}Next steps:${RESET}"
  echo "    1. Sync Astra overlay:"
  echo "       ./scripts/sync-overlay.sh"
  echo ""
  echo "    2. Apply Astra patches to Chromium:"
  echo "       ./scripts/apply-patches.sh"
  echo ""
  echo "    3. Build Astra Chromium:"
  echo "       ./scripts/build-astra.sh"
  echo ""
  echo -e "  ${BOLD}Documentation:${RESET}"
  echo "    docs/BUILD.md — Full build guide and troubleshooting"
  echo ""

  success "Bootstrap complete."
}

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------
banner "Astra Chromium Bootstrap"
echo -e "  ${BOLD}Revision:${RESET}    $CHROMIUM_REVISION"
echo -e "  ${BOLD}Platform:${RESET}    $OS"
echo -e "  ${BOLD}Force:${RESET}       $FORCE"
echo -e "  ${BOLD}Skip sync:${RESET}   $NO_SYNC"

ensure_depot_tools
setup_gclient
ensure_chromium_checkout
print_status_and_next_steps
