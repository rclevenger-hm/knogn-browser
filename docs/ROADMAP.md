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

## Milestone 0.2 — daily-driver foundations

- [x] persistent Settings UI
- [x] configurable search engines, home page and downloads
- [x] bookmarks/favorites, toolbar and manager
- [x] bookmark import from Chromium-family stores and standard bookmark HTML
- [x] OS-keychain-backed saved login storage
- [x] password CSV import, manual fill and optional autofill
- [x] system VPN routing plus HTTP/SOCKS browser-specific tunnel settings
- [x] Windows executable/installer icon and desktop shortcut integration
- [ ] local history + retention/clear-on-exit controls
- [ ] session restore / recently closed tabs
- [ ] download history/manager
- [ ] richer bookmark folders/editing

## Milestone 0.2.1 — compatibility blockers

- [x] keep third-party state blocked generally while allowing narrow known identity-provider state
- [x] modern Windows application manifest for correct browser/OS identity reporting
- [x] local `knogn://identity` diagnostics for Credentials API, FedCM surface, WebAuthn and Storage Access
- [ ] successful representative Sign in with Google flow
- [ ] successful Microsoft / Apple / GitHub OAuth flows
- [ ] prove FedCM behavior on released builds
- [ ] H.264/AAC direct playback in released packages
- [ ] H.264/AAC Media Source Extensions playback
- [ ] Widevine detection/integration through an approved distribution path
- [ ] Plex direct-play and transcode compatibility tests

## Milestone 0.3 — full Chromium browser backend

Qt WebEngine remains the bootstrap backend while Knogn brings up a full Chromium browser target. The Chromium path is now the highest-priority engine work because both federated identity and mainstream media expose hard limits of an embedded WebEngine shell.

- [x] adopt full-browser backend decision (ADR 0004)
- [x] pin initial Chromium backend spike to an upstream stable version
- [x] add baseline GN configuration and codec experiment notes
- [ ] external Chromium checkout/bootstrap tooling
- [ ] Knogn patch-overlay framework
- [ ] remove/disable unwanted browser-owned Google services without breaking ordinary Google websites
- [ ] Knogn branding/product resources in Chromium UI
- [ ] privacy network-leak contract against the Chromium target
- [ ] normal/private profile parity
- [ ] extension compatibility parity
- [ ] federated identity test suite
- [ ] media capability/playback test suite
- [ ] Windows/Linux/macOS packages from the Chromium backend
- [ ] security-update/rebase automation tracking upstream stable

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
- [ ] signed release artifacts, SBOM and dependency policy

## Milestone 0.5 — extension breadth

- [ ] extension manager UI
- [ ] permission review before enabling extensions
- [ ] Firefox `.xpi` static compatibility inspector
- [ ] safe Firefox-to-MV3 normalization for supported cases
- [ ] clear reporting for unsupported Firefox APIs
- [ ] optional extension store browsing without a Knogn account

## Later

- automatic password-save detection
- passkeys/WebAuthn UX
- generated passwords
- profiles/containers
- reader mode
- vertical tabs/tab groups
- picture-in-picture controls
- developer tools launcher
- optional E2E encrypted/self-hostable sync
- ARM64 Windows/Linux builds
- Android shell
- Servo adapter prototype

## Definition of faster

Knogn will publish reproducible numbers rather than rely on perceived speed. Performance targets include startup latency, idle RSS, multi-tab RSS, page responsiveness, load CPU, network throughput and battery impact. A speed optimization that weakens sandboxing, identity security or privacy policy is not an acceptable optimization.
