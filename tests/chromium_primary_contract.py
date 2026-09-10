from pathlib import Path

root = Path(__file__).resolve().parents[1]
engine = root / "engine/chromium"

args = (engine / "args.gn.example").read_text(encoding="utf-8")
bootstrap = (engine / "bootstrap.py").read_text(encoding="utf-8")
overlay = (engine / "knogn_overlay.py").read_text(encoding="utf-8")
probe = (engine / "probe_runtime.py").read_text(encoding="utf-8")
probe_page = (engine / "runtime_probe.html").read_text(encoding="utf-8")
builder = (root / "tools/build_knogn.py").read_text(encoding="utf-8")
docs = (root / "docs/ENGINE_MIGRATION.md").read_text(encoding="utf-8")

checks = {
    "open Chromium branding": "is_chrome_branded = false" in args,
    "no official Google API keys": "use_official_google_api_keys = false" in args,
    "browser OAuth client omitted": 'google_default_client_id = ""' in args and 'google_default_client_secret = ""' in args,
    "Widevine support hook": "enable_widevine = true" in args,
    "safe public codec baseline": "proprietary_codecs = false" in args and 'ffmpeg_branding = "Chromium"' in args,
    "media experiment supported": "proprietary_codecs = true" in bootstrap and 'ffmpeg_branding = "Chrome"' in bootstrap,
    "overlay automatically applied": "knogn_overlay.py" in bootstrap and "subprocess.run([sys.executable" in bootstrap,
    "Knogn Chromium branding": '"PRODUCT_FULLNAME": "Knogn"' in overlay,
    "Windows icon replacement": "chromium.ico" in overlay and "assets/knogn.ico" in overlay,
    "metrics disabled at source": "FEATURE_DISABLED_BY_DEFAULT" in overlay and "MetricsReporting" in overlay,
    "Chromium is primary build path": "Chromium is the default backend" in builder,
    "Qt requires explicit fallback": 'add_parser("qt-fallback"' in builder,
    "runtime identity page rejects Qt": "QtWebEngine" in probe_page and "chromiumBrowserIdentity" in probe_page,
    "runtime identity harness enforces result": 'result.get("chromiumBrowserIdentity")' in probe,
    "runtime media acceptance": "--require-media" in probe and "mseH264Aac" in probe_page and "h264Aac" in probe_page,
    "migration gate documented": "Google" in docs and "Plex" in docs and "Qt" in docs,
}

failed = [name for name, ok in checks.items() if not ok]
if failed:
    raise SystemExit("Chromium primary-backend contract failed: " + ", ".join(failed))

print(f"Chromium primary-backend contract passed: {len(checks)} checks")
