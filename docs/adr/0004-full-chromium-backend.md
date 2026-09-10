# ADR 0004: Move mainstream compatibility to a full Chromium browser backend

- **Status:** Accepted for implementation spike
- **Date:** 2026-09-10

## Context

Knogn began as a native Qt shell around Qt WebEngine because that let the project establish privacy policy, product UI, packaging, extension handling and performance controls quickly while retaining strong web compatibility.

Two requirements now expose the limit of that boundary:

1. **Federated identity:** providers such as Google can reject embedded user-agents independently of the Chromium version underneath them. This prevents Knogn from promising normal linked-account login parity while it remains only a Qt WebEngine embedder.
2. **Mainstream media:** H.264/AAC capability is determined when Qt WebEngine is built, and Widevine is a separate CDM. The stock Qt packages used by Knogn cannot be treated as equivalent to a mainstream browser distribution.

Continuing to add application-level workarounds would produce fragile special cases without solving the root problem.

## Decision

Knogn will develop a **full Chromium browser backend** using an overlay/patch model against a pinned upstream stable Chromium release.

Qt WebEngine remains the bootstrap backend until the Chromium path reaches functional parity and passes Knogn's privacy/security/performance acceptance gates.

The Chromium backend will:

- build the browser target rather than an embedded WebView/CEF-style surface;
- retain Chromium's standards-based browser mechanisms needed for FedCM, WebAuthn, OAuth redirects/popups and modern site compatibility;
- remove or disable browser-owned Google services that Knogn does not need;
- retain Knogn's no-telemetry design and private-profile guarantees;
- expose media capabilities honestly and test actual H.264/AAC/MSE/CDM behavior;
- keep patented codec and Widevine redistribution decisions separate from the open-source browser code;
- preserve a patch/configuration layer small enough to rebase rapidly onto Chromium security updates.

## Media configuration

Chromium exposes `proprietary_codecs` and `ffmpeg_branding` build arguments. A codec experiment may use:

```gn
proprietary_codecs = true
ffmpeg_branding = "Chrome"
```

This configuration may not be enabled in public Knogn release builds until the required distribution/licensing path is established.

Widevine is handled separately and must not be copied or redistributed without an approved path.

## Identity compatibility

The full browser backend is expected to remove the embedded-user-agent classification that blocks some OAuth providers, but this is not assumed. Google, Microsoft, Apple and GitHub login flows must be tested against released Knogn builds.

The interim Qt backend may narrowly allow identity-provider third-party state, but it must not spoof another browser solely to evade provider security policy.

## Consequences

- Chromium builds are substantially heavier than Qt application builds and will require dedicated or self-hosted build capacity.
- Knogn gains responsibility for maintaining a fast-moving browser fork and security-update cadence.
- The engine boundary becomes capable of the media, identity and browser-security work required for daily-driver parity.
- Existing Qt code remains useful as a product prototype and fallback during the transition, but is no longer the long-term compatibility ceiling.
