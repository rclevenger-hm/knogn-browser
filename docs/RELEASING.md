# Releasing Knogn

Knogn produces desktop install units in GitHub Actions using native runners and Qt's deployment API. The build is intentionally packaged on the operating system it targets rather than cross-compiled.

## Release asset matrix

Every GitHub Release represents one Knogn application version and contains explicitly versioned files for every supported desktop platform.

For version `0.1.0`, the release asset names are:

| Platform | Architecture | Install unit | Portable unit |
| --- | --- | --- | --- |
| Windows | x86-64 | `Knogn-0.1.0-windows-x86_64.exe` | `Knogn-0.1.0-windows-x86_64-portable.zip` |
| macOS | Apple Silicon / ARM64 | `Knogn-0.1.0-macos-arm64.dmg` | `Knogn-0.1.0-macos-arm64-portable.zip` |
| Linux | x86-64 | `Knogn-0.1.0-linux-x86_64.deb` | `Knogn-0.1.0-linux-x86_64-portable.tar.xz` |

Each platform also has a versioned SHA-256 manifest, for example `SHA256SUMS-Knogn-0.1.0-windows-x86_64.txt`.

The version comes from `project(KnognBrowser VERSION ...)` in `CMakeLists.txt`. Explicit release tags must match that version exactly. For example, application version `0.1.0` can only be tagged `v0.1.0`; a mismatched tag fails before publishing anything.

## Pipeline

`.github/workflows/package.yml` runs on:

- pull requests targeting `main` so packaging changes are tested before merge;
- pushes to `main`;
- version tags matching `v*`;
- manual `workflow_dispatch` runs.

The workflow performs these steps on every platform:

1. resolve the application version from `CMakeLists.txt` and verify an explicit release tag matches it;
2. install the pinned Qt 6.11.2 WebEngine toolchain;
3. configure and compile Knogn in Release mode;
4. let `qt_generate_deploy_app_script()` collect the Qt libraries, plugins, WebEngine process, resources and other runtime dependencies;
5. run CPack with the platform-specific generators;
6. rename the generated packages to the canonical version/platform/architecture filenames;
7. inspect the portable package and fail if Knogn, `QtWebEngineProcess`, or required WebEngine resources are missing;
8. report package sizes and calculate versioned SHA-256 checksums;
9. remove temporary CPack staging data and upload only the real distribution files.

Linux packages omit Qt translations, strip deployable binaries, and use xz compression to keep the WebEngine distribution substantially smaller without dropping required runtime content.

After a successful `main` packaging run, the release job checks whether `v<application-version>` already has a published GitHub Release:

- if the version has never been released, the workflow creates the version tag at that exact `main` commit and publishes all Windows, Linux, and macOS assets;
- if that version is already published, the release is left unchanged so published versions remain immutable;
- an explicit matching `v*` tag is still supported and follows the same complete asset-matrix validation.

Before publishing, the release job verifies that all six expected OS files and all three checksum manifests exist.

## Version release

To publish a new Knogn version, update the project version in `CMakeLists.txt` as part of the release change and merge it to `main`. For example:

```cmake
project(KnognBrowser VERSION 0.2.0 LANGUAGES CXX)
```

Once the `main` packaging matrix succeeds, GitHub Actions automatically creates `v0.2.0` and the **Knogn 0.2.0** release with the complete Windows, Linux, and macOS distribution set.

You may also create the matching version tag manually if needed:

```bash
git tag v0.2.0
git push origin v0.2.0
```

The important rule is that the source version, tag, filenames, checksums, and GitHub Release version must all agree.

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
