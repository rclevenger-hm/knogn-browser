# Knogn Browser

**Fast by omission. Private by design.**

Knogn is an experimental privacy-first desktop browser built around a native Qt shell and Qt WebEngine. The project starts from a simple premise: a browser should know enough to serve its owner without turning that knowledge into telemetry, profiling, advertising data, or mandatory cloud state.

Knogn is not attempting to build a new web rendering engine in v1. It uses a maintained Chromium-derived engine through Qt WebEngine for site compatibility while keeping the browser product, UI, privacy policy, storage behavior, extension UX, and performance controls under our control.

## Current status

Knogn is privacy-first and performance-focused, but **it is not yet proven faster than Chrome**. Initial 0.1.0 testing found Chrome Incognito ahead in raw network throughput on the same system. That result is being treated as a performance defect rather than hidden behind a marketing claim.

Version 0.1.1 begins the measured performance work by removing an unnecessarily broad Chromium networking restriction and enabling Qt WebEngine's disabled-by-default back/forward cache while retaining the actual privacy controls. See [`docs/PERFORMANCE.md`](docs/PERFORMANCE.md) for the benchmark protocol and performance guardrails.

## Milestone 0.1 / 0.1.1

The runnable browser currently includes:

- native Qt Widgets browser chrome;
- tabs, omnibox/search, navigation, pop-up/new-tab handling and downloads;
- normal and private windows;
- memory-only off-the-record private profiles;
- third-party cookie/state blocking by default;
- disabled Qt/Chromium push service;
- disabled DNS prefetch, hyperlink auditing, browser sync, component updates, domain reliability and crash reporting;
- restricted WebRTC interface exposure;
- session-scoped permission decisions;
- `DNT: 1` and `Sec-GPC: 1` preference headers;
- back/forward cache enabled for faster history navigation;
- automatic use of Qt WebEngine lifecycle recommendations to freeze/discard safe background tabs;
- Chrome/Chromium Manifest V3 extension installation for normal profiles;
- explicit disabling of the built-in Hangouts extension;
- privacy, source and performance contract tests;
- Windows, Linux and macOS build CI;
- native installer and portable-package generation for all three desktop platforms.

## Install packages

The `package-installers` workflow produces self-contained Qt WebEngine distributions on each native runner:

- **Windows x86-64:** NSIS `.exe` installer and portable `.zip`;
- **macOS Apple Silicon:** `.dmg` disk image and portable `.zip`;
- **Linux x86-64:** Debian `.deb` package and portable `.tar.xz`.

Every platform artifact set includes its own SHA-256 checksum manifest. The pipeline also validates that the portable package contains Knogn, `QtWebEngineProcess`, and the required WebEngine resources before upload. New application versions merged to `main` are published automatically as versioned GitHub Releases.

Current packages are unsigned development builds. Platform code signing/notarization is a separate release-hardening step; until signing is configured, Windows SmartScreen and macOS Gatekeeper may warn when launching downloaded builds.

See [`docs/RELEASING.md`](docs/RELEASING.md) for the release pipeline and artifact policy.

## Extension compatibility

### Chrome / Chromium

Knogn's first engine supports zipped and unpacked **Manifest V3** Chrome extensions through Qt WebEngine 6.11. Extensions are installed per normal profile. Private/off-the-record windows intentionally do not load user extensions because Qt WebEngine does not permit them there.

### Firefox

Firefox WebExtensions are a planned compatibility layer, not a claim that arbitrary `.xpi` files work today. Many modern Firefox extensions share the WebExtensions model with Chromium, but Firefox-specific APIs and background semantics must be inspected and translated where possible. See [`docs/EXTENSIONS.md`](docs/EXTENSIONS.md).

## Privacy boundary

Knogn aims to eliminate **browser-owned tracking and unsolicited browser-owned network traffic**. That does not make the user anonymous. Websites, search providers, ISPs, logged-in services, extensions and network observers can still learn information about a browsing session.

The promises we are willing to make are documented in [`docs/PRIVACY.md`](docs/PRIVACY.md) and [`docs/THREAT_MODEL.md`](docs/THREAT_MODEL.md). Privacy-sensitive defaults are also asserted by tests so regressions are visible in CI.

## Build

### Requirements

- CMake 3.22+
- C++20 compiler
- Qt 6.11+ with `Widgets`, `WebEngineWidgets`, and `WebEngineCore`
- Ninja is recommended but not required

### Linux

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
./build/Knogn
```

### macOS

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
open build/Knogn.app
```

### Windows

```powershell
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
.\build\Release\Knogn.exe
```

Start directly in a memory-only private profile with:

```bash
Knogn --private
```

## Development checks

```bash
python3 tests/privacy_contract.py
python3 tests/source_contract.py
python3 tests/performance_contract.py
```

On a development system with Qt installed, `tools/check.sh` runs the contracts and performs a release build.

## Architecture

Knogn is deliberately layered so the shell does not become inseparable from one renderer:

```text
Native browser shell
        │
Browser services / local state
        │
Privacy + security policy
        │
Engine adapter
        │
Qt WebEngine / Chromium (v1)
        │
Future experimental adapters (e.g. Servo)
```

See [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) and the ADRs under [`docs/adr`](docs/adr).

## Product principle

> Your browser should know you. Nobody else should.

Knogn will prefer local features, explicit user actions, inspectable behavior, and performance that comes from doing less unnecessary work—not from silently weakening security.

## License

MIT. See [`LICENSE`](LICENSE).
