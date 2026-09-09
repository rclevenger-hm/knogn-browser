#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
flags = (ROOT / "src/browserflags.cpp").read_text(encoding="utf-8")
profile = (ROOT / "src/privacyprofile.cpp").read_text(encoding="utf-8")

checks = {
    "no broad background-networking kill switch": "--disable-background-networking" not in flags,
    "back-forward cache enabled": "BackForwardCacheEnabled, true" in profile,
    "hardware acceleration not disabled": "--disable-gpu" not in flags,
    "software rendering not forced": "--disable-gpu-compositing" not in flags,
    "single-process mode not forced": "--single-process" not in flags,
}

failed = [name for name, ok in checks.items() if not ok]
if failed:
    raise SystemExit("Performance contract failures: " + ", ".join(failed))
print(f"performance contract: {len(checks)} checks passed")
