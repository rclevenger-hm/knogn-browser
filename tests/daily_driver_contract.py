from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
cmake = (root / "CMakeLists.txt").read_text()
window = (root / "src" / "browserwindow.cpp").read_text()
settings = (root / "src" / "settingsdialog.cpp").read_text()
bookmarks = (root / "src" / "bookmarks.cpp").read_text()
credentials = (root / "src" / "credentialstore.cpp").read_text()
flags = (root / "src" / "browserflags.cpp").read_text()
privacy = (root / "src" / "privacyprofile.cpp").read_text()

checks = {
    "0.2 daily-driver version line": bool(re.search(r"VERSION 0\.2\.\d+", cmake)),
    "windows executable icon resource": "assets/knogn.ico" in cmake and "knogn.rc" in cmake,
    "windows installer icon": "CPACK_NSIS_MUI_ICON" in cmake,
    "windows installed app icon": "CPACK_NSIS_INSTALLED_ICON_NAME" in cmake,
    "windows desktop shortcut option": "CPACK_CREATE_DESKTOP_LINKS" in cmake,
    "persistent settings dialog": "Knogn Settings" in settings and "QSettings" in settings,
    "vpn and network settings": "VPN & Network" in settings and "SOCKS5" in settings,
    "bookmark persistence": "bookmarks.json" in bookmarks,
    "bookmark browser import": "importChromiumJson" in bookmarks and "importHtml" in bookmarks,
    "bookmark user interface": "Manage bookmarks" in window and "Import bookmarks" in window,
    "secure platform credential library": "qtkeychain" in cmake.lower(),
    "no plaintext password fallback": "setInsecureFallback(false)" in credentials,
    "password csv import": "importCsv" in credentials and "Import passwords from CSV" in window,
    "saved login fill": "runJavaScript" in window and "input[type=\"password\"]" in window,
    "configurable browser routing": "--proxy-server=" in flags and "--no-proxy-server" in flags,
    "privacy settings cached into profile": "AppSettings::blockThirdPartyState" in privacy,
}

failed = [name for name, ok in checks.items() if not ok]
if failed:
    raise SystemExit("Daily-driver contract failed: " + ", ".join(failed))

print(f"Daily-driver contract passed: {len(checks)} checks")
