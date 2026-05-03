#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
VENV="$SCRIPT_DIR/.venv"

if [ ! -d "$VENV" ]; then
    python3 -m venv "$VENV"
fi

"$VENV/bin/pip" install --quiet -r "$SCRIPT_DIR/requirements.txt"
"$VENV/bin/python" "$SCRIPT_DIR/plot.py" "${1:-results/summary.csv}"