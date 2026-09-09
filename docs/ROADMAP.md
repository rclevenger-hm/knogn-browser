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

## Milestone 0.2 — prove the privacy claim

- [ ] startup network-leak integration harness using a local intercepting proxy
- [ ] enumerate and classify every browser-owned startup request
- [ ] test normal and private profiles independently
- [ ] build artifacts from all desktop CI jobs
- [ ] signed checksums and SBOM
- [ ] dependency/security update policy for Qt/Chromium

## Milestone 0.3 — daily-driver local services

- [ ] bookmarks + import/export
- [ ] local history + retention/clear-on-exit controls
- [ ] session restore
- [ ] find/zoom/print/PDF
- [ ] download manager
- [ ] configurable search engines
- [ ] per-site permissions and storage exceptions
- [ ] tab sleeping UI and resource view

## Milestone 0.4 — privacy engine

- [ ] EasyList/EasyPrivacy-compatible request blocking
- [ ] query-parameter stripping
- [ ] HTTPS-only mode
- [ ] per-site JavaScript policy
- [ ] anti-fingerprinting modes with breakage tests
- [ ] DNS-over-HTTPS without a mandatory provider
- [ ] proxy/SOCKS/Tor launch profiles
- [ ] local-only privacy dashboard

## Milestone 0.5 — extension breadth

- [ ] extension manager UI
- [ ] permission review before enabling extensions
- [ ] Firefox `.xpi` static compatibility inspector
- [ ] safe Firefox-to-MV3 normalization for supported cases
- [ ] clear reporting for unsupported Firefox APIs
- [ ] optional extension store browsing without a Knogn account

## Later

- password manager + OS keychain integration
- passkeys/WebAuthn UX
- profiles/containers
- reader mode
- vertical tabs/tab groups
- picture-in-picture controls
- developer tools launcher
- optional E2E encrypted/self-hostable sync
- Windows/Linux/macOS installers
- ARM64 builds
- Android shell
- Servo adapter prototype

## Definition of faster

Knogn will publish reproducible numbers rather than rely on perceived speed. Performance targets include startup latency, idle RSS, multi-tab RSS, page responsiveness, load CPU and battery impact. A speed optimization that weakens sandboxing or privacy policy is not an acceptable optimization.
