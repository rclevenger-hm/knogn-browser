# ADR 0003: Media codec parity must be explicit and cross-platform

## Status

Accepted for the 0.1.x architecture.

## Context

A Plex playback failure in Knogn 0.1.1 exposed a browser-wide requirement: media compatibility cannot be inferred from the Chromium-derived user agent or from the presence of HTML5 video APIs alone.

Qt WebEngine separates browser media features from codec availability. HTML5 fullscreen, MSE, EME, WebGL and related APIs can be enabled by the application, but formats such as H.264/AAC MP4 depend on how Qt WebEngine itself was built. Widevine is a separate CDM dependency and is not shipped with Qt WebEngine.

## Decision

Knogn will treat media support as a first-class engine capability with four rules:

1. Browser-side media APIs that mainstream desktop browsers expose will be enabled unless there is a documented privacy/security reason not to expose them.
2. Every running Knogn build must be able to report its actual codec/API capability matrix locally through `knogn://media`.
3. Knogn will not claim support for a codec merely by spoofing user-agent or JavaScript capability results.
4. Cross-platform codec parity must come from either a properly licensed codec-enabled WebEngine build or an operating-system/native fallback backend. Platform-specific system codec paths may be used where appropriate, but they cannot be described as the universal solution.

## Consequences

- Fullscreen and normal desktop media behavior are part of the tested browser contract.
- Missing H.264/AAC support is visible and diagnosable rather than surfacing only as opaque site errors.
- A future codec-enabled WebEngine distribution can be introduced without changing the browser UI contract.
- Linux may use a system FFmpeg-based WebEngine build where technically and legally appropriate.
- Windows/macOS need a codec-enabled WebEngine or native-engine/media fallback for mainstream codec parity.
- Widevine remains a separate capability and must be detected independently.

## Non-goals

This ADR does not authorize redistribution of proprietary codec implementations or DRM components. Licensing/distribution rights must be resolved before such binaries are attached to public Knogn releases.
