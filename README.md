# Knogn Browser

**Fast by omission. Private by design.**

Knogn is a privacy-first desktop browser. The 0.2.x line established the product shell, privacy model, settings, bookmarks, login storage, network controls, packaging and branding. The project is now moving to a **full Chromium browser backend** for the 0.3.x line.

The engine change is deliberate. Embedded Qt WebEngine proved too limiting for two daily-driver requirements Knogn considers non-negotiable:

- mainstream federated account login such as Google, Microsoft, Apple and GitHub;
- mainstream video/media playback including H.264/AAC, Media Source Extensions, Plex scenarios and Widevine where an approved CDM path exists.

Knogn is not building a rendering engine from scratch. It tracks a pinned upstream Chromium stable source tree, applies a small Knogn product/privacy overlay, and builds the complete browser target.

## Current engine status

**Primary development backend: full Chromium.**

**Compatibility fallback: Qt WebEngine 6.11.**

Knogn **0.2.1 is the final Qt-WebEngine compatibility line**. Qt remains buildable while the Chromium backend reaches package parity, but new browser capability targets Chromium first. The first public **0.3.x** release must be a Chromium-backed build; it will not silently fall back to Qt.

See [`docs/ENGINE_MIGRATION.md`](docs/ENGINE_MIGRATION.md) and [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md).

## Why full Chromium

A full Chromium browser gives Knogn control over the parts that matter for mainstream compatibility:

- browser identity rather than an embedded user-agent;
- Chromium-native OAuth/OIDC/FedCM/WebAuthn surfaces;
- Chromium-native tabs, profiles, bookmarks, history, permissions, downloads, password services and extensions;
- compile-time media capability including the H.264/AAC experiment path;
- Widevine key-system support without pretending the CDM itself is open source;
- browser networking, sandboxing and site-isolation behavior;
- platform-native Windows/macOS/Linux packaging paths.

Knogn keeps Google Chrome branding disabled and does not use official Google API keys. Browser-level Google OAuth client credentials are intentionally empty: Knogn needs normal **website login**, not Chrome Sync or a Google account coupled to the browser itself.

## Chromium development

The Chromium checkout lives outside this repository because the source tree and build output are very large.

Prepare the pinned Chromium workspace:

```bash
python3 tools/build_knogn.py prepare
```

Build Knogn's full Chromium browser target:

```bash
python3 tools/build_knogn.py build
```

Probe the built browser for identity and media capabilities:

```bash
python3 tools/build_knogn.py probe
```

Run the local H.264/AAC media-parity experiment:

```bash
python3 tools/build_knogn.py build --media-experiment
python3 tools/build_knogn.py probe --require-media
```

The proprietary-codec experiment is **not approved for public redistribution** until codec redistribution rights are resolved.

The legacy Qt fallback now requires an explicit command:

```bash
python3 tools/build_knogn.py qt-fallback
```

## Chromium overlay

`engine/chromium/bootstrap.py` uses Chromium's depot_tools/fetch/gclient workflow and applies `engine/chromium/knogn_overlay.py` before GN generation.

The current overlay:

- rewrites Chromium product branding metadata to Knogn;
- installs the approved Knogn Windows icon into Chromium's open-source branding path;
- replaces the base Chromium product SVG with the Knogn mark;
- changes Chromium MetricsReporting to disabled by default;
- leaves Google Chrome branding disabled;
- leaves official Google API keys disabled;
- leaves browser-level Google OAuth credentials empty.

The overlay is intentionally small and fail-fast. Larger downstream changes should become reviewable Chromium patch files rather than an ever-growing text-rewrite script.

## Identity acceptance

Issue #10 tracks federated identity parity. The Chromium backend is not considered ready until released builds demonstrate representative:

- Google website login;
- third-party **Sign in with Google**;
- Microsoft OAuth/OIDC;
- Apple sign-in;
- GitHub OAuth;
- FedCM flows where used;
- passkeys/WebAuthn.

Knogn does not spoof Chrome to bypass provider security checks.

## Media acceptance

Issue #7 tracks media parity. The Chromium backend is not considered ready until released builds demonstrate:

- H.264/AVC + AAC playback;
- H.264/AAC through Media Source Extensions;
- MP3, FLAC, Opus/Vorbis, VP9 and AV1 where supported;
- Plex direct-play and transcoded playback;
- Widevine playback where a legitimate CDM installation/distribution path exists.

The normal Chromium profile compiles Widevine key-system support but does **not** bundle a Widevine CDM. Local proprietary-codec experiments use Chromium's `proprietary_codecs`/FFmpeg branding path and are kept out of public artifacts until licensing is resolved.

## Privacy boundary

Knogn aims to eliminate **browser-owned tracking and unsolicited browser-owned network traffic**. That does not make the user anonymous. Websites, search providers, ISPs, logged-in services, extensions and network observers can still learn information about a browsing session.

Hard requirements include:

- no Knogn telemetry/analytics endpoint;
- Chromium MetricsReporting disabled by default;
- no browser-level Google account requirement;
- normal website authentication remains functional;
- sandboxing, site isolation and Chromium multiprocess security stay enabled;
- private profiles remain ephemeral;
- third-party-state protections are not globally disabled to fix login compatibility;
- performance work cannot trade away security or compatibility.

See [`docs/PRIVACY.md`](docs/PRIVACY.md) and [`docs/THREAT_MODEL.md`](docs/THREAT_MODEL.md).

## 0.2.x compatibility features

The Qt compatibility line already includes:

- Knogn green branding and Windows desktop icon/installer identity;
- tabs, omnibox/search, navigation and downloads;
- persistent normal and memory-only private profiles;
- third-party-state blocking and identity-provider exceptions;
- DNT and GPC signals;
- settings for search, home page, downloads, privacy, passwords and network routing;
- bookmarks/favorites with import from Chromium-family stores and standard bookmark HTML;
- OS-keychain-backed saved logins and browser password CSV import;
- system-VPN routing plus HTTP/SOCKS tunnel settings;
- Chrome/Chromium Manifest V3 extension installation;
- `knogn://media` and `knogn://identity` diagnostics;
- Windows, Linux and macOS installers/portable packages.

These features are migration requirements for Chromium rather than reasons to keep extending the Qt shell.

## CI and heavy engine builds

Hosted CI validates privacy, source, performance, media, identity, branding and Chromium-backend contracts. The Qt fallback remains compiled on all three desktop systems during migration.

A full Chromium source build runs through `.github/workflows/chromium-engine.yml` on dedicated self-hosted capacity with the `knogn-chromium` label. A persistent workspace is expected because Chromium source and object output are too large for normal ephemeral PR runners.

The engine workflow can build either the public open-codec baseline or the local media experiment. Media-experiment output is never uploaded by that workflow.

## Performance

Knogn is **not yet proven faster than Chrome**. Earlier 0.1 testing showed Chrome Incognito ahead in raw throughput on the same system. That remains a benchmark Knogn must beat through measured engineering rather than marketing language.

Full Chromium removes the Qt embedding layer as a confounding variable. Performance will be measured across startup, network throughput, memory, CPU, page responsiveness and battery behavior while preserving browser security and compatibility.

See [`docs/PERFORMANCE.md`](docs/PERFORMANCE.md).

## Product principle

> Your browser should know you. Nobody else should.

Knogn favors local state, explicit user action, inspectable behavior and doing less unnecessary work.

## License

Knogn-owned code is MIT licensed. Chromium and third-party components retain their upstream licenses. Codec and DRM distribution rights are handled separately from the Knogn source-code license.
