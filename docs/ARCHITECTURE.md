# Architecture

## 1. Primary desktop engine: full Chromium

Knogn 0.3.x and later target the complete open-source Chromium browser, not Qt WebEngine embedded inside a separate shell. This is required for mainstream browser identity, media, extension, profile, networking and platform integration behavior.

Chromium itself owns the native browser window/tab/omnibox stack, renderer processes, network service, content settings, permissions, downloads, extensions and profile infrastructure. Knogn customizes those layers through a small source overlay and downstream patches rather than rebuilding a parallel browser shell around WebEngine.

The Chromium checkout is deliberately external to this repository. `engine/chromium/bootstrap.py` prepares a pinned upstream source tree using depot_tools/gclient, applies Knogn changes, generates `out/Knogn`, and optionally builds the full `chrome` target.

## 2. Knogn source overlay

`engine/chromium/knogn_overlay.py` is the first downstream customization layer. It currently owns:

- product branding metadata;
- base Windows application icon;
- base product SVG;
- metrics-reporting default state.

The overlay must fail loudly when upstream source structure changes rather than silently skipping privacy or branding changes. As the fork grows, larger behavioral changes should move into reviewable patch files grouped by purpose instead of making the overlay an unbounded text-rewrite script.

## 3. Privacy model

Knogn keeps Chromium's sandbox, site isolation, multiprocess architecture and web-platform compatibility. Privacy work removes or disables browser-owned tracking/network services without pretending ordinary websites should stop functioning.

Key rules:

- no Knogn telemetry or analytics endpoint;
- metrics reporting disabled by default at the browser source layer;
- no official Google API keys;
- no browser-level Google OAuth client ID/secret;
- ordinary Google/Microsoft/Apple/GitHub website authentication remains supported;
- normal profiles persist locally; private profiles must be ephemeral;
- third-party-state protections must not be globally disabled merely to fix federated login;
- privacy regressions are validated by network and source contracts.

## 4. Identity model

Website identity and browser-account identity are separate.

Knogn must support normal standards-based web login, including OAuth/OIDC, FedCM and WebAuthn/passkeys. It does not require Chrome Sync or a Google account attached to the browser itself. The Chromium build therefore intentionally omits Google browser OAuth credentials while retaining the ordinary web platform used by identity providers.

The Qt 0.2.x bridge remains useful for comparison but is not the target for identity parity because providers can classify embedded user-agents differently from a full browser.

## 5. Media model

The full Chromium build gives Knogn direct control over media compile-time capability.

Baseline public configuration:

- open Chromium codec profile;
- Widevine key-system support compiled in;
- no bundled Widevine CDM;
- no proprietary-codec redistribution assumption.

Local media experiment configuration enables Chromium's proprietary codec path for H.264/AAC validation. It is not publishable until redistribution rights are resolved.

`engine/chromium/probe_runtime.py` verifies the built browser's actual H.264/AAC, MSE, EME, credential, FedCM-surface and WebAuthn behavior rather than trusting build flags alone.

## 6. Product services

Bookmarks, history, passwords, settings, permissions, downloads, sessions and extensions should increasingly use Chromium's mature native browser services instead of maintaining parallel Qt implementations. Knogn-specific defaults and UX are layered over those services.

The secure-password design remains local-first. Platform credential/keychain integration and Chromium password-store integration must not introduce a Knogn cloud dependency.

## 7. Performance strategy

Knogn should win by removing unnecessary product services, not by disabling browser security or web compatibility.

- preserve Chromium sandbox/site isolation/multiprocess boundaries;
- remove browser-owned telemetry/sync/recommendation work that Knogn does not offer;
- keep optional Knogn features lazy;
- benchmark startup, network throughput, memory, CPU, page responsiveness and battery use against Chrome/Firefox/Edge;
- test representative media and identity flows as part of performance work so 'fast' does not mean 'broken.'

## 8. Build and CI strategy

Normal hosted CI performs source/contract validation and keeps the Qt fallback buildable during migration. Full Chromium source/build work runs on dedicated self-hosted capacity with persistent disk because Chromium checkout and object output are too large for ordinary ephemeral PR runners.

The first 0.3.x release is blocked until Chromium packages exist for Windows, Linux and macOS and pass privacy, identity, media, performance and runtime-content validation.

## 9. Qt compatibility fallback

The Qt Widgets / Qt WebEngine implementation is frozen as the 0.2.x compatibility line. It may receive regression fixes for existing users, but new product capability should target Chromium first. Once the Chromium backend reaches packaging parity, the Qt implementation can be removed from the primary build/release path entirely.
