#!/usr/bin/env python3
"""Primary Knogn developer entrypoint.

Chromium is the default backend. The Qt shell is retained only as an explicit
legacy fallback while 0.3.x reaches packaging parity.
"""

from __future__ import annotations

import argparse
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
BOOTSTRAP = ROOT / "engine/chromium/bootstrap.py"
PROBE = ROOT / "engine/chromium/probe_runtime.py"


def main() -> int:
    parser = argparse.ArgumentParser(description="Build Knogn")
    sub = parser.add_subparsers(dest="command", required=True)

    prepare = sub.add_parser("prepare", help="prepare the full Chromium backend")
    prepare.add_argument("--workspace", type=Path, default=Path.home() / "knogn-chromium")
    prepare.add_argument("--media-experiment", action="store_true")

    build = sub.add_parser("build", help="build the full Chromium browser")
    build.add_argument("--workspace", type=Path, default=Path.home() / "knogn-chromium")
    build.add_argument("--media-experiment", action="store_true")
    build.add_argument("--jobs", type=int)

    probe = sub.add_parser("probe", help="probe a built Chromium browser")
    probe.add_argument("--out", type=Path, default=Path.home() / "knogn-chromium/chromium/src/out/Knogn")
    probe.add_argument("--require-media", action="store_true")

    qt = sub.add_parser("qt-fallback", help="build the legacy Qt fallback shell")
    qt.add_argument("--build-dir", type=Path, default=ROOT / "build-qt")

    args = parser.parse_args()

    if args.command in {"prepare", "build"}:
        command = [sys.executable, str(BOOTSTRAP), "--execute", "--workspace", str(args.workspace)]
        if args.media_experiment:
            command.append("--media-experiment")
        if args.command == "build":
            command.append("--build")
            if args.jobs:
                command += ["--jobs", str(args.jobs)]
        return subprocess.call(command)

    if args.command == "probe":
        command = [sys.executable, str(PROBE), "--out", str(args.out)]
        if args.require_media:
            command.append("--require-media")
        return subprocess.call(command)

    build_dir = args.build_dir.resolve()
    subprocess.run(["cmake", "-S", str(ROOT), "-B", str(build_dir), "-DCMAKE_BUILD_TYPE=Release"], check=True)
    subprocess.run(["cmake", "--build", str(build_dir), "--config", "Release", "--parallel", "2"], check=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
