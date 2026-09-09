# Threat Model

## Knogn is designed to defend against

- browser-vendor telemetry and behavioral analytics;
- unsolicited browser-owned background network services;
- third-party cookie/state tracking by default;
- accidental persistence of private-window state;
- silent camera/microphone/location permission grants;
- ordinary local-network interface exposure through WebRTC;
- privacy regressions introduced during product development;
- unnecessary always-on browser services.

## Knogn reduces, but cannot eliminate

- browser fingerprinting;
- first-party tracking;
- tracking while logged into websites;
- extension-based observation;
- IP-level observation by sites, networks and ISPs;
- compromised or malicious websites operating within their granted capabilities.

## Knogn does not claim to defend against

- a compromised operating system;
- kernel/hypervisor malware;
- endpoint malware reading browser memory;
- a hostile physical administrator;
- traffic correlation against a user who is not using an anonymity network.

## Security rules

- Chromium renderer sandboxing stays enabled.
- Privacy is not an excuse to suppress engine security updates; Knogn must track Qt/Chromium security releases aggressively.
- No feature weakens a privacy boundary merely to improve a benchmark.
- New speculation, prefetch, preconnect or remote services require privacy review.
