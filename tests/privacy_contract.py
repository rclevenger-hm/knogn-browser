#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "src"

source = "\n".join(p.read_text(encoding="utf-8") for p in SRC.glob("*.*"))

required = {
    "component update disabled": "--disable-component-update",
    "domain reliability disabled": "--disable-domain-reliability",
    "sync disabled": "--disable-sync",
    "crash reporter disabled": "--disable-breakpad",
    "navigation pings disabled": "--no-pings",
    "push service disabled": "setPushServiceEnabled(false)",
    "third-party state filter": "return !request.thirdParty",
    "DNS prefetch disabled": "DnsPrefetchEnabled, false",
    "hyperlink auditing disabled": "HyperlinkAuditingEnabled, false",
    "WebRTC interface restriction": "WebRTCPublicInterfacesOnly, true",
    "GPC preference": '"Sec-GPC"',
    "DNT preference": '"DNT"',
}

missing = [name for name, needle in required.items() if needle not in source]
if missing:
    raise SystemExit("Missing privacy invariants: " + ", ".join(missing))

for forbidden in (
    "google-analytics.com",
    "analytics.google.com",
    "segment.io",
    "sentry.io",
    "mixpanel.com",
    "amplitude.com",
):
    if forbidden in source.lower():
        raise SystemExit(f"Forbidden telemetry endpoint in source: {forbidden}")

print(f"privacy contract: {len(required)} targeted invariants present; no telemetry endpoints found")
