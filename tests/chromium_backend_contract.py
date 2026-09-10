from pathlib import Path
import importlib.util
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[1]
engine = root / "engine" / "chromium"
bootstrap = engine / "bootstrap.py"
overlay = engine / "knogn_overlay.py"
pin = engine / "chromium_version.txt"
args_file = engine / "args.gn.example"
adr = root / "docs" / "adr" / "0004-full-chromium-backend.md"

for path in [bootstrap, overlay, pin, args_file, adr]:
    assert path.exists(), f"missing Chromium backend file: {path.relative_to(root)}"

version = pin.read_text(encoding="utf-8").strip()
assert version == "153.0.8010.36"

baseline = args_file.read_text(encoding="utf-8")
assert 'is_chrome_branded = false' in baseline
assert 'use_official_google_api_keys = false' in baseline
assert 'google_default_client_id = ""' in baseline
assert 'google_default_client_secret = ""' in baseline
assert 'enable_widevine = true' in baseline
assert 'proprietary_codecs = false' in baseline
assert 'ffmpeg_branding = "Chromium"' in baseline

spec = importlib.util.spec_from_file_location("knogn_chromium_bootstrap", bootstrap)
module = importlib.util.module_from_spec(spec)
assert spec.loader is not None
spec.loader.exec_module(module)

media_args = module.render_args(True)
assert 'proprietary_codecs = true' in media_args
assert 'ffmpeg_branding = "Chrome"' in media_args
assert 'is_chrome_branded = false' in media_args

with tempfile.TemporaryDirectory() as temp:
    result = subprocess.run(
        [sys.executable, str(bootstrap), "--workspace", temp],
        check=True,
        text=True,
        capture_output=True,
    )
    output = result.stdout
    assert version in output
    assert "fetch --nohooks --no-history chromium" in output
    assert "gclient sync" in output
    assert "gclient runhooks" in output
    assert "knogn_overlay.py" in output
    assert "gn gen" in output
    assert "Dry run complete" in output

print("Chromium backend bootstrap contract: pass")
