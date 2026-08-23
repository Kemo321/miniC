#!/usr/bin/env bash
# Thin wrapper around run_e2e.py
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
exec python3 "$ROOT/run_e2e.py" "$@"
