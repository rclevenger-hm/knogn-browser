#!/usr/bin/env python3
"""Generate tiny synthetic playback fixtures locally, with no downloaded media."""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import subprocess

from probe_runtime import sha256_file


def generate(directory: Path, ffmpeg: str, include_proprietary: bool) -> dict:
    directory.mkdir(parents=True, exist_ok=True)
    manifest_path = directory / "fixtures.json"
    manifest_path.unlink(missing_ok=True)
    base = [ffmpeg, "-hide_banner", "-loglevel", "error", "-y",
            "-f", "lavfi", "-i", "testsrc2=size=160x90:rate=24",
            "-f", "lavfi", "-i", "sine=frequency=440:sample_rate=48000",
            "-t", "2", "-threads", "1"]
    encodings = {"sample.webm": ["-c:v", "libvpx-vp9", "-b:v", "150k", "-c:a", "libopus"]}
    if include_proprietary:
        mp4 = ["-c:v", "libx264", "-profile:v", "baseline", "-level", "3.0", "-pix_fmt", "yuv420p",
               "-g", "24", "-c:a", "aac", "-b:a", "64k"]
        encodings["sample.mp4"] = mp4 + ["-movflags", "+faststart"]
        encodings["sample-fragmented.mp4"] = mp4 + ["-movflags", "+frag_keyframe+empty_moov+default_base_moof"]
    for filename, encoding in encodings.items():
        subprocess.run(base + encoding + [str(directory / filename)], check=True, timeout=60)
    version = subprocess.check_output([ffmpeg, "-version"], text=True, timeout=10).splitlines()[0]
    manifest = {"schemaVersion": 1, "generator": version, "synthetic": True,
                "includesProprietary": include_proprietary,
                "files": {name: sha256_file(directory / name) for name in encodings}}
    manifest_path.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    return manifest


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--ffmpeg", default="ffmpeg")
    parser.add_argument("--include-proprietary", action="store_true", help="generate local H.264/AAC experiments too")
    args = parser.parse_args()
    print(json.dumps(generate(args.out, args.ffmpeg, args.include_proprietary), indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
