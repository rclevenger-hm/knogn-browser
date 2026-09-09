from pathlib import Path

root = Path(__file__).resolve().parents[1]
branding = (root / "src" / "branding.cpp").read_text()
main = (root / "src" / "main.cpp").read_text()
site = (root / "site" / "index.html").read_text()
workflow = (root / ".github" / "workflows" / "pages.yml").read_text()

checks = {
    "runtime application icon": "Branding::applicationIcon" in main,
    "runtime green stylesheet": "Branding::applicationStyleSheet" in main,
    "approved forest palette": "#071711" in branding,
    "local branded new tab prepared": "A more mindful web" in branding and "Explore. Privately." in branding,
    "pages landing page": "A more mindful web" in site and "No analytics. No trackers" in site,
    "pages deployment workflow": "actions/deploy-pages@v4" in workflow,
}

failed = [name for name, ok in checks.items() if not ok]
if failed:
    raise SystemExit("Branding contract failed: " + ", ".join(failed))

print(f"Branding contract passed: {len(checks)} checks")
