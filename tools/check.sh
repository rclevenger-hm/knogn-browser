#!/usr/bin/env bash
set -euo pipefail

python3 tests/privacy_contract.py
python3 tests/source_contract.py
python3 tests/performance_contract.py
python3 tests/media_contract.py
python3 tests/branding_contract.py
python3 tests/daily_driver_contract.py
python3 tests/identity_contract.py
python3 tests/chromium_backend_contract.py
python3 tests/chromium_primary_contract.py

# Validate the exact primary-engine bootstrap/overlay plan without downloading
# Chromium. Use tools/build_knogn.py build for the real full-browser build.
python3 engine/chromium/bootstrap.py --workspace /tmp/knogn-chromium-contract

echo "Knogn Chromium-primary local contracts passed."
echo "Qt fallback compile, when needed: python3 tools/build_knogn.py qt-fallback"
