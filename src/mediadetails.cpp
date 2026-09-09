#include "mediadetails.h"

QString mediaDiagnosticsHtml() {
    return QString::fromUtf8(R"KNOGN(
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Knogn Media Compatibility</title>
<style>
:root { color-scheme: light dark; font-family: system-ui, -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif; }
body { margin: 0; background: Canvas; color: CanvasText; }
main { max-width: 980px; margin: 0 auto; padding: 32px 24px 64px; }
h1 { margin: 0 0 8px; font-size: 30px; }
p { line-height: 1.55; }
.note { padding: 14px 16px; border: 1px solid color-mix(in srgb, CanvasText 20%, transparent); border-radius: 10px; margin: 18px 0 24px; }
table { width: 100%; border-collapse: collapse; margin-top: 18px; }
th, td { text-align: left; padding: 10px 12px; border-bottom: 1px solid color-mix(in srgb, CanvasText 18%, transparent); }
th { font-weight: 650; }
.ok { font-weight: 700; }
.no { font-weight: 700; }
code { font-family: ui-monospace, SFMono-Regular, Consolas, monospace; font-size: 0.9em; }
.small { opacity: .75; font-size: 0.92rem; }
#summary { font-weight: 650; }
</style>
</head>
<body>
<main>
<h1>Knogn Media Compatibility</h1>
<p>This page probes the capabilities exposed by the exact WebEngine build currently running. It makes no external network requests.</p>
<div class="note">
<div id="summary">Running media probes…</div>
<div class="small" id="engine"></div>
</div>
<table>
<thead><tr><th>Format / feature</th><th>HTML media</th><th>Media Source Extensions</th></tr></thead>
<tbody id="results"></tbody>
</table>
<p class="small">“Probably” or “Maybe” means Chromium reports the type as playable; actual playback can still depend on profile, level, bit depth, encryption, hardware acceleration, and the media server's stream.</p>
<p class="small">Plex commonly direct-plays H.264/AAC MP4. If H.264 + AAC reports unsupported here, Plex playback failures are expected until Knogn ships an engine build with that codec capability.</p>
</main>
<script>
(() => {
  const probes = [
    ['H.264 + AAC (MP4)', 'video', 'video/mp4; codecs="avc1.42E01E, mp4a.40.2"'],
    ['H.264 video (MP4)', 'video', 'video/mp4; codecs="avc1.42E01E"'],
    ['AAC audio (MP4)', 'audio', 'audio/mp4; codecs="mp4a.40.2"'],
    ['HEVC / H.265 (MP4)', 'video', 'video/mp4; codecs="hvc1.1.6.L93.B0"'],
    ['AV1 (MP4)', 'video', 'video/mp4; codecs="av01.0.08M.08"'],
    ['VP9 + Opus (WebM)', 'video', 'video/webm; codecs="vp9, opus"'],
    ['VP8 + Vorbis (WebM)', 'video', 'video/webm; codecs="vp8, vorbis"'],
    ['Opus audio (WebM)', 'audio', 'audio/webm; codecs="opus"'],
    ['Ogg Vorbis', 'audio', 'audio/ogg; codecs="vorbis"'],
    ['FLAC', 'audio', 'audio/flac'],
    ['MP3', 'audio', 'audio/mpeg'],
    ['Dolby Digital / AC-3', 'audio', 'audio/mp4; codecs="ac-3"'],
    ['Dolby Digital Plus / E-AC-3', 'audio', 'audio/mp4; codecs="ec-3"'],
  ];

  const tbody = document.getElementById('results');
  let supported = 0;
  let h264 = false;
  for (const [label, kind, mime] of probes) {
    const el = document.createElement(kind);
    const raw = el.canPlayType(mime);
    const html = raw || 'unsupported';
    const mse = window.MediaSource && typeof MediaSource.isTypeSupported === 'function'
      ? (MediaSource.isTypeSupported(mime) ? 'supported' : 'unsupported')
      : 'unavailable';
    if (raw) supported++;
    if (label === 'H.264 + AAC (MP4)' && raw) h264 = true;

    const tr = document.createElement('tr');
    tr.innerHTML = `<td><strong>${label}</strong><br><code>${mime}</code></td>` +
      `<td class="${raw ? 'ok' : 'no'}">${html}</td>` +
      `<td class="${mse === 'supported' ? 'ok' : 'no'}">${mse}</td>`;
    tbody.appendChild(tr);
  }

  const featureRows = [
    ['Media Source Extensions', !!window.MediaSource],
    ['Encrypted Media Extensions API', !!navigator.requestMediaKeySystemAccess],
    ['MediaCapabilities API', !!navigator.mediaCapabilities],
    ['WebCodecs API', !!window.VideoDecoder],
    ['WebGL', !!document.createElement('canvas').getContext('webgl')],
  ];
  for (const [label, value] of featureRows) {
    const tr = document.createElement('tr');
    tr.innerHTML = `<td><strong>${label}</strong></td><td class="${value ? 'ok' : 'no'}">${value ? 'available' : 'unavailable'}</td><td>—</td>`;
    tbody.appendChild(tr);
  }

  const summary = document.getElementById('summary');
  summary.textContent = h264
    ? `Core H.264/AAC browser media support is exposed. ${supported}/${probes.length} probed formats are accepted.`
    : `H.264/AAC browser media support is NOT exposed. ${supported}/${probes.length} probed formats are accepted.`;
  document.getElementById('engine').textContent = navigator.userAgent;

  if (navigator.requestMediaKeySystemAccess) {
    const widevine = document.createElement('tr');
    widevine.innerHTML = '<td><strong>Widevine DRM</strong></td><td id="widevine" colspan="2">probing…</td>';
    tbody.appendChild(widevine);
    const configs = [{
      initDataTypes: ['cenc', 'webm'],
      videoCapabilities: [
        {contentType: 'video/webm; codecs="vp9"'},
        {contentType: 'video/mp4; codecs="avc1.42E01E"'}
      ]
    }];
    navigator.requestMediaKeySystemAccess('com.widevine.alpha', configs)
      .then(() => { document.getElementById('widevine').textContent = 'available'; })
      .catch(() => { document.getElementById('widevine').textContent = 'unavailable'; });
  }
})();
</script>
</body>
</html>
)KNOGN");
}
