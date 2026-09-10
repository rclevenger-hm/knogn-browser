#!/usr/bin/env python3
"""Apply Knogn product/privacy overlay to a checked-out Chromium source tree."""

from __future__ import annotations

import argparse
from pathlib import Path
import shutil

HERE = Path(__file__).resolve().parent
REPO_ROOT = HERE.parents[1]


def replace_once(path: Path, old: str, new: str) -> None:
    text = path.read_text(encoding="utf-8")
    if old not in text:
        raise SystemExit(f"Expected source pattern not found in {path}: {old!r}")
    path.write_text(text.replace(old, new, 1), encoding="utf-8")


def rewrite_branding(path: Path) -> None:
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
    lines = path.read_text(encoding="utf-8").splitlines()
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
    path.write_text("\n".join(output) + "\n", encoding="utf-8")


def apply(src: Path) -> None:
    src = src.resolve()
    branding = src / "chrome/app/theme/chromium/BRANDING"
    metrics = src / "chrome/browser/metrics/chrome_metrics_services_manager_client.cc"
    windows_icon = src / "chrome/app/theme/chromium/win/chromium.ico"
    product_svg = src / "chrome/app/theme/chromium/product_logo.svg"

    required = [branding, metrics, windows_icon]
    missing = [str(path) for path in required if not path.exists()]
    if missing:
        raise SystemExit("Not a compatible Chromium source tree; missing: " + ", ".join(missing))

    rewrite_branding(branding)

    metric_enabled = '''BASE_FEATURE(kMetricsReportingFeature,\n             "MetricsReporting",\n             base::FEATURE_ENABLED_BY_DEFAULT);'''
    metric_disabled = '''BASE_FEATURE(kMetricsReportingFeature,\n             "MetricsReporting",\n             base::FEATURE_DISABLED_BY_DEFAULT);'''
    text = metrics.read_text(encoding="utf-8")
    if metric_disabled not in text:
        replace_once(metrics, metric_enabled, metric_disabled)

    shutil.copy2(REPO_ROOT / "assets/knogn.ico", windows_icon)
    source_svg = REPO_ROOT / "site/assets/knogn-mark.svg"
    if product_svg.exists() and source_svg.exists():
        shutil.copy2(source_svg, product_svg)

    marker = src / ".knogn-overlay"
    marker.write_text(
        "Knogn Chromium source overlay applied.\n"
        "Browser-level Google OAuth credentials intentionally remain unset.\n"
        "Metrics reporting feature default: disabled.\n",
        encoding="utf-8",
    )
    print(f"Knogn overlay applied to {src}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("src", type=Path, help="Chromium src directory")
    args = parser.parse_args()
    apply(args.src)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
