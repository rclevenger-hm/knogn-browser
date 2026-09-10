# Knogn contributor instructions

Knogn is a privacy-first browser. Treat privacy, identity compatibility, media compatibility, and security behavior as product API, not implementation detail.

## Hard rules

1. Do not add telemetry, analytics, crash-upload, advertising, tracking, remote feature flags, or mandatory cloud services.
2. Do not add a network dependency without documenting exactly what leaves the device, when, why, and how the user disables it.
3. Never disable Chromium sandboxing to make a test or build easier.
4. Do not weaken third-party-state blocking, WebRTC restrictions, permission policy, or private-profile persistence behavior without an ADR and explicit review.
5. Do not spoof Google Chrome or another browser to bypass identity-provider security checks.
6. Website federated login must remain separate from browser-level Chrome Sync/account coupling.
7. New browser-owned network features require an integration test proving clean startup remains silent.
8. Do not claim anonymity. Knogn is a private browser, not an anonymity network.
9. Prefer explicit local state ownership over always-running remote services.
10. Run the privacy/source/identity/media/Chromium-backend contracts before proposing changes.
11. Commit messages should describe the engineering change and must not mention code-generation assistants.

## Current engine contract

**Full Chromium is Knogn's primary desktop engine for the 0.3.x line and later.**

- Chromium source lives in an external pinned checkout prepared by `engine/chromium/bootstrap.py`.
- Knogn product/privacy changes are applied through `engine/chromium/knogn_overlay.py` before GN generation.
- Use `python3 tools/build_knogn.py prepare|build|probe` for normal engine work.
- The Qt WebEngine shell is a compatibility fallback for the 0.2.x line only; build it explicitly with `python3 tools/build_knogn.py qt-fallback`.
- New product features should target Chromium first. Do not expand the Qt shell unless required to repair a supported 0.2.x regression.
- A public 0.3.x release must be packaged from the Chromium backend and must pass identity, media, privacy, performance, and platform distribution gates.

## Media and identity release gates

Do not mark the Chromium migration complete until released builds can complete representative Google/Microsoft/Apple/GitHub federated sign-in flows and can play representative H.264/AAC HTML5/MSE media plus Plex scenarios. Widevine may only be distributed or installed through an approved licensed path.
