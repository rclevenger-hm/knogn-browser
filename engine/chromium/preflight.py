#!/usr/bin/env python3
"""Fail fast when a host cannot reasonably build Knogn's full Chromium backend."""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import platform
import shutil
import sys

GIB = 1024 ** 3


def gib(value: int) -> float:
    return value / GIB


def total_memory_bytes() -> int | None:
    if sys.platform.startswith("linux"):
        try:
            for line in Path("/proc/meminfo").read_text(encoding="utf-8").splitlines():
                if line.startswith("MemTotal:"):
                    return int(line.split()[1]) * 1024
        except (OSError, ValueError, IndexError):
            return None
    if sys.platform == "darwin":
        try:
            import subprocess
            return int(subprocess.check_output(["sysctl", "-n", "hw.memsize"], text=True).strip())
        except (OSError, ValueError):
            return None
    if os.name == "nt":
        try:
            import ctypes

            class MemoryStatus(ctypes.Structure):
                _fields_ = [
                    ("dwLength", ctypes.c_ulong),
                    ("dwMemoryLoad", ctypes.c_ulong),
                    ("ullTotalPhys", ctypes.c_ulonglong),
                    ("ullAvailPhys", ctypes.c_ulonglong),
                    ("ullTotalPageFile", ctypes.c_ulonglong),
                    ("ullAvailPageFile", ctypes.c_ulonglong),
                    ("ullTotalVirtual", ctypes.c_ulonglong),
                    ("ullAvailVirtual", ctypes.c_ulonglong),
                    ("sullAvailExtendedVirtual", ctypes.c_ulonglong),
                ]

            status = MemoryStatus()
            status.dwLength = ctypes.sizeof(status)
            if ctypes.windll.kernel32.GlobalMemoryStatusEx(ctypes.byref(status)):
                return int(status.ullTotalPhys)
        except (AttributeError, OSError):
            return None
    return None


def main() -> int:
    parser = argparse.ArgumentParser(description="Validate a Knogn Chromium build host")
    parser.add_argument("--workspace", type=Path, default=Path.home() / "knogn-chromium")
    parser.add_argument("--min-disk-gib", type=float, default=100.0)
    parser.add_argument("--min-memory-gib", type=float, default=16.0)
    parser.add_argument("--skip-tools", action="store_true", help="skip executable checks for unit/contract use")
    args = parser.parse_args()

    workspace = args.workspace.expanduser().resolve()
    probe_path = workspace
    while not probe_path.exists() and probe_path != probe_path.parent:
        probe_path = probe_path.parent

    failures: list[str] = []
    warnings: list[str] = []

    machine = platform.machine().lower()
    if machine not in {"x86_64", "amd64"}:
        failures.append(f"unsupported primary build architecture: {platform.machine()} (expected x86-64)")

    usage = shutil.disk_usage(probe_path)
    free_gib = gib(usage.free)
    if free_gib < args.min_disk_gib:
        failures.append(f"free disk {free_gib:.1f} GiB < required {args.min_disk_gib:.1f} GiB")

    memory = total_memory_bytes()
    if memory is None:
        warnings.append("unable to determine physical memory")
        memory_gib = None
    else:
        memory_gib = gib(memory)
        if memory_gib < args.min_memory_gib:
            failures.append(f"physical memory {memory_gib:.1f} GiB < Knogn build minimum {args.min_memory_gib:.1f} GiB")
        elif memory_gib < 32:
            warnings.append("less than 32 GiB RAM: Chromium build will be slower and may require substantial swap")

    if not args.skip_tools:
        for tool in ("git", "python3"):
            if shutil.which(tool) is None:
                failures.append(f"required tool not found on PATH: {tool}")
        if platform.system() == "Linux" and shutil.which("lsb_release") is None:
            warnings.append("lsb_release not found; Chromium dependency setup may need it")

    print(f"host={platform.system()} {platform.machine()}")
    print(f"workspace={workspace}")
    print(f"free_disk_gib={free_gib:.1f}")
    print(f"memory_gib={memory_gib:.1f}" if memory_gib is not None else "memory_gib=unknown")
    for warning in warnings:
        print(f"warning: {warning}")

    if failures:
        for failure in failures:
            print(f"error: {failure}", file=sys.stderr)
        return 1

    print("Chromium build host preflight: pass")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
