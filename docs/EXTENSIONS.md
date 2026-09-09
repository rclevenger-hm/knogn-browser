# Extension Compatibility

## Goal

Knogn should be feature-rich without recreating every browser feature in core. Extensions are therefore a first-class compatibility target, with privacy boundaries made explicit.

## Chromium Manifest V3

Qt WebEngine 6.11 supports loading/installing zipped or unpacked Chrome Manifest V3 extensions. Knogn exposes this in normal profiles and enables an extension after a successful install.

Qt WebEngine does not load user extensions into off-the-record profiles, so Knogn private windows are extension-free in the first engine. This is considered a useful privacy property rather than something to bypass.

## Firefox WebExtensions

Firefox's WebExtensions model overlaps heavily with Chromium's, but compatibility is not binary. A future Knogn importer will:

1. unpack `.xpi`/ZIP packages locally;
2. inspect `manifest.json` without executing extension code;
3. classify manifest version and API usage;
4. normalize straightforward `browser.*` / `chrome.*` compatibility cases;
5. identify Firefox-only APIs such as contextual identities/containers;
6. identify incompatible background/service-worker semantics;
7. produce a human-readable compatibility report;
8. install only when the resulting package can be represented safely as MV3.

Knogn will not silently rewrite broad permissions or pretend an incompatible extension works.

## Store integration

Direct Chrome Web Store or Firefox Add-ons browsing/install flows are later work. They must not require a Knogn account or introduce browser-owned tracking.
