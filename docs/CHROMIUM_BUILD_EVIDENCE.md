# Chromium build evidence gate

The full Chromium backend is now the accepted compatibility direction. Because a Chromium build is too large for an ordinary lightweight CI runner, progress should be measured with reproducible build evidence rather than by claiming parity from bootstrap scripts alone.

## Evidence required for a successful build

Record:

- the pinned Chromium version;
- host operating system, CPU architecture, logical CPU count, RAM, and free disk before checkout;
- the exact Knogn bootstrap command and GN arguments used;
- checkout/sync success;
- `gn gen` success;
- browser target build success;
- resulting binary path and size;
- a launch smoke result;
- the commit SHA of the Knogn overlay used.

Do not commit machine-specific absolute paths, credentials, or generated Chromium source trees.

## Minimum smoke checks

A build proof should demonstrate that the browser launches with the Knogn overlay, opens a local test page, reports the expected product branding, and exits cleanly. Media and federated-login parity remain separate acceptance gates; a successful compilation does not close those issues by itself.

## Failure classification

Classify failures before changing source:

- insufficient disk/RAM;
- depot_tools or checkout failure;
- GN configuration failure;
- compile/link failure;
- overlay/patch conflict;
- runtime launch failure.

Keep the first failing command and relevant compiler output with the build record. This makes rebases against new Chromium security versions measurable instead of anecdotal.

## Release implication

No public build should be described as the Chromium backend until this build gate is satisfied on a reproducible host and the resulting binary passes the project's privacy, identity, media, and packaging contracts.
