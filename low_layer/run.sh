#!/usr/bin/env bash
# Run-only script for low_layer
# Usage: ./run.sh <data_file> <algorithm>

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DATA_FILE=${1:-"$SCRIPT_DIR/../data/10_2_2_2.txt"}
if [ "$#" -ge 2 ]; then
    shift
    ALGORITHMS=("$@")
else
    ALGORITHMS=("neh")
fi

EXE_PATH="$SCRIPT_DIR/bin/pfsp_sdst"

if [ ! -f "$DATA_FILE" ]; then
    echo "[run.sh] Data file not found: $DATA_FILE" >&2
    exit 2
fi

if [ ! -x "$EXE_PATH" ]; then
    echo "[run.sh] Binary not found or not executable: $EXE_PATH" >&2
    exit 3
fi

echo "[run.sh] Running: $EXE_PATH $DATA_FILE ${ALGORITHMS[*]}"
"$EXE_PATH" "$DATA_FILE" "${ALGORITHMS[@]}"
