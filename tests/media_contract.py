#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
page = (ROOT / "src/browserpage.cpp").read_text(encoding="utf-8")
profile = (ROOT / "src/privacyprofile.cpp").read_text(encoding="utf-8")
diagnostics = (ROOT / "src/mediadetails.cpp").read_text(encoding="utf-8")

checks = {
    "fullscreen enabled": "FullScreenSupportEnabled, true" in profile and "fullScreenRequested" in page,
    "desktop media gesture policy": "PlaybackRequiresUserGesture, false" in profile,
    "GPU media path": "WebGLEnabled, true" in profile and "Accelerated2dCanvasEnabled, true" in profile,
    "media diagnostics route": 'QStringLiteral("media")' in page and "mediaDiagnosticsHtml" in page,
    "H264 probe": "avc1.42E01E" in diagnostics,
    "AAC probe": "mp4a.40.2" in diagnostics,
    "VP9 probe": "vp9" in diagnostics,
    "AV1 probe": "av01.0.08M.08" in diagnostics,
    "MSE probe": "MediaSource.isTypeSupported" in diagnostics,
    "Widevine probe": "com.widevine.alpha" in diagnostics,
    "diagnostics are local": "fetch(" not in diagnostics and "XMLHttpRequest" not in diagnostics,
}

failed = [name for name, ok in checks.items() if not ok]
if failed:
    raise SystemExit("Media contract failures: " + ", ".join(failed))
print(f"media contract: {len(checks)} checks passed")
