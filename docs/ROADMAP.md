# Knogn Roadmap

## Milestone 0.1 — runnable privacy shell

- [x] Knogn branding and native shell
- [x] tabs / omnibox / navigation / popups
- [x] downloads and explicit site permission prompts
- [x] persistent local normal profile
- [x] memory-only private profile
- [x] default third-party state blocking
- [x] push, DNS prefetch and hyperlink auditing disabled
- [x] browser background-network/sync/component-update/crash flags
- [x] WebRTC public-interface restriction
- [x] DNT and GPC signals
- [x] background page lifecycle optimization
- [x] Chrome Manifest V3 extension installer
- [x] privacy/source contract tests
- [x] Windows/Linux/macOS CI definition
- [x] Windows/Linux/macOS installer and portable-package pipeline

## Milestone 0.2 — daily-driver foundations (Qt compatibility line)

- [x] persistent Settings UI
- [x] configurable search engines, home page and downloads
- [x] bookmarks/favorites, toolbar and manager
- [x] bookmark import from Chromium-family stores and standard bookmark HTML
- [x] OS-keychain-backed saved login storage
- [x] password CSV import, manual fill and optional autofill
- [x] system VPN routing plus HTTP/SOCKS browser-specific tunnel settings
- [x] Windows executable/installer icon and desktop shortcut integration
- [x] narrow identity-provider state exceptions
- [x] `knogn://identity` diagnostics

Knogn 0.2.1 is the final Qt-WebEngine compatibility line. Remaining browser-product work moves to Chromium unless a Qt-only regression must be repaired for existing 0.2.x users.

## Milestone 0.3 — full Chromium browser backend (ACTIVE)

Chromium is now the primary development target. Qt WebEngine is an explicit fallback only until the Chromium packages pass the production gates.

### Engine and build

- [x] adopt full-browser backend decision (ADR 0004)
- [x] pin an upstream Chromium stable version
- [x] external depot_tools/fetch/gclient bootstrap tooling
- [x] dedicated self-hosted Chromium engine workflow
- [x] Chromium-first `tools/build_knogn.py` developer entrypoint
- [x] Knogn source-overlay framework applied before GN generation
- [x] runtime identity/media probe for built Chromium
- [x] mark Qt builds as compatibility fallback in CI
- [ ] successful dedicated full Chromium build from the pinned source
- [ ] reproducible Windows Chromium build lane
- [ ] reproducible macOS Chromium build lane

### Product identity and privacy

- [x] replace Chromium BRANDING metadata with Knogn
- [x] replace base Windows Chromium icon with Knogn icon
- [x] replace base product SVG with Knogn mark
- [x] disable Chromium MetricsReporting feature by default in source overlay
- [x] keep Google Chrome branding disabled
- [x] keep official Google API keys disabled
- [x] keep browser-level Google OAuth client ID/secret empty
- [ ] port Knogn new-tab/settings/bookmarks/password UX into Chromium-native UI
- [ ] port normal/private profile defaults
- [ ] zero unsolicited Knogn-owned startup traffic proof against Chromium target
- [ ] EasyPrivacy/request-blocking layer on Chromium network stack

### Federated identity acceptance

- [ ] successful direct Google website login
- [ ] successful third-party “Sign in with Google” flow
- [ ] successful Microsoft OAuth/OIDC flow
- [ ] successful Apple sign-in flow
- [ ] successful GitHub OAuth flow
- [ ] FedCM compatibility where sites use it
- [ ] passkeys/WebAuthn UX and end-to-end test

Browser-level Chrome Sync authentication is intentionally not a requirement. Knogn needs standards-based website authentication without inheriting Chrome account coupling.

### Media acceptance

- [x] compile Widevine key-system support hook without bundling a CDM
- [x] local H.264/AAC proprietary-codec experiment configuration
- [x] runtime H.264/AAC/MSE probe
- [ ] H.264/AAC direct playback in full Chromium test build
- [ ] H.264/AAC Media Source Extensions playback
- [ ] Plex direct-play scenario
- [ ] Plex transcoded-stream scenario
- [ ] approved Widevine CDM installation/distribution path
- [ ] representative Widevine playback scenario
- [ ] public codec redistribution/legal review before shipping codec-enabled packages

### Distribution

- [ ] package Chromium backend for Windows x86-64
- [ ] package Chromium backend for macOS ARM64
- [ ] package Chromium backend for Linux x86-64
- [ ] sign/notarize platform builds
- [ ] SBOM and dependency manifest
- [ ] automated upstream Chromium stable tracking/rebase
- [ ] publish Knogn 0.3.x only after Chromium package matrix passes

## Milestone 0.4 — privacy engine

- [ ] EasyList/EasyPrivacy-compatible request blocking
- [ ] query-parameter stripping
- [ ] HTTPS-only mode
- [ ] per-site JavaScript policy
- [ ] anti-fingerprinting modes with breakage tests
- [ ] DNS-over-HTTPS without a mandatory provider
- [ ] proxy/SOCKS/Tor launch profiles
- [ ] local-only privacy dashboard
- [ ] startup network-leak integration harness using a local intercepting proxy
- [ ] enumerate and classify every browser-owned startup request

## Milestone 0.5 — extension breadth

- [ ] extension manager UI
- [ ] permission review before enabling extensions
- [ ] Firefox `.xpi` static compatibility inspector
- [ ] safe Firefox-to-MV3 normalization for supported cases
- [ ] clear reporting for unsupported Firefox APIs
- [ ] optional extension store browsing without a Knogn account

## Later

- automatic password-save detection
- generated passwords
- profiles/containers
- reader mode
- vertical tabs/tab groups
- picture-in-picture controls
- optional E2E encrypted/self-hostable sync
- ARM64 Windows/Linux builds
- Android shell
- Servo adapter prototype

## Definition of faster

Knogn will publish reproducible numbers rather than rely on perceived speed. Performance targets include startup latency, idle RSS, multi-tab RSS, page responsiveness, load CPU, network throughput and battery impact. A speed optimization that weakens sandboxing, identity security or privacy policy is not acceptable.
