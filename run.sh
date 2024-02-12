#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"

# run
cd "$SCRIPT_DIR"
cd interface
source .venv/bin/activate
python dashboard.py