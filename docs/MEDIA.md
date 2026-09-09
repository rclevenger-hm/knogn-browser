# Media compatibility

Knogn's media target is straightforward: ordinary web audio/video that plays in mainstream desktop browsers should also play in Knogn whenever the underlying engine and operating system expose the required codec and DRM capability.

## Runtime diagnostics

Open `knogn://media` in the address bar. The page runs entirely locally and reports the media capabilities exposed by the exact Knogn build currently running, including:

- H.264 + AAC in MP4;
- H.264, HEVC/H.265 and AV1 video probes;
- AAC, MP3, FLAC, Opus, Vorbis, AC-3 and E-AC-3 audio probes;
- WebM VP8/VP9 support;
- Media Source Extensions (MSE);
- Encrypted Media Extensions (EME);
- Widevine availability;
- MediaCapabilities, WebCodecs and WebGL API availability.

A codec reporting `probably` or `maybe` is a browser capability declaration, not a guarantee that every profile, level, bit depth or encrypted stream will decode.

## Browser feature requirements

Knogn explicitly enables and tests the browser-side features that media sites normally expect:

- HTML5 fullscreen support and fullscreen request handling;
- desktop-style media playback gesture behavior;
- WebGL and accelerated 2D canvas paths;
- Media Source Extensions provided by Chromium/Qt WebEngine;
- normal site permission handling for capture devices when requested.

These features are covered by `tests/media_contract.py`.

## Codec boundary

Qt WebEngine only exposes MP4/H.264-class proprietary formats when Qt WebEngine itself was built with the corresponding proprietary codec support. Qt documents the `-webengine-proprietary-codecs` build option and warns that distributing proprietary codec libraries can require separate licenses.

Knogn therefore does not make a false codec-parity claim based only on the Chromium user agent. The release must either:

1. use a codec-enabled WebEngine distribution for which the required distribution rights are in place; or
2. use an operating-system/native media engine path that provides the codec without Knogn bundling the patented implementation; or
3. clearly report the codec as unavailable and allow the media service to transcode to a supported format.

Linux offers an additional Qt WebEngine build option for using system FFmpeg (`webengine-system-ffmpeg`), which may be useful for distro-specific packages. It is not a cross-platform solution by itself.

## DRM

Widevine is separate from ordinary codec support. Qt WebEngine supports Widevine CDM discovery, but Widevine is not shipped with Qt WebEngine. A site that requires Widevine can still fail even when H.264/AAC is available.

Knogn will not silently copy a Widevine binary out of another installed browser. `knogn://media` reports whether the current environment exposes Widevine to the running engine.

## Plex

Plex commonly direct-plays H.264/AAC MP4 and uses MSE for browser playback. A Plex failure can therefore indicate one of several layers:

- H.264/AAC is unavailable in the WebEngine build;
- the Plex server selected a codec/profile the browser cannot decode;
- MSE or playback policy is unavailable;
- an encrypted/DRM path is involved;
- a site/network/authentication problem occurred independently of media decoding.

For a Plex playback failure, capture the `knogn://media` results first. If H.264 + AAC is unsupported, that is the primary compatibility gap to resolve before debugging Plex-specific behavior.

## Release policy

A Knogn release should not be described as having Chrome-level media compatibility until the release pipeline or runtime diagnostics demonstrate the expected codec matrix on each supported operating system.
