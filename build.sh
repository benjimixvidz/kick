#!/usr/bin/env bash
# Builds the desktop Linux version and optionally installs or runs it.
#
#   ./build.sh                    Release build in ./build
#   ./build.sh --debug            Debug build in ./build-debug
#   ./build.sh --install [PREFIX] Build, then install to PREFIX (default ~/.local)
#   ./build.sh --run              Build, then start it from the build tree
#
# Dependencies (Debian/Ubuntu names): cmake ninja-build g++ pkg-config libasound2-dev
# libjack-jackd2-dev libx11-dev libxext-dev libxrandr-dev libxinerama-dev libxcursor-dev
# libxcomposite-dev libxi-dev libxrender-dev libfreetype-dev libfontconfig1-dev
# libcurl4-openssl-dev libgl1-mesa-dev libglu1-mesa-dev
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_TYPE=Release
BUILD_DIR="$ROOT/build"
INSTALL_PREFIX=""
RUN=0

while [ $# -gt 0 ]; do
    case "$1" in
        --debug) BUILD_TYPE=Debug; BUILD_DIR="$ROOT/build-debug" ;;
        --install)
            INSTALL_PREFIX="$HOME/.local"
            if [ $# -gt 1 ] && [ "${2#--}" = "$2" ]; then INSTALL_PREFIX="$2"; shift; fi ;;
        --run) RUN=1 ;;
        -h|--help) sed -n '2,12p' "$0"; exit 0 ;;
        *) echo "Unknown option: $1" >&2; exit 2 ;;
    esac
    shift
done

missing=()
for tool in cmake ninja g++ pkg-config git; do
    command -v "$tool" >/dev/null 2>&1 || missing+=("$tool")
done
for pkg in alsa freetype2 x11 xext xrandr xinerama xcursor xi; do
    pkg-config --exists "$pkg" 2>/dev/null || missing+=("$pkg (dev package)")
done
if [ ${#missing[@]} -gt 0 ]; then
    echo "Missing build dependencies: ${missing[*]}" >&2
    sed -n '9,12p' "$0" >&2
    exit 1
fi

# Downloads (JUCE) are cached next to the sources so clean builds stay offline.
cmake -S "$ROOT/Kick" -B "$BUILD_DIR" -G Ninja \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DKICK_DESKTOP=ON \
    -DTIMESTAMP=OFF \
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
    -DCPM_SOURCE_CACHE="$ROOT/.cache/cpm" \
    -DCPM_AudIO_SOURCE="$ROOT/AudIO" \
    -DCPM_COM_PROTOCOL_SOURCE="$ROOT/ComProtocol"
cmake --build "$BUILD_DIR" --target Kick

BINARY="$BUILD_DIR/apps/kick/Kick_artefacts/$BUILD_TYPE/Kick"
echo "Built: $BINARY"

if [ -n "$INSTALL_PREFIX" ]; then
    share="$INSTALL_PREFIX/share/kick-desktop"
    mkdir -p "$INSTALL_PREFIX/bin" "$share" "$INSTALL_PREFIX/share/applications"
    install -m 755 "$BINARY" "$INSTALL_PREFIX/bin/kick-desktop"
    for dir in assets demos templates; do
        rm -rf "${share:?}/$dir"
        cp -r "$ROOT/Kick/$dir" "$share/$dir"
    done
    sed "s|@BINARY@|$INSTALL_PREFIX/bin/kick-desktop|" "$ROOT/packaging/kick-desktop.desktop.in" \
        > "$INSTALL_PREFIX/share/applications/kick-desktop.desktop"
    echo "Installed to $INSTALL_PREFIX (run: kick-desktop)"
fi

if [ "$RUN" = 1 ]; then
    exec "$BINARY"
fi
