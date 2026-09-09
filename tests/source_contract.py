#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
main = (ROOT / "src/main.cpp").read_text(encoding="utf-8")
profile = (ROOT / "src/privacyprofile.cpp").read_text(encoding="utf-8")
window = (ROOT / "src/browserwindow.cpp").read_text(encoding="utf-8")

checks = {
    "Knogn project name": "project(KnognBrowser" in cmake,
    "Qt 6.11 floor": "find_package(Qt6 6.11 REQUIRED" in cmake,
    "MV3 extension install": "installExtension(path)" in profile,
    "private profiles": "new QWebEngineProfile(this)" in profile,
    "back-forward cache": "BackForwardCacheEnabled, true" in profile,
    "background lifecycle optimization": "recommendedState()" in window,
    "private CLI mode": "privateOption" in main,
}

failed = [name for name, ok in checks.items() if not ok]
if failed:
    raise SystemExit("Source contract failures: " + ", ".join(failed))
print(f"source contract: {len(checks)} checks passed")
