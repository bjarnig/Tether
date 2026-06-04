#!/usr/bin/env bash
# Configure, build, and install the Tether plugins into the user Extensions folder.
# Usage: ./build.sh [path-to-supercollider-source]
# SC_PATH must point at a source tree whose sc_api_version matches the scsynth
# that will load the plugins (3.12–3.14.x = 3, develop/3.15-dev = 6).
set -euo pipefail

SC_PATH="${1:-${SC_PATH:-../../supercollider-3.14.1}}"
BUILD_DIR="build"

cmake -S . -B "$BUILD_DIR" -DSC_PATH="$SC_PATH" -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD_DIR" --config Release
cmake --install "$BUILD_DIR" --config Release

echo "Installed. In SuperCollider: recompile the class library (Cmd/Ctrl-Shift-L), then reboot the server."
