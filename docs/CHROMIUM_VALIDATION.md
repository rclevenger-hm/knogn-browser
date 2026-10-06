# Chromium build and runtime validation

The migration has two distinct kinds of evidence: **tooling/harness validation** and **a built Knogn browser's compatibility results**. Passing the former does not prove the latter. Google/Microsoft/Apple/GitHub login, Plex, Widevine, startup network behavior and signed platform packages remain release gates.

## Build host and repeatable preparation

Check capacity before downloading Chromium:

```bash
python3 tools/build_knogn.py preflight --workspace ~/knogn-chromium
python3 tools/build_knogn.py build --workspace ~/knogn-chromium --jobs 8
```

Preparation and builds automatically run the same preflight. The current floor is 100 GiB free disk and 16 GiB physical RAM; 32 GiB or more is recommended. These are rejection thresholds, not a guarantee that every full checkout/link will fit. The dedicated Linux runner also needs FFmpeg with VP9/Opus encoders (and H.264/AAC for the local media experiment), normal Chromium Linux build dependencies, and an unprivileged account capable of running the sandbox. No test disables the sandbox.

The overlay now records original bytes and before/after checksums. On the next preparation it restores only verified overlay changes, refuses edited managed files, and rejects remaining tracked edits before switching Chromium revisions. Upstream pattern drift is detected before branding files are changed. Interrupted writes or legacy text-only `.knogn-overlay` markers fail safely: preserve the workspace and start with a clean checkout rather than resetting unknown local changes.

A build fetches the exact version tag, syncs dependencies without running hooks early, then installs dependencies, runs hooks, applies the overlay and generates GN files. The persistent engine workflow serializes access to this workspace. For local work, run only one prepare/build process against a workspace at a time.

The two profiles use separate output directories:

| Profile | Output | Distribution |
| --- | --- | --- |
| Open-codec baseline | `chromium/src/out/Knogn` | Not release-approved until all gates pass |
| Local H.264/AAC experiment | `chromium/src/out/KnognMedia` | No public binary redistribution |

Each completed preparation writes `knogn-build.json`; only a completed browser build records `status: built` and an executable checksum. It also records the Chromium/depot_tools/Knogn commits, local Knogn modification status, GN and overlay checksums, platform and codec profile. Runtime probing checks that the binary and GN arguments match this record. Preparation invalidates the selected output's previous evidence before changing Chromium. This is traceable build provenance, not a claim of byte-for-byte reproducibility: depot_tools, the host toolchain and build dependencies still need tighter pinning for that claim.

## Real media playback

FFmpeg generates two-second synthetic moving video with a sine-wave audio track locally. No media is downloaded, no account is needed, and fixtures are never packaged with browser releases.

Open-codec baseline:

```bash
python3 tools/build_knogn.py media-fixtures --out test-results/media
python3 tools/build_knogn.py probe --media-dir test-results/media --report test-results/runtime.json
```

H.264/AAC experiment:

```bash
python3 tools/build_knogn.py build --media-experiment --jobs 8
python3 tools/build_knogn.py media-fixtures --out test-results/media-experiment --include-proprietary
python3 tools/build_knogn.py probe --out ~/knogn-chromium/chromium/src/out/KnognMedia --media-dir test-results/media-experiment --require-media --report test-results/runtime-media.json
```

Use the matching `--out` path when your build workspace differs from the default. `--require-media` now requires fixtures and successful playback, not just `canPlayType()` answers.

The probe serves an allowlisted set of files on an ephemeral **127.0.0.1** port. Results return as JSON to a unique local route with an origin check; nested results and HTML characters do not rely on DOM scraping. The page restricts its requests to the same origin and local media blobs. The browser uses a temporary profile, which is removed after the process exits. No report is sent to Knogn or any third-party service. `--report` writes an explicitly requested local JSON file; omitting it prints to the terminal. CI retains these JSON reports in the repository's Actions artifacts for 14 days (harness) or 30 days (engine); omit the upload step to keep reports only on the build host.

The baseline plays VP9/Opus WebM. The proprietary experiment additionally plays a combined H.264/AAC MP4 directly and appends fragmented MP4 to an actual `MediaSource`/`SourceBuffer`, ends the stream and plays to completion. Success requires elapsed playback time and non-dropped video frames. Playback is muted to respect autoplay policy; this is not an audible-output, hardware acceleration, adaptive streaming or DRM certification.

A probe with no fixtures is labeled `capabilities-only`. It can inspect credential, WebAuthn, FedCM, storage-access and codec surfaces, but API presence is **not successful authentication**. Likewise, EME presence is **not an installed Widevine CDM**. Every report retains `releaseAcceptance: false` because these probes cover only part of the release matrix. The probe disables background networking to isolate media tests, so it cannot establish startup silence; a separate unsuppressed startup network audit remains required.

## Regression checks and CI evidence

```bash
bash tools/check.sh
python3 -m unittest discover -s tests -p 'test_*.py' -v
```

The behavioral tests run on Linux, Windows and macOS. Hosted Linux CI generates local media and tests direct/MSE playback with the runner's installed Chrome, without changing its identity or sandbox. Those reports are labeled `external-browser-harness`, and never count as a Knogn build. To reproduce this harness-only check with an installed Chromium-family browser:

```bash
python3 engine/chromium/probe_runtime.py --binary /path/to/browser --media-dir test-results/media-experiment --require-media --report test-results/harness.json
```

The self-hosted `chromium-engine` workflow runs the same tests against the actual compiled Knogn binary and retains only build/runtime JSON. Browser binaries, CDMs and test media are not uploaded. Failed probes return a nonzero exit status and retain the failure report; timeouts, missing output, mismatched checksums, missing capabilities and failed playback cannot become successful acceptance evidence.
