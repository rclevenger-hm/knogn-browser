#!/usr/bin/env python3
"""Apply Knogn product/privacy overlay to a checked-out Chromium source tree."""

from __future__ import annotations

import argparse
import base64
import hashlib
import json
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO_ROOT = HERE.parents[1]
MARKER = ".knogn-overlay"
MANAGED_PATHS = {
    "chrome/app/theme/chromium/BRANDING",
    "chrome/browser/metrics/chrome_metrics_services_manager_client.cc",
    "chrome/app/theme/chromium/win/chromium.ico",
    "chrome/app/theme/chromium/product_logo.svg",
}


def rewrite_branding(text: str) -> str:
    replacements = {
        "COMPANY_FULLNAME": "Knogn",
        "COMPANY_SHORTNAME": "Knogn",
        "PRODUCT_FULLNAME": "Knogn",
        "PRODUCT_SHORTNAME": "Knogn",
        "PRODUCT_INSTALLER_FULLNAME": "Knogn Installer",
        "PRODUCT_INSTALLER_SHORTNAME": "Knogn Installer",
        "COPYRIGHT": "Copyright 2026 Knogn contributors. Chromium notices retained with upstream sources.",
        "MAC_BUNDLE_ID": "org.knogn.browser",
        "MAC_CREATOR_CODE": "KnGn",
    }
    lines = text.splitlines()
    seen: set[str] = set()
    output: list[str] = []
    for line in lines:
        if "=" not in line:
            output.append(line)
            continue
        key, _ = line.split("=", 1)
        if key in replacements:
            output.append(f"{key}={replacements[key]}")
            seen.add(key)
        else:
            output.append(line)
    for key, value in replacements.items():
        if key not in seen:
            output.append(f"{key}={value}")
    return "\n".join(output) + "\n"


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def verified_manifest(src: Path) -> dict:
    try:
        manifest = json.loads((src / MARKER).read_text(encoding="utf-8"))
        if manifest["schemaVersion"] != 1 or not manifest["files"]:
            raise ValueError("unsupported overlay manifest")
        for relative, entry in manifest["files"].items():
            if relative not in MANAGED_PATHS:
                raise ValueError(f"unmanaged path: {relative}")
            original = base64.b64decode(entry["original"], validate=True)
            if digest(original) != entry["originalSha256"]:
                raise ValueError(f"invalid original content: {relative}")
            if digest((src / relative).read_bytes()) != entry["appliedSha256"]:
                raise ValueError(f"local edits in overlay-managed file: {relative}")
    except (OSError, ValueError, KeyError, TypeError) as error:
        raise SystemExit(f"Cannot safely restore overlay: {error}. Preserve local work and use a clean checkout.") from error
    return manifest


def restore(src: Path) -> None:
    if not (src / MARKER).exists():
        return
    # Validate every file before touching any file. Never reset arbitrary edits.
    manifest = verified_manifest(src)
    for relative, entry in manifest["files"].items():
        (src / relative).write_bytes(base64.b64decode(entry["original"]))
    (src / MARKER).unlink()
    print(f"Restored upstream files in {src}")


def apply(src: Path) -> None:
    src = src.resolve()
    if (src / MARKER).exists():
        verified_manifest(src)
        print(f"Knogn overlay already applied to {src}")
        return
    branding = src / "chrome/app/theme/chromium/BRANDING"
    metrics = src / "chrome/browser/metrics/chrome_metrics_services_manager_client.cc"
    windows_icon = src / "chrome/app/theme/chromium/win/chromium.ico"
    product_svg = src / "chrome/app/theme/chromium/product_logo.svg"

    required = [branding, metrics, windows_icon]
    missing = [str(path) for path in required if not path.exists()]
    if missing:
        raise SystemExit("Not a compatible Chromium source tree; missing: " + ", ".join(missing))

    metric_enabled = '''BASE_FEATURE(kMetricsReportingFeature,\n             "MetricsReporting",\n             base::FEATURE_ENABLED_BY_DEFAULT);'''
    metric_disabled = '''BASE_FEATURE(kMetricsReportingFeature,\n             "MetricsReporting",\n             base::FEATURE_DISABLED_BY_DEFAULT);'''
    text = metrics.read_text(encoding="utf-8")
    if text.count(metric_enabled) != 1:
        raise SystemExit(f"Expected exactly one upstream MetricsReporting feature in {metrics}")
    # Compute and validate the complete overlay before mutating upstream files.
    changes = {
        branding: rewrite_branding(branding.read_text(encoding="utf-8")).encode("utf-8"),
        metrics: text.replace(metric_enabled, metric_disabled, 1).encode("utf-8"),
        windows_icon: (REPO_ROOT / "assets/knogn.ico").read_bytes(),
    }
    source_svg = REPO_ROOT / "site/assets/knogn-mark.svg"
    if product_svg.exists() and source_svg.exists():
        changes[product_svg] = source_svg.read_bytes()
    originals = {path: path.read_bytes() for path in changes}
    manifest = {"schemaVersion": 1, "files": {
        path.relative_to(src).as_posix(): {
            "original": base64.b64encode(originals[path]).decode("ascii"),
            "originalSha256": digest(originals[path]),
            "appliedSha256": digest(content),
        } for path, content in changes.items()
    }}
    try:
        for path, content in changes.items():
            path.write_bytes(content)
        (src / MARKER).write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    except OSError:
        for path, content in originals.items():
            path.write_bytes(content)
        (src / MARKER).unlink(missing_ok=True)
        raise
    print(f"Knogn overlay applied to {src}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("src", type=Path, help="Chromium src directory")
    parser.add_argument("--restore", action="store_true", help="restore only verified, unmodified overlay files")
    args = parser.parse_args()
    (restore if args.restore else apply)(args.src)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
