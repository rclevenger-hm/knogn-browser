# Releasing Knogn

Knogn produces desktop install units in GitHub Actions using native runners and Qt's deployment API. The build is intentionally packaged on the operating system it targets rather than cross-compiled.

## Artifact matrix

| Platform | Architecture | Install unit | Portable unit |
| --- | --- | --- | --- |
| Windows | x86-64 | NSIS `.exe` | `.zip` |
| macOS | Apple Silicon | `.dmg` | `.zip` |
| Linux | x86-64 | Debian `.deb` | `.tar.gz` |

Each platform artifact also contains `SHA256SUMS.txt` for integrity verification.

## Pipeline

`.github/workflows/package.yml` runs on:

- pull requests targeting `main` so packaging changes are tested before merge;
- pushes to `main` so the current branch always has downloadable build artifacts;
- version tags matching `v*`;
- manual `workflow_dispatch` runs.

The workflow performs these steps on every platform:

1. install the pinned Qt 6.11.2 WebEngine toolchain;
2. configure and compile Knogn in Release mode;
3. let `qt_generate_deploy_app_script()` collect the Qt libraries, plugins, WebEngine process, resources and other runtime dependencies;
4. run CPack with the platform-specific generators;
5. calculate SHA-256 checksums;
6. upload the platform artifact set.

For version tags, the three artifact sets are downloaded into a release job and attached to the matching GitHub Release.

## Version release

Prepare and merge the desired source state, update the project version in `CMakeLists.txt`, then create a version tag such as:

```bash
git tag v0.1.0
git push origin v0.1.0
```

The tag starts the packaging matrix and, if all packaging jobs succeed, the release job publishes the generated files.

## Signing status

Development artifacts are currently unsigned. That means:

- Windows may show SmartScreen reputation warnings;
- macOS may block or warn on a downloaded application until the user explicitly allows it;
- packages should not yet be described as production-trusted binaries.

Before a stable release, add platform signing without exposing private keys to pull-request jobs:

- Windows Authenticode signing for the executable and NSIS installer;
- Apple Developer ID signing, hardened runtime and notarization for the macOS bundle/DMG;
- signed release checksums and, later, package-repository signing for Linux.

Signing must happen only in trusted release jobs and must not introduce telemetry or network calls into the Knogn application itself.
