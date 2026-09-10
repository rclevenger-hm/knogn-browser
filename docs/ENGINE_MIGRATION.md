# Engine migration: Qt WebEngine → full Chromium

Knogn 0.2.1 is the final Qt-WebEngine compatibility line. Development after that point targets a full Chromium browser backend. Qt remains in the repository only as a fallback while Chromium reaches feature and packaging parity.

## Why the switch is mandatory

Two daily-driver requirements hit hard boundaries in the embedded Qt backend:

1. **Federated login.** Providers such as Google can reject embedded user-agents independently of the Chromium renderer underneath them. Knogn must look and behave like a normal general-purpose browser, not an embedded browser widget.
2. **Mainstream media.** H.264/AAC, MSE behavior and Widevine integration need control over the browser engine build and distribution. Qt WebEngine does not give Knogn enough control over that release surface.

## Primary development commands

Chromium is now the default development path:

```bash
python3 tools/build_knogn.py prepare
python3 tools/build_knogn.py build
python3 tools/build_knogn.py probe
```

For a local H.264/AAC parity experiment:

```bash
python3 tools/build_knogn.py build --media-experiment
python3 tools/build_knogn.py probe --require-media
```

The media experiment is for local engineering validation only until redistribution rights for the enabled codecs are explicitly resolved.

The old Qt shell requires an explicit fallback command:

```bash
python3 tools/build_knogn.py qt-fallback
```

## Chromium source strategy

Knogn does **not** vendor the Chromium source tree. `engine/chromium/bootstrap.py` maintains an external pinned Chromium checkout using depot_tools and gclient, then applies Knogn's source overlay before GN generation.

The overlay currently:

- replaces Chromium product branding metadata with Knogn;
- replaces the Windows Chromium icon with the approved Knogn icon;
- substitutes the Knogn SVG where Chromium's base product SVG is used;
- changes Chromium's MetricsReporting feature default to disabled;
- leaves Google Chrome branding disabled;
- leaves official Google API keys disabled;
- deliberately leaves browser-level Google OAuth client credentials empty.

The last point is intentional. Knogn needs ordinary **website** login such as “Sign in with Google”; it does not need or want restricted Chrome browser-sync authentication. Website identity compatibility and browser-level Chrome Sync are separate concerns.

## Media configuration

The normal source profile keeps the open Chromium codec baseline but compiles the Widevine key-system hook:

- `enable_widevine = true`
- `proprietary_codecs = false`
- `ffmpeg_branding = "Chromium"`

No Widevine CDM is bundled by that setting.

`--media-experiment` changes only the codec experiment knobs:

- `proprietary_codecs = true`
- `ffmpeg_branding = "Chrome"`

The `Chrome` value here is Chromium's FFmpeg codec-branding selector; it does not turn the product into a Google Chrome branded build.

## Runtime acceptance

`engine/chromium/probe_runtime.py` runs the built browser against a local, network-free probe page and fails if the browser still identifies as Qt WebEngine or lacks basic browser credential/WebAuthn surfaces. With `--require-media`, it additionally requires H.264/AAC and H.264/AAC MSE capability.

The automated probe is necessary but not sufficient. Promotion of the Chromium backend requires manual/automated scenario coverage for:

- Google website login and third-party “Sign in with Google” flows;
- Microsoft, Apple and GitHub OAuth/OIDC flows;
- FedCM where used;
- passkeys/WebAuthn;
- Plex direct-play and transcoded playback;
- representative H.264/AAC HTML5 and MSE streams;
- Widevine-protected media where a legitimate CDM path is available;
- startup/network privacy and zero Knogn-owned telemetry;
- performance relative to the Qt shell and mainstream browsers.

## Packaging gate

The existing Qt packages remain available only as the 0.2.x compatibility line. The first 0.3.x public release must be built from the Chromium backend. We will not ship a 0.3.x package that silently falls back to Qt.

Chromium becomes the only desktop backend once Windows, macOS and Linux Chromium packages pass the privacy, identity, media, performance and runtime-content gates.
