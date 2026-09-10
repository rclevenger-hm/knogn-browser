# Knogn Chromium backend

The Qt WebEngine implementation remains Knogn's bootstrap browser while this directory establishes the replacement engine path required for mainstream browser compatibility.

## Why this exists

Two compatibility classes now exceed what should be papered over in an embedded WebEngine shell:

1. **Federated identity.** Google explicitly rejects OAuth authorization in embedded user-agents. Current Qt WebEngine builds can therefore be rejected even though their renderer is Chromium-derived.
2. **Media.** Stock Qt WebEngine packages do not expose H.264/AAC unless WebEngine is built with the corresponding proprietary codec capability, and Widevine is a separate CDM that Qt does not ship.

A full Chromium browser backend gives Knogn control at the actual browser layer rather than only at the embedder API. It also moves Knogn onto the browser implementation where FedCM, WebAuthn, popup/redirect OAuth, media pipelines, CDM integration, site isolation, permissions and other web-platform features are developed upstream.

## Source model

Knogn should not vendor the complete Chromium tree in this repository. The intended model is an overlay/patch repository:

- `chromium_version.txt` pins the upstream browser revision used by a Knogn engine generation.
- Knogn build scripts fetch Chromium using `depot_tools`/`gclient` into an external build workspace.
- Knogn-owned patches and generated resources are applied after checkout.
- privacy/network contracts scan the final source/build configuration for unwanted first-party service calls.
- platform packaging consumes the resulting Knogn browser output.

This is similar to the maintainable patch-overlay approach used by established Chromium forks rather than keeping millions of upstream files in the Knogn repository.

## Current pin

The initial spike targets Chromium `153.0.8010.36`, the desktop stable generation current when this backend work began. This pin is a starting point, not a promise to freeze there; security update cadence must track upstream stable releases closely.

## Baseline build profile

`args.gn.example` is intentionally conservative. It builds an unbranded Chromium browser with official Google API keys disabled and proprietary codecs disabled.

For the media experiment, Chromium supports:

```gn
proprietary_codecs = true
ffmpeg_branding = "Chrome"
```

The `Chrome` value here is Chromium's FFmpeg codec configuration name. It does **not** turn Knogn into Google Chrome. However, producing and distributing binaries with patented codecs can create licensing obligations, so the public release workflow must not enable that profile until redistribution rights are resolved.

## Widevine

Widevine remains separate from H.264/AAC. Knogn must not copy or redistribute a Widevine binary without an approved distribution path. The Chromium backend should support a user-installed or properly licensed CDM and report the exact runtime capability.

## Privacy requirements for the Chromium backend

The backend is not accepted merely because it builds. Before replacing Qt WebEngine it must prove:

- no Knogn-owned telemetry or analytics;
- no mandatory Google account/browser sync integration;
- no unsolicited first-party background requests;
- component/update behavior is explicit and auditable;
- third-party tracking protections remain enabled without breaking standards-based federated identity;
- private profiles remain ephemeral;
- extension policy is inspectable;
- media/CDM components are separately accounted for and licensed;
- startup, page-load, memory and network benchmarks are no worse than the bootstrap backend unless a compatibility/security tradeoff is documented.

## Migration target

The Qt browser remains available while the Chromium backend is brought up. The backend becomes the primary Knogn desktop engine only after it passes the existing privacy/performance/media contracts plus real federated-login tests.
