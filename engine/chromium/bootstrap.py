#!/usr/bin/env python3
"""Prepare an external Chromium workspace for the Knogn full-browser backend.

This script intentionally does not vendor Chromium into knogn-browser. It creates
an external checkout using Chromium's depot_tools workflow, checks out the
version pinned in chromium_version.txt, installs build hooks, and generates a
Knogn output directory from args.gn.example.

By default the script prints its plan. Pass --execute to run the network- and
storage-heavy checkout/build preparation commands.
"""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import platform
import shutil
import subprocess
import sys

HERE = Path(__file__).resolve().parent
PIN_FILE = HERE / "chromium_version.txt"
ARGS_FILE = HERE / "args.gn.example"
DEPOT_TOOLS_URL = "https://chromium.googlesource.com/chromium/tools/depot_tools.git"


def run(command: list[str], cwd: Path | None, env: dict[str, str], execute: bool) -> None:
    printable = " ".join(command)
    where = f" (cwd={cwd})" if cwd else ""
    print(f"+ {printable}{where}")
    if execute:
        subprocess.run(command, cwd=cwd, env=env, check=True)


def read_pin() -> str:
    version = PIN_FILE.read_text(encoding="utf-8").strip()
    if not version or any(ch.isspace() for ch in version):
        raise SystemExit(f"Invalid Chromium pin in {PIN_FILE}")
    return version


def render_args(media_experiment: bool) -> str:
    text = ARGS_FILE.read_text(encoding="utf-8")
    if media_experiment:
        text = text.replace("proprietary_codecs = false", "proprietary_codecs = true")
        text = text.replace('ffmpeg_branding = "Chromium"', 'ffmpeg_branding = "Chrome"')
    return text


def main() -> int:
    parser = argparse.ArgumentParser(description="Prepare the Knogn Chromium backend workspace")
    parser.add_argument(
        "--workspace",
        type=Path,
        default=Path.home() / "knogn-chromium",
        help="external workspace; Chromium is never written into the Knogn repo",
    )
    parser.add_argument(
        "--execute",
        action="store_true",
        help="run the checkout/sync/hooks/GN commands instead of only printing them",
    )
    parser.add_argument(
        "--media-experiment",
        action="store_true",
        help="generate proprietary-codec experiment args; not approved for public distribution",
    )
    parser.add_argument(
        "--skip-fetch",
        action="store_true",
        help="require an existing Chromium src checkout instead of running fetch",
    )
    parser.add_argument(
        "--jobs",
        type=int,
        default=max(2, (os.cpu_count() or 4) - 1),
        help="ninja parallelism used only when --build is supplied",
    )
    parser.add_argument(
        "--build",
        action="store_true",
        help="build the Chromium browser target after generation",
    )
    args = parser.parse_args()

    version = read_pin()
    workspace = args.workspace.expanduser().resolve()
    depot = workspace / "depot_tools"
    src = workspace / "chromium" / "src"
    checkout_root = src.parent
    out = src / "out" / "Knogn"

    print(f"Knogn Chromium pin: {version}")
    print(f"Host platform: {platform.system()} {platform.machine()}")
    print(f"External workspace: {workspace}")
    if args.media_experiment:
        print("MEDIA EXPERIMENT: proprietary codecs enabled in generated args; DO NOT DISTRIBUTE by default")

    env = os.environ.copy()
    path_sep = os.pathsep
    env["PATH"] = str(depot) + path_sep + env.get("PATH", "")
    if platform.system() == "Windows":
        env.setdefault("DEPOT_TOOLS_WIN_TOOLCHAIN", "0")

    if not depot.exists():
        run(["git", "clone", DEPOT_TOOLS_URL, str(depot)], cwd=workspace, env=env, execute=args.execute)
    elif args.execute:
        run(["git", "pull", "--ff-only"], cwd=depot, env=env, execute=True)
    else:
        print(f"+ depot_tools already exists at {depot}")

    if not src.exists():
        if args.skip_fetch:
            raise SystemExit(f"--skip-fetch supplied but {src} does not exist")
        chromium_root = workspace / "chromium"
        if args.execute:
            chromium_root.mkdir(parents=True, exist_ok=True)
        run(
            ["fetch", "--nohooks", "--no-history", "chromium"],
            cwd=chromium_root,
            env=env,
            execute=args.execute,
        )
    else:
        print(f"+ Chromium checkout already exists at {src}")

    # The stable Chrome/Chromium version is used as a detached revision pin. A
    # full sync after checkout aligns DEPS-managed repositories to that revision.
    run(["git", "fetch", "origin", "--tags", "--force"], cwd=src, env=env, execute=args.execute)
    run(["git", "checkout", "--detach", version], cwd=src, env=env, execute=args.execute)
    run(["gclient", "sync", "-D", "--with_branch_heads", "--with_tags"], cwd=checkout_root, env=env, execute=args.execute)

    if platform.system() == "Linux":
        run(["./build/install-build-deps.sh", "--no-prompt"], cwd=src, env=env, execute=args.execute)

    run(["gclient", "runhooks"], cwd=src, env=env, execute=args.execute)

    generated_args = render_args(args.media_experiment)
    print(f"+ write {out / 'args.gn'} from {ARGS_FILE}")
    if args.execute:
        out.mkdir(parents=True, exist_ok=True)
        (out / "args.gn").write_text(generated_args, encoding="utf-8")

    run(["gn", "gen", str(out)], cwd=src, env=env, execute=args.execute)

    if args.build:
        run(
            ["autoninja", "-C", str(out), f"-j{args.jobs}", "chrome"],
            cwd=src,
            env=env,
            execute=args.execute,
        )

    print("\nBootstrap plan complete." if not args.execute else "\nChromium workspace prepared.")
    print("Public Knogn packages must keep the proprietary-codec experiment disabled until redistribution rights are resolved.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
