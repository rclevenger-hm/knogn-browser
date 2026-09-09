#!/usr/bin/env bash
set -euo pipefail
python3 tests/privacy_contract.py
python3 tests/source_contract.py
python3 tests/performance_contract.py
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 2
