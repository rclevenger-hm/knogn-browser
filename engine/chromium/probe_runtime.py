#!/usr/bin/env python3
"""Probe a built Knogn Chromium browser for identity/media capabilities."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import platform
import re
import subprocess
import tempfile

HERE = Path(__file__).resolve().parent
PROBE_PAGE = HERE / "runtime_probe.html"


def browser_binary(out: Path) -> Path:
    system = platform.system()
    candidates = []
    if system == "Windows":
        candidates = [out / "chrome.exe", out / "Knogn.exe"]
    elif system == "Darwin":
        candidates = [
            out / "Chromium.app/Contents/MacOS/Chromium",
            out / "Knogn.app/Contents/MacOS/Knogn",
        ]
    else:
        candidates = [out / "chrome", out / "Knogn", out / "chromium"]
    for candidate in candidates:
        if candidate.exists():
            return candidate
    raise SystemExit(f"No Chromium browser executable found under {out}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", type=Path, required=True, help="Chromium out/Knogn directory")
    parser.add_argument("--require-media", action="store_true", help="fail unless H.264/AAC + MSE are exposed")
    args = parser.parse_args()

    binary = browser_binary(args.out.resolve())
    with tempfile.TemporaryDirectory(prefix="knogn-probe-") as profile:
        command = [
            str(binary),
            "--headless=new",
            "--no-first-run",
            "--no-default-browser-check",
            f"--user-data-dir={profile}",
            "--disable-background-networking",
            "--dump-dom",
            PROBE_PAGE.resolve().as_uri(),
        ]
        completed = subprocess.run(command, text=True, capture_output=True, check=True, timeout=90)

    match = re.search(r"KNOGN_PROBE:(\{.*?\})", completed.stdout, re.DOTALL)
    if not match:
        raise SystemExit("Runtime probe marker was not returned by the built browser")
    result = json.loads(match.group(1))
    print(json.dumps(result, indent=2, sort_keys=True))

    if not result.get("chromiumBrowserIdentity"):
        raise SystemExit("Browser identity still looks embedded/non-Chromium")
    if not result.get("credentialsApi") or not result.get("webauthn"):
        raise SystemExit("Mainstream credential/WebAuthn browser surfaces are missing")
    if args.require_media:
        if result.get("h264Aac") == "unsupported" or not result.get("mseH264Aac"):
            raise SystemExit("H.264/AAC Media Source parity is not present")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
