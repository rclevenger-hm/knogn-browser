#!/usr/bin/env python3
"""Prepare an external Chromium workspace for the Knogn full-browser backend.

Chromium is intentionally not vendored into knogn-browser. This tool uses the
upstream depot_tools workflow, checks out the revision pinned in
chromium_version.txt, synchronizes DEPS, runs Chromium hooks, and generates an
out/Knogn directory from args.gn.example.

The default mode is a dry run so CI can validate the plan without downloading a
large Chromium checkout. Pass --execute for the storage/network-heavy operation.
"""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import platform
import subprocess

HERE = Path(__file__).resolve().parent
PIN_FILE = HERE / "chromium_version.txt"
ARGS_FILE = HERE / "args.gn.example"
DEPOT_TOOLS_URL = "https://chromium.googlesource.com/chromium/tools/depot_tools.git"


def executable_command(command: list[str]) -> list[str]:
    """Wrap depot_tools batch commands correctly when invoked from Python."""
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
        help="run checkout/sync/hooks/GN commands instead of only printing them",
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
        help="build the full Chromium browser target after generation",
    )
    args = parser.parse_args()

    version = read_pin()
    workspace = args.workspace.expanduser().resolve()
    depot = workspace / "depot_tools"
    chromium_root = workspace / "chromium"
    src = chromium_root / "src"
    out = src / "out" / "Knogn"

    print(f"Knogn Chromium pin: {version}")
    print(f"Host platform: {platform.system()} {platform.machine()}")
    print(f"External workspace: {workspace}")
    if args.media_experiment:
        print("MEDIA EXPERIMENT: proprietary codecs enabled in generated args; DO NOT DISTRIBUTE by default")

    if args.execute:
        workspace.mkdir(parents=True, exist_ok=True)

    env = os.environ.copy()
    env["PATH"] = str(depot) + os.pathsep + env.get("PATH", "")
    if platform.system() == "Windows":
        # Chromium's public Windows instructions use the locally installed
        # Visual Studio toolchain when this is 0.
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
        run(
            ["fetch", "--nohooks", "--no-history", "chromium"],
            cwd=chromium_root,
            env=env,
            execute=args.execute,
        )
    else:
        print(f"+ Chromium checkout already exists at {src}")

    # Align Chromium and every DEPS-managed repository to the explicit stable
    # version pin before hooks or generated build files are run.
    run(["git", "fetch", "origin", "--tags", "--force"], cwd=src, env=env, execute=args.execute)
    run(["git", "checkout", "--detach", version], cwd=src, env=env, execute=args.execute)
    run(
        ["gclient", "sync", "-D", "--with_branch_heads", "--with_tags"],
        cwd=chromium_root,
        env=env,
        execute=args.execute,
    )

    if platform.system() == "Linux":
        run(
            ["./build/install-build-deps.sh", "--no-prompt"],
            cwd=src,
            env=env,
            execute=args.execute,
        )

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

    if not args.execute:
        print("\nBootstrap dry run complete. Re-run with --execute to perform these steps.")
    else:
        print("\nChromium workspace prepared.")
    print("Public Knogn packages must keep the proprietary-codec experiment disabled until redistribution rights are resolved.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
