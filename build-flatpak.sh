#!/usr/bin/env bash
# build-flatpak.sh
# Sets up the Flatpak builder environment and builds the Flatpak bundle.
#
# Usage:
#   ./build-flatpak.sh              # build only
#   ./build-flatpak.sh --install    # build and install for the current user
#
# Requirements:
#   flatpak, flatpak-builder
#   Manifest file: io.github.wheat32.CSServerManager.yml (must be created)
#
# The output bundle is written to:
#   dist/io.github.wheat32.CSServerManager.flatpak

set -euo pipefail

MANIFEST="io.github.wheat32.CSServerManager.yml"
APP_ID="io.github.wheat32.CSServerManager"
BUILD_DIR=".flatpak-build"
REPO_DIR=".flatpak-repo"
BUNDLE="dist/${APP_ID}.flatpak"
RUNTIME="org.kde.Platform"
RUNTIME_VERSION=""   # resolved dynamically from Flathub at runtime

# ── Resolve latest KDE runtime version from Flathub ──────────────────────────
resolve_runtime_version() {
    info "Querying Flathub for the latest ${RUNTIME} version..."
    local latest
    latest=$(flatpak remote-ls --user --runtime --columns=ref flathub 2>/dev/null \
        | grep "^runtime/${RUNTIME}/x86_64/" \
        | sed "s|runtime/${RUNTIME}/x86_64/||" \
        | grep -E '^[0-9]+\.[0-9]+$' \
        | sort -V \
        | tail -1)

    if [[ -z "$latest" ]]; then
        die "Could not resolve the latest ${RUNTIME} version from Flathub. Are you online?"
    fi

    RUNTIME_VERSION="$latest"
    info "Using KDE runtime version: ${RUNTIME_VERSION}"
}

# ── Color helpers ────────────────────────────────────────────────────────────
GREEN='\033[0;32m'; YELLOW='\033[1;33m'; RED='\033[0;31m'; NC='\033[0m'
info()    { echo -e "${GREEN}[build-flatpak]${NC} $*"; }
warn()    { echo -e "${YELLOW}[build-flatpak]${NC} $*"; }
die()     { echo -e "${RED}[build-flatpak] ERROR:${NC} $*" >&2; exit 1; }

# ── Sanity checks ─────────────────────────────────────────────────────────────
command -v flatpak         >/dev/null 2>&1 || die "flatpak is not installed. Install with: sudo apt install flatpak"
command -v flatpak-builder >/dev/null 2>&1 || die "flatpak-builder is not installed. Install with: sudo apt install flatpak-builder"

[[ -f "$MANIFEST" ]] || die "Manifest '$MANIFEST' not found. Create it before running this script."

# ── Ensure the KDE runtime + SDK are available ───────────────────────────────
info "Checking for Flatpak remote 'flathub'..."
if ! flatpak remote-list --user | grep -q flathub; then
    info "Adding flathub remote (user)..."
    flatpak remote-add --user --if-not-exists flathub \
        https://dl.flathub.org/repo/flathub.flatpakrepo
fi

resolve_runtime_version

# ── Patch manifest with resolved runtime version ──────────────────────────────
info "Patching manifest: runtime-version -> ${RUNTIME_VERSION}..."
sed -i "s/^runtime-version:.*/runtime-version: '${RUNTIME_VERSION}'/" "$MANIFEST"

for component in "${RUNTIME}/${RUNTIME_VERSION}" "${RUNTIME%Platform}Sdk/${RUNTIME_VERSION}"; do
    ref="${component%%/*}/x86_64/${component#*/}"
    if ! flatpak info --user "$ref" >/dev/null 2>&1; then
        info "Installing $ref from flathub..."
        flatpak install --user --noninteractive flathub "$ref" || \
            warn "Could not auto-install $ref — you may need to run: flatpak install flathub $ref"
    fi
done

# ── Build ─────────────────────────────────────────────────────────────────────
info "Building Flatpak (this may take a while on first run)..."
mkdir -p dist

flatpak-builder \
    --force-clean \
    --repo="$REPO_DIR" \
    "$BUILD_DIR" \
    "$MANIFEST"

# ── Export bundle ─────────────────────────────────────────────────────────────
info "Exporting bundle to $BUNDLE..."
flatpak build-bundle \
    "$REPO_DIR" \
    "$BUNDLE" \
    "$APP_ID"

info "Build complete: $BUNDLE"

# ── Optional install ──────────────────────────────────────────────────────────
if [[ "${1:-}" == "--install" ]]; then
    info "Installing $APP_ID for current user..."
    flatpak install --user --noninteractive "$BUNDLE"
    info "Run with: flatpak run $APP_ID"
fi
