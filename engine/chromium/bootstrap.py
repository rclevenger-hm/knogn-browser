#!/usr/bin/env python3
"""Prepare and optionally build Knogn's full Chromium backend.

Chromium remains in an external workspace. The tool uses the upstream depot_tools
workflow, checks out the pinned stable version, syncs DEPS, applies the Knogn
source overlay, generates out/Knogn, and optionally builds the full browser target.

Default mode is a dry run so normal CI can validate the exact plan without
performing a multi-gigabyte Chromium checkout. Use --execute for real work.
"""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import platform
import subprocess
import sys

HERE = Path(__file__).resolve().parent
PIN_FILE = HERE / "chromium_version.txt"
ARGS_FILE = HERE / "args.gn.example"
OVERLAY = HERE / "knogn_overlay.py"
DEPOT_TOOLS_URL = "https://chromium.googlesource.com/chromium/tools/depot_tools.git"


def executable_command(command: list[str]) -> list[str]:
    if platform.system() != "Windows":
        return command
    if command and command[0] in {"fetch", "gclient", "gn", "autoninja"}:
        return ["cmd.exe", "/d", "/s", "/c", subprocess.list2cmdline(command)]
    return command


def run(command: list[str], cwd: Path | None, env: dict[str, str], execute: bool) -> None:
    printable = " ".join(command)
    where = f" (cwd={cwd})" if cwd else ""
    print(f"+ {printable}{where}")
    if execute:
        subprocess.run(executable_command(command), cwd=cwd, env=env, check=True)


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
    parser = argparse.ArgumentParser(description="Prepare/build Knogn's Chromium backend")
    parser.add_argument(
        "--workspace",
        type=Path,
        default=Path.home() / "knogn-chromium",
        help="external workspace; Chromium is never vendored into the Knogn repo",
    )
    parser.add_argument(
        "--execute",
        action="store_true",
        help="perform checkout/sync/overlay/GN operations instead of printing them",
    )
    parser.add_argument(
        "--media-experiment",
        action="store_true",
        help="enable H.264/AAC-capable Chromium build settings for local testing only",
    )
    parser.add_argument(
        "--skip-fetch",
        action="store_true",
        help="require an existing Chromium checkout instead of running fetch",
    )
    parser.add_argument(
        "--skip-overlay",
        action="store_true",
        help="do not apply the Knogn product/privacy source overlay (diagnostic use only)",
    )
    parser.add_argument(
        "--jobs",
        type=int,
        default=max(2, (os.cpu_count() or 4) - 1),
        help="autoninja parallelism when --build is supplied",
    )
    parser.add_argument(
        "--build",
        action="store_true",
        help="build the full Chromium browser target after generation",
    )
    args = parser.parse_args()

    version = read_pin()
    workspace = args.workspace.expanduser().resolve()
    depot = workspace / "depot_tools"
    chromium_root = workspace / "chromium"
    src = chromium_root / "src"
    out = src / "out" / "Knogn"

    print(f"Knogn primary engine: Chromium {version}")
    print(f"Host: {platform.system()} {platform.machine()}")
    print(f"External workspace: {workspace}")
    if args.media_experiment:
        print("MEDIA EXPERIMENT: proprietary codecs enabled; local validation only, do not publish")

    if args.execute:
        workspace.mkdir(parents=True, exist_ok=True)

    env = os.environ.copy()
    env["PATH"] = str(depot) + os.pathsep + env.get("PATH", "")
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
        if args.execute:
            chromium_root.mkdir(parents=True, exist_ok=True)
        run(["fetch", "--nohooks", "--no-history", "chromium"], cwd=chromium_root, env=env, execute=args.execute)
    else:
        print(f"+ Chromium checkout already exists at {src}")

    run(["git", "fetch", "origin", "--tags", "--force"], cwd=src, env=env, execute=args.execute)
    run(["git", "checkout", "--detach", version], cwd=src, env=env, execute=args.execute)
    run(
        ["gclient", "sync", "-D", "--with_branch_heads", "--with_tags"],
        cwd=chromium_root,
        env=env,
        execute=args.execute,
    )

    if platform.system() == "Linux":
        run(["./build/install-build-deps.sh", "--no-prompt"], cwd=src, env=env, execute=args.execute)

    run(["gclient", "runhooks"], cwd=src, env=env, execute=args.execute)

    if not args.skip_overlay:
        print(f"+ {sys.executable} {OVERLAY} {src}")
        if args.execute:
            subprocess.run([sys.executable, str(OVERLAY), str(src)], check=True)

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

    if args.execute:
        print(f"\nKnogn Chromium backend prepared at {out}")
        if args.build:
            print("Full Chromium browser target built.")
    else:
        print("\nDry run complete. Use --execute to perform the Chromium preparation/build.")

    if args.media_experiment:
        print("Media experiment output is not approved for public redistribution.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
