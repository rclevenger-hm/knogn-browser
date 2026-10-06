#!/usr/bin/env python3
"""Probe a browser on loopback; record capability and actual playback evidence."""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import json
import math
from pathlib import Path
import platform
import secrets
import subprocess
import tempfile
import threading
import time

HERE = Path(__file__).resolve().parent
PROBE_PAGE = HERE / "runtime_probe.html"
MEDIA_FILES = {"sample.webm", "sample.mp4", "sample-fragmented.mp4"}


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def browser_binary(out: Path) -> Path:
    system = platform.system()
    if system == "Windows":
        candidates = [out / "chrome.exe", out / "Knogn.exe"]
    elif system == "Darwin":
        candidates = [out / "Chromium.app/Contents/MacOS/Chromium", out / "Knogn.app/Contents/MacOS/Knogn"]
    else:
        candidates = [out / "chrome", out / "Knogn", out / "chromium"]
    for candidate in candidates:
        if candidate.is_file():
            return candidate.resolve()
    raise ValueError(f"No Chromium browser executable found under {out}")


def load_build(out: Path, binary: Path) -> dict:
    out, binary = out.resolve(), binary.resolve()
    manifest = json.loads((out / "knogn-build.json").read_text(encoding="utf-8"))
    if manifest.get("schemaVersion") != 1 or manifest.get("status") != "built":
        raise ValueError("A completed build manifest is required; rebuild this output directory")
    if not manifest.get("overlaySha256"):
        raise ValueError("Build was not prepared with the Knogn overlay")
    if (out / manifest["browser"]["path"]).resolve() != binary:
        raise ValueError("Build manifest names a different executable")
    if manifest["browser"]["sha256"] != sha256_file(binary):
        raise ValueError("Browser executable does not match its build manifest")
    if manifest["gnArgsSha256"] != sha256_file(out / "args.gn"):
        raise ValueError("GN arguments changed after the recorded build")
    return manifest


def load_fixtures(directory: Path | None, require_media: bool) -> dict:
    if directory is None:
        if require_media:
            raise ValueError("--require-media needs --media-dir with generated H.264/AAC playback fixtures")
        return {}
    manifest = json.loads((directory / "fixtures.json").read_text(encoding="utf-8"))
    if manifest.get("schemaVersion") != 1:
        raise ValueError("Unsupported fixture manifest")
    files = manifest.get("files", {})
    required = MEDIA_FILES if require_media else {"sample.webm"}
    if not required <= files.keys() or not files.keys() <= MEDIA_FILES:
        raise ValueError("Fixture manifest has missing or unexpected media files")
    for name, expected in files.items():
        if sha256_file(directory / name) != expected:
            raise ValueError(f"Media fixture checksum mismatch: {name}")
    return manifest


def validate_result(result: dict, require_media: bool, playback: bool) -> list[str]:
    errors = []
    if result.get("schemaVersion") != 1 or result.get("complete") is not True:
        errors.append("Probe did not complete")
    if result.get("chromiumBrowserIdentity") is not True:
        errors.append("Browser identity still looks embedded/non-Chromium")
    if result.get("credentialsApi") is not True or result.get("webauthn") is not True:
        errors.append("Mainstream credential/WebAuthn browser surfaces are missing")
    if result.get("secureContext") is not True:
        errors.append("Probe was not run in a secure context")
    if require_media:
        if result.get("h264Aac") not in {"maybe", "probably"} or result.get("mseH264Aac") is not True:
            errors.append("H.264/AAC Media Source capability is not present")
    scenarios = ["webmDirect"] if playback else []
    if require_media:
        scenarios += ["h264Direct", "h264Mse"]
    observations = result.get("playback", {})
    if not isinstance(observations, dict):
        return errors + ["Malformed playback observations"]
    # A supplied optional scenario cannot fail silently behind a passing baseline.
    scenarios = sorted(set(scenarios) | set(observations))
    for scenario in scenarios:
        observation = observations.get(scenario, {})
        if not isinstance(observation, dict):
            errors.append(f"Malformed playback observation: {scenario}")
            continue
        if (observation.get("status") != "passed" or observation.get("ended") is not True
                or type(observation.get("currentTime")) not in (int, float)
                or not math.isfinite(observation["currentTime"])
                or observation.get("currentTime", 0) < 1
                or type(observation.get("decodedFrames")) not in (int, float)
                or not math.isfinite(observation["decodedFrames"])
                or observation.get("decodedFrames", 0) < 1):
            errors.append(f"Actual playback failed or missing: {scenario}")
    return errors


class ProbeServer(ThreadingHTTPServer):
    daemon_threads = True

    def __init__(self, media_dir: Path | None, fixture_manifest: dict):
        super().__init__(("127.0.0.1", 0), ProbeHandler)
        self.token = secrets.token_hex(24)
        self.result = None
        self.finished = threading.Event()
        self.media_dir = media_dir
        self.files = fixture_manifest.get("files", {})
        self.origin = f"http://127.0.0.1:{self.server_port}"


class ProbeHandler(BaseHTTPRequestHandler):
    def log_message(self, *_args):
        pass

    def send_content(self, content: bytes, mime: str):
        self.send_response(200)
        self.send_header("Content-Type", mime)
        self.send_header("Content-Length", str(len(content)))
        self.send_header("Cache-Control", "no-store")
        self.send_header("Content-Security-Policy", "default-src 'none'; script-src 'unsafe-inline'; connect-src 'self'; media-src 'self' blob:")
        self.end_headers()
        self.wfile.write(content)

    def do_GET(self):
        if self.headers.get("Host") != self.server.origin.removeprefix("http://"):
            self.send_error(403)
            return
        if self.path == f"/{self.server.token}/":
            config = {"resultUrl": f"/{self.server.token}/result", "files": list(self.server.files)}
            page = PROBE_PAGE.read_text(encoding="utf-8").replace("__KNOGN_CONFIG__", json.dumps(config))
            self.send_content(page.encode("utf-8"), "text/html; charset=utf-8")
        elif self.path.startswith(f"/{self.server.token}/media/"):
            name = self.path.rsplit("/", 1)[-1]
            if name not in self.server.files or name not in MEDIA_FILES:
                self.send_error(404)
                return
            self.send_content((self.server.media_dir / name).read_bytes(), "video/webm" if name.endswith("webm") else "video/mp4")
        else:
            self.send_error(404)

    def do_POST(self):
        if (self.path != f"/{self.server.token}/result"
                or self.headers.get("Origin") != self.server.origin
                or self.server.finished.is_set()):
            self.send_error(403)
            return
        try:
            length = int(self.headers.get("Content-Length", "0"))
            if not 0 < length <= 64 * 1024:
                raise ValueError("invalid result size")
            result = json.loads(self.rfile.read(length))
            if not isinstance(result, dict):
                raise ValueError("expected an object")
        except (ValueError, UnicodeDecodeError):
            self.send_error(400)
            return
        self.server.result = result
        self.send_content(b"ok", "text/plain")
        self.server.finished.set()


def run_probe(binary: Path, media_dir: Path | None, fixtures: dict, timeout: float) -> dict:
    with tempfile.TemporaryDirectory(prefix="knogn-probe-") as temp, ProbeServer(media_dir, fixtures) as server:
        thread = threading.Thread(target=server.serve_forever, daemon=True)
        thread.start()
        command = [str(binary), "--headless=new", "--no-first-run", "--no-default-browser-check",
                   f"--user-data-dir={Path(temp) / 'profile'}", "--disable-background-networking",
                   f"{server.origin}/{server.token}/"]
        process = None
        try:
            with (Path(temp) / "browser.log").open("w+", encoding="utf-8") as log:
                process = subprocess.Popen(command, stdout=log, stderr=log)
                deadline = time.monotonic() + timeout
                while not server.finished.wait(0.1):
                    if process.poll() is not None:
                        log.seek(0)
                        raise ValueError(f"Browser exited before completing the probe ({process.returncode}): {log.read()[-4000:]}")
                    if time.monotonic() >= deadline:
                        raise ValueError(f"Browser probe timed out after {timeout:g} seconds")
                return server.result
        finally:
            if process is not None and process.poll() is None:
                process.terminate()
                try:
                    process.wait(timeout=10)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait(timeout=10)
            server.shutdown()
            thread.join(timeout=5)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    source = parser.add_mutually_exclusive_group(required=True)
    source.add_argument("--out", type=Path, help="completed Knogn output with knogn-build.json")
    source.add_argument("--binary", type=Path, help="test the harness with an external browser; never Knogn release evidence")
    parser.add_argument("--media-dir", type=Path, help="directory produced by generate_media_fixtures.py")
    parser.add_argument("--require-media", action="store_true", help="require actual H.264/AAC direct and MSE playback")
    parser.add_argument("--report", type=Path, help="write a JSON report, including failures")
    parser.add_argument("--timeout", type=float, default=90)
    args = parser.parse_args()
    if not 1 <= args.timeout <= 600:
        parser.error("--timeout must be between 1 and 600 seconds")
    report = {"schemaVersion": 1, "createdAt": datetime.now(timezone.utc).isoformat(),
              "status": "failed", "scope": "external-browser-harness" if args.binary else "knogn-runtime",
              "platform": platform.system(), "architecture": platform.machine(),
              "validationScope": "playback" if args.media_dir else "capabilities-only",
              "releaseAcceptance": False, "errors": []}
    try:
        binary = args.binary.resolve() if args.binary else browser_binary(args.out.resolve())
        if args.out:
            report["build"] = load_build(args.out.resolve(), binary)
        report["browserSha256"] = sha256_file(binary)
        fixtures = load_fixtures(args.media_dir, args.require_media)
        report["fixtures"] = fixtures
        result = run_probe(binary, args.media_dir, fixtures, args.timeout)
        report["result"] = result
        report["errors"] = validate_result(result, args.require_media, bool(fixtures))
        report["status"] = "failed" if report["errors"] else "passed"
    except (OSError, ValueError, KeyError, TypeError, subprocess.SubprocessError) as error:
        report["errors"].append(str(error))
    rendered = json.dumps(report, indent=2, sort_keys=True) + "\n"
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(rendered, encoding="utf-8")
    print(rendered, end="")
    return 0 if report["status"] == "passed" else 1


if __name__ == "__main__":
    raise SystemExit(main())
