#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "src"

source = "\n".join(p.read_text(encoding="utf-8") for p in SRC.glob("*.*"))
settings = (SRC / "appsettings.cpp").read_text(encoding="utf-8")
profile = (SRC / "privacyprofile.cpp").read_text(encoding="utf-8")

required = {
    "component update disabled": "--disable-component-update",
    "domain reliability disabled": "--disable-domain-reliability",
    "sync disabled": "--disable-sync",
    "crash reporter disabled": "--disable-breakpad",
    "navigation pings disabled": "--no-pings",
    "push service disabled": "setPushServiceEnabled(false)",
    "third-party state setting wired": "AppSettings::blockThirdPartyState" in profile,
    "third-party state filter honors setting": "!blockThirdParty || !request.thirdParty" in profile,
    "third-party state blocked by default": 'privacy/blockThirdPartyState"), true' in settings,
    "DNS prefetch disabled": "DnsPrefetchEnabled, false",
    "hyperlink auditing disabled": "HyperlinkAuditingEnabled, false",
    "WebRTC interface restriction": "WebRTCPublicInterfacesOnly, true",
    "GPC preference": '"Sec-GPC"',
    "DNT preference": '"DNT"',
}

missing = []
for name, test in required.items():
    if isinstance(test, bool):
        if not test:
            missing.append(name)
    elif test not in source:
        missing.append(name)
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
