"""Behavioral regressions for the Chromium build and acceptance path (stdlib only)."""
from __future__ import annotations

import base64
from contextlib import redirect_stdout
from http.client import HTTPConnection
import io
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import threading
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "engine/chromium"))
import bootstrap
import knogn_overlay as overlay
import probe_runtime as probe


def make_source(root):
    contents = {
        "chrome/app/theme/chromium/BRANDING": b"PRODUCT_FULLNAME=Chromium\nCOMPANY_FULLNAME=The Chromium Authors\nOTHER=preserved\n",
        "chrome/browser/metrics/chrome_metrics_services_manager_client.cc": (
            b'// upstream\nBASE_FEATURE(kMetricsReportingFeature,\n             "MetricsReporting",\n'
            b'             base::FEATURE_ENABLED_BY_DEFAULT);\n'),
        "chrome/app/theme/chromium/win/chromium.ico": b"upstream-icon",
        "chrome/app/theme/chromium/product_logo.svg": b"<svg>upstream</svg>",
    }
    for name, content in contents.items():
        path = root / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(content)
    return contents


def passing_result():
    played = {"status": "passed", "ended": True, "currentTime": 2.0, "decodedFrames": 48}
    return {"schemaVersion": 1, "complete": True, "secureContext": True,
            "chromiumBrowserIdentity": True, "credentialsApi": True, "webauthn": True,
            "h264Aac": "probably", "mseH264Aac": True,
            "playback": {name: dict(played) for name in ("webmDirect", "h264Direct", "h264Mse")}}


class OverlayTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.originals = make_source(self.root)

    def apply(self):
        with redirect_stdout(io.StringIO()):
            overlay.apply(self.root)

    def test_apply_restore_roundtrip_preserves_upstream_bytes(self):
        self.apply()
        branding = (self.root / "chrome/app/theme/chromium/BRANDING").read_text()
        self.assertIn("PRODUCT_FULLNAME=Knogn", branding)
        self.assertIn("OTHER=preserved", branding)
        with redirect_stdout(io.StringIO()):
            overlay.restore(self.root)
        self.assertFalse((self.root / overlay.MARKER).exists())
        for name, content in self.originals.items():
            self.assertEqual((self.root / name).read_bytes(), content)

    def test_apply_twice_is_idempotent(self):
        self.apply()
        manifest = (self.root / overlay.MARKER).read_bytes()
        self.apply()
        self.assertEqual((self.root / overlay.MARKER).read_bytes(), manifest)

    def test_source_drift_does_not_partially_rebrand(self):
        metrics = self.root / "chrome/browser/metrics/chrome_metrics_services_manager_client.cc"
        metrics.write_text("changed upstream pattern")
        with self.assertRaisesRegex(SystemExit, "exactly one"):
            self.apply()
        self.assertEqual((self.root / "chrome/app/theme/chromium/BRANDING").read_bytes(), self.originals["chrome/app/theme/chromium/BRANDING"])
        self.assertFalse((self.root / overlay.MARKER).exists())

    def test_edited_overlay_blocks_restore_without_touching_other_files(self):
        self.apply()
        icon = self.root / "chrome/app/theme/chromium/win/chromium.ico"
        icon.write_bytes(b"user artwork")
        before = {name: (self.root / name).read_bytes() for name in self.originals}
        with self.assertRaisesRegex(SystemExit, "local edits"):
            overlay.restore(self.root)
        for name, content in before.items():
            self.assertEqual((self.root / name).read_bytes(), content)

    def test_legacy_marker_is_not_destructively_reset(self):
        (self.root / overlay.MARKER).write_text("Knogn Chromium source overlay applied.\n")
        with self.assertRaisesRegex(SystemExit, "clean checkout"):
            overlay.restore(self.root)

    def test_manifest_cannot_restore_arbitrary_paths(self):
        self.apply()
        marker = self.root / overlay.MARKER
        manifest = json.loads(marker.read_text())
        manifest["files"]["../../unrelated"] = next(iter(manifest["files"].values()))
        marker.write_text(json.dumps(manifest))
        with self.assertRaisesRegex(SystemExit, "unmanaged path"):
            overlay.restore(self.root)

    def test_corrupted_backup_blocks_restore(self):
        self.apply()
        marker = self.root / overlay.MARKER
        manifest = json.loads(marker.read_text())
        next(iter(manifest["files"].values()))["original"] = base64.b64encode(b"wrong").decode()
        marker.write_text(json.dumps(manifest))
        with self.assertRaisesRegex(SystemExit, "invalid original"):
            overlay.restore(self.root)


class BootstrapTests(unittest.TestCase):
    def test_profiles_have_separate_object_and_binary_directories(self):
        src = Path("checkout")
        self.assertNotEqual(bootstrap.output_directory(src, False), bootstrap.output_directory(src, True))
        self.assertEqual(bootstrap.output_directory(src, True), src / "out/KnognMedia")

    def test_dry_run_is_side_effect_free_and_orders_restore_sync_hooks(self):
        with tempfile.TemporaryDirectory() as temp:
            workspace = Path(temp) / "absent"
            result = subprocess.run([sys.executable, str(bootstrap.HERE / "bootstrap.py"), "--workspace", str(workspace), "--media-experiment", "--build"], capture_output=True, text=True, check=True)
            self.assertFalse(workspace.exists())
            self.assertLess(result.stdout.index("--restore"), result.stdout.index("git checkout"))
            self.assertLess(result.stdout.index("gclient sync -D --nohooks"), result.stdout.index("gclient runhooks"))
            self.assertIn("out" + "/" + "KnognMedia" if sys.platform != "win32" else "KnognMedia", result.stdout)

    def test_nonpositive_parallelism_rejected(self):
        for count in ("0", "-1"):
            result = subprocess.run([sys.executable, str(bootstrap.HERE / "bootstrap.py"), "--jobs", count], capture_output=True, text=True)
            self.assertEqual(result.returncode, 2)

    def test_dirty_checkout_is_preserved_and_rejected(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            subprocess.run(["git", "init", "-q", str(root)], check=True)
            file = root / "source.cc"
            file.write_text("original")
            subprocess.run(["git", "add", "."], cwd=root, check=True)
            subprocess.run(["git", "-c", "user.name=Test", "-c", "user.email=test@example.invalid", "commit", "-qm", "initial"], cwd=root, check=True)
            file.write_text("local change")
            with self.assertRaisesRegex(SystemExit, "local edits"):
                bootstrap.require_clean_checkout(root)
            self.assertEqual(file.read_text(), "local change")

    def test_local_entrypoint_runs_preflight_before_checkout(self):
        with patch.object(sys, "argv", ["bootstrap.py", "--execute"]), patch.object(bootstrap.subprocess, "run", side_effect=subprocess.CalledProcessError(1, "preflight")) as run:
            with redirect_stdout(io.StringIO()), self.assertRaises(subprocess.CalledProcessError):
                bootstrap.main()
            self.assertEqual(run.call_count, 1)
            self.assertTrue(run.call_args.args[0][1].endswith("preflight.py"))


class AcceptanceTests(unittest.TestCase):
    def test_complete_direct_and_mse_evidence_passes(self):
        self.assertEqual(probe.validate_result(passing_result(), True, True), [])

    def test_advertised_codec_alone_cannot_pass_playback(self):
        result = passing_result()
        result["playback"] = {}
        self.assertEqual(len(probe.validate_result(result, True, True)), 3)

    def test_required_capabilities_must_be_present_and_typed(self):
        for field in ("chromiumBrowserIdentity", "credentialsApi", "webauthn", "secureContext", "complete", "mseH264Aac"):
            result = passing_result()
            result[field] = "true"
            with self.subTest(field=field):
                self.assertTrue(probe.validate_result(result, True, True))
        result = passing_result()
        del result["h264Aac"]
        self.assertTrue(probe.validate_result(result, True, True))

    def test_playback_requires_time_frames_and_end(self):
        for field, value in (("currentTime", 0), ("decodedFrames", 0), ("currentTime", float('nan')), ("decodedFrames", True), ("ended", False), ("status", "failed")):
            result = passing_result()
            result["playback"]["h264Mse"][field] = value
            with self.subTest(field=field):
                self.assertTrue(probe.validate_result(result, True, True))

    def test_optional_playback_failure_is_not_ignored(self):
        result = passing_result()
        result["playback"]["h264Mse"]["status"] = "failed"
        self.assertTrue(probe.validate_result(result, False, True))

    def test_media_gate_requires_fixtures(self):
        with self.assertRaisesRegex(ValueError, "needs --media-dir"):
            probe.load_fixtures(None, True)

    def test_fixture_tampering_rejected(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            sample = root / "sample.webm"
            sample.write_bytes(b"sample")
            manifest = {"schemaVersion": 1, "files": {"sample.webm": probe.sha256_file(sample)}}
            (root / "fixtures.json").write_text(json.dumps(manifest))
            self.assertEqual(probe.load_fixtures(root, False), manifest)
            sample.write_bytes(b"changed")
            with self.assertRaisesRegex(ValueError, "checksum"):
                probe.load_fixtures(root, False)

    def test_manifest_detects_stale_binary_and_args(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            binary = root / "chrome"
            binary.write_bytes(b"browser")
            gn = root / "args.gn"
            gn.write_text("proprietary_codecs = false")
            manifest = {"schemaVersion": 1, "status": "built", "overlaySha256": "present",
                        "browser": {"path": "chrome", "sha256": probe.sha256_file(binary)}, "gnArgsSha256": probe.sha256_file(gn)}
            (root / "knogn-build.json").write_text(json.dumps(manifest))
            self.assertEqual(probe.load_build(root, binary), manifest)
            binary.write_bytes(b"stale browser")
            with self.assertRaisesRegex(ValueError, "executable"):
                probe.load_build(root, binary)
            binary.write_bytes(b"browser")
            gn.write_text("proprietary_codecs = true")
            with self.assertRaisesRegex(ValueError, "GN arguments"):
                probe.load_build(root, binary)

    def test_failure_report_is_written_even_when_browser_missing(self):
        with tempfile.TemporaryDirectory() as temp:
            report = Path(temp) / "report.json"
            with patch.object(sys, "argv", ["probe_runtime.py", "--out", temp, "--report", str(report)]), redirect_stdout(io.StringIO()):
                self.assertEqual(probe.main(), 1)
            data = json.loads(report.read_text())
            self.assertEqual(data["status"], "failed")
            self.assertFalse(data["releaseAcceptance"])


class ProbeTransportTests(unittest.TestCase):
    def setUp(self):
        self.server = probe.ProbeServer(None, {})
        self.thread = threading.Thread(target=self.server.serve_forever, daemon=True)
        self.thread.start()
        self.addCleanup(self.cleanup)

    def cleanup(self):
        self.server.shutdown()
        self.server.server_close()
        self.thread.join()

    def request(self, method, path, body=None, headers=None):
        connection = HTTPConnection("127.0.0.1", self.server.server_port, timeout=5)
        connection.request(method, path, body=body, headers=headers or {})
        response = connection.getresponse()
        status, content = response.status, response.read()
        connection.close()
        return status, content

    def test_nested_results_with_html_characters_roundtrip_exactly(self):
        result = passing_result()
        result["nested"] = {"string": 'a < b & "quoted" } value', "object": {"inner": True}}
        status, _ = self.request("POST", f"/{self.server.token}/result", json.dumps(result), {"Origin": self.server.origin})
        self.assertEqual(status, 200)
        self.assertTrue(self.server.finished.wait(1))
        self.assertEqual(self.server.result, result)

    def test_foreign_origin_and_unknown_token_rejected(self):
        for token, origin in (("wrong", self.server.origin), (self.server.token, "https://example.com")):
            status, _ = self.request("POST", f"/{token}/result", "{}", {"Origin": origin})
            self.assertEqual(status, 403)
        self.assertFalse(self.server.finished.is_set())

    def test_only_explicit_probe_routes_are_served(self):
        for path in ("/", f"/{self.server.token}/media/../../README.md"):
            self.assertEqual(self.request("GET", path)[0], 404)
        status, content = self.request("GET", f"/{self.server.token}/")
        self.assertEqual(status, 200)
        self.assertNotIn(b"__KNOGN_CONFIG__", content)

    def test_oversized_and_malformed_results_rejected(self):
        for body in ("[1]", "not json", "x" * (65536 + 1)):
            self.assertEqual(self.request("POST", f"/{self.server.token}/result", body, {"Origin": self.server.origin})[0], 400)
        self.assertFalse(self.server.finished.is_set())


if __name__ == "__main__":
    unittest.main()
