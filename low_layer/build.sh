#!/usr/bin/env bash
# Build-only script for low_layer
# Usage: ./build.sh

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "[build.sh] Cleaning..."
make -C "$SCRIPT_DIR" clean || true
echo "[build.sh] Building..."
make -C "$SCRIPT_DIR" build
echo "[build.sh] Build finished"
