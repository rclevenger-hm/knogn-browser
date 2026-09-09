# Knogn contributor/agent instructions

Knogn is a privacy-first browser. Treat privacy and security behavior as product API, not implementation detail.

## Hard rules

1. Do not add telemetry, analytics, crash-upload, advertising, tracking, remote feature flags, or mandatory cloud services.
2. Do not add a network dependency without documenting exactly what leaves the device, when, why, and how the user disables it.
3. Never disable Chromium/Qt sandboxing to make a test or build easier.
4. Do not weaken third-party-state blocking, WebRTC restrictions, permission policy, or private-profile persistence behavior without an ADR and explicit review.
5. Keep browser chrome native; do not introduce Electron or a browser-UI web runtime.
6. New browser-owned network features require an integration test proving clean startup remains silent.
7. Do not claim anonymity. Knogn is a private browser, not an anonymity network.
8. Prefer small local services and explicit state ownership over always-running daemons.
9. Run the privacy/source contracts before proposing changes.
10. Commit messages should describe the engineering change and must not mention code-generation assistants.

## Current engine contract

Qt 6.11+ / Qt WebEngine is the compatibility engine for v1. Keep engine-specific behavior behind narrow classes so another renderer can be evaluated later.
