# Knogn Browser

**Fast by omission. Private by design.**

Knogn is an experimental privacy-first desktop browser built around a native Qt shell and Qt WebEngine. The project starts from a simple premise: a browser should know enough to serve its owner without turning that knowledge into telemetry, profiling, advertising data, or mandatory cloud state.

Knogn is not attempting to build a new web rendering engine in v1. It uses a maintained Chromium-derived engine through Qt WebEngine for site compatibility while keeping the browser product, UI, privacy policy, storage behavior, extension UX, performance controls, settings, bookmarks, login storage, and network routing under our control.

## Current status

Knogn is moving from an experimental browser shell toward a daily-driver browser. Version **0.2.0** adds the first product-level foundations that were absent from the early 0.1.x builds:

- persistent Settings UI;
- bookmarks/favorites with bookmark bar and manager;
- bookmark import from Chrome, Edge and Brave plus standard bookmark HTML;
- OS-keychain-backed saved login storage and CSV password import;
- manual and optional single-match login autofill;
- configurable home page and search provider template;
- System / OS VPN networking plus direct, HTTP proxy and SOCKS5 routing modes;
- a real embedded Windows executable icon and installer-created desktop shortcut;
- the existing privacy, performance, media, branding and cross-platform packaging contracts.

Knogn is privacy-first and performance-focused, but **it is not yet proven faster than Chrome**. Initial 0.1.0 testing found Chrome Incognito ahead in raw network throughput on the same system. That result is treated as a performance defect rather than hidden behind a marketing claim. See [`docs/PERFORMANCE.md`](docs/PERFORMANCE.md).

Knogn also **does not yet claim Chrome-level media codec/DRM parity**. Browser-side fullscreen/media support and `knogn://media` diagnostics are present, while issue #7 tracks H.264/AAC, MSE, Widevine and related engine/distribution work. See [`docs/MEDIA.md`](docs/MEDIA.md).

For the 0.2.0 feature boundaries and remaining daily-driver work, see [`docs/DAILY_DRIVER.md`](docs/DAILY_DRIVER.md).

## Browser features

The current browser includes:

- native Qt Widgets browser chrome with the Knogn green identity;
- tabs, omnibox/search, navigation, pop-up/new-tab handling and downloads;
- a local branded `knogn://newtab` experience;
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
- persistent settings for home page, search template, bookmarks bar, passwords and network routing;
- local bookmarks/favorites with manager, toolbar and imports;
- secure saved-login secrets delegated to the OS credential store through QtKeychain;
- saved-login CSV import, manual fill and optional single-match autofill;
- System networking / OS VPN mode plus HTTP and SOCKS5 browser-specific tunnel configuration;
- `knogn://media` runtime media capability diagnostics;
- privacy, source, performance, media, branding and daily-driver contract tests;
- Windows, Linux and macOS build CI;
- native installer and portable-package generation for all three desktop platforms.

## Install packages

The `package-installers` workflow produces self-contained Qt WebEngine distributions on each native runner:

- **Windows x86-64:** NSIS `.exe` installer and portable `.zip`;
- **macOS Apple Silicon:** `.dmg` disk image and portable `.zip`;
- **Linux x86-64:** Debian `.deb` package and portable `.tar.xz`.

Every platform artifact set includes its own SHA-256 checksum manifest. The pipeline validates that the portable package contains Knogn, `QtWebEngineProcess`, and the required WebEngine resources before upload. New application versions merged to `main` are published automatically as versioned GitHub Releases.

Current packages are unsigned development builds. Platform code signing/notarization remains a release-hardening step; until signing is configured, Windows SmartScreen and macOS Gatekeeper may warn when launching downloaded builds.

See [`docs/RELEASING.md`](docs/RELEASING.md).

## Bookmarks and browser import

Use **Bookmarks → Add Bookmark** or `Ctrl+D` to save the current page. Bookmarks persist locally and can be displayed on the bookmark bar.

**Bookmarks → Import Bookmarks** can automatically detect the default bookmark stores for Chrome, Edge and Brave when present. Knogn can also import Chromium `Bookmarks` JSON files and standard bookmark HTML exported by Firefox or other browsers.

## Saved logins

Saved login secrets are not written to Knogn's settings or bookmark files. QtKeychain delegates password storage to the platform credential service with insecure fallback disabled. Knogn stores only the site/username index and the generated credential key locally.

Use **Passwords → Manage Passwords** to add/delete logins, **Import Passwords CSV** for browser-exported password CSV files, and **Fill Saved Login** on a site. Private windows do not expose saved logins.

The 0.2.0 password manager is deliberately a foundation. Automatic login capture, passkeys, generated-password UX, richer form heuristics and breach monitoring are future work.

## Network and VPN settings

Knogn does not disguise a proxy as a VPN.

- **System networking / OS VPN** follows the operating system's route and proxy settings. If the machine is connected to a VPN, Knogn follows that route.
- **Direct connection** disables application proxy routing.
- **HTTP proxy / tunnel** and **SOCKS5 proxy / tunnel** route Knogn through an explicitly configured endpoint.

A true built-in VPN provider requires an actual tunnel implementation or provider service/driver and authentication layer. That is a separate future feature.

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
- Git (QtKeychain is fetched at configure time)
- C++20 compiler
- Qt 6.11+ with `Widgets`, `Network`, `WebEngineWidgets`, and `WebEngineCore`
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
python3 tests/media_contract.py
python3 tests/branding_contract.py
python3 tests/daily_driver_contract.py
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
Future experimental adapters
```

See [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) and the ADRs under [`docs/adr`](docs/adr).

## Product principle

> Your browser should know you. Nobody else should.

Knogn will prefer local features, explicit user actions, inspectable behavior, and performance that comes from doing less unnecessary work—not from silently weakening security.

## License

MIT. See [`LICENSE`](LICENSE).
