#!/usr/bin/env python3
# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import hashlib
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest import mock
from urllib.error import HTTPError

sys.path.insert(0, str(Path(__file__).resolve().parent))

import spm_publish_helper as helper


class SpmPublishHelperTest(unittest.TestCase):
    def test_validate_version_rejects_nightly(self):
        helper.validate_version("4.2.0")
        helper.validate_version("4.2.0-alpha.1")
        helper.validate_version("4.2.0-rc.1")

        with self.assertRaisesRegex(ValueError, "Invalid SwiftPM release version"):
            helper.validate_version("4.2.0-nightly.202608280001.1.gabcdef12")

    def test_iter_subspec_names_excludes_non_shipping_subtrees(self):
        spec = {
            "name": "Lynx",
            "subspecs": [
                {"name": "Framework"},
                {
                    "name": "NapiBinding",
                    "subspecs": [{"name": "QuickJS"}],
                },
                {
                    "name": "UnitTests",
                    "subspecs": [{"name": "Fixtures"}],
                },
                {"name": "Replay"},
            ],
        }

        self.assertEqual(
            list(helper.iter_subspec_names(spec, "Lynx")),
            [
                "Lynx/Framework",
                "Lynx/NapiBinding",
                "Lynx/NapiBinding/QuickJS",
            ],
        )

    def test_product_name_replaces_subspec_separator(self):
        self.assertEqual(
            helper.product_name_for_spec("Lynx/NapiBinding/QuickJS"),
            "Lynx-NapiBinding-QuickJS",
        )

    def test_dependency_roots_collects_nested_dependencies(self):
        spec = {
            "name": "Lynx",
            "dependencies": {"LynxBase/Framework": ["= 1.2.3"]},
            "subspecs": [
                {"name": "Framework", "dependencies": {"PrimJS/quickjs": []}},
            ],
        }

        self.assertEqual(
            helper.dependency_roots(spec),
            {"LynxBase", "PrimJS"},
        )

    def test_dependency_closure_handles_cycles(self):
        graph = {
            "Lynx": ["LynxBase", "PrimJS"],
            "LynxBase": ["LynxServiceAPI"],
            "LynxServiceAPI": ["Lynx"],
        }

        self.assertEqual(
            helper.dependency_closure("Lynx", graph),
            {"Lynx", "LynxBase", "LynxServiceAPI", "PrimJS"},
        )

    def test_create_podfile_uses_all_publishable_subspecs(self):
        with tempfile.TemporaryDirectory() as directory:
            work_dir = Path(directory) / "work"
            work_dir.mkdir()
            helper.create_podfile(
                work_dir,
                Path("/repo/lynx"),
                {
                    "Lynx": {
                        "subspecs": [
                            {"name": "Framework"},
                            {"name": "UnitTests"},
                        ]
                    }
                },
            )

            podfile = (work_dir / "Podfile").read_text(encoding="utf-8")

        self.assertIn('pod "Lynx/Framework", :path => "/repo/lynx"', podfile)
        self.assertNotIn("UnitTests", podfile)
        self.assertIn("use_frameworks! :linkage => :static", podfile)
        self.assertIn("installer.pod_targets", podfile)
        self.assertIn(helper.RESOLVED_SPECS_FILE, podfile)

    def test_installed_specs_reads_cocoapods_resolved_export(self):
        with tempfile.TemporaryDirectory() as directory:
            work_dir = Path(directory)
            helper.write_json(
                work_dir / helper.RESOLVED_SPECS_FILE,
                {
                    "Lynx": {"name": "Lynx"},
                    "PrimJS": {"name": "PrimJS"},
                },
            )

            specs = helper.installed_specs(work_dir)

        self.assertEqual(set(specs), {"Lynx", "PrimJS"})

    def test_create_host_project_writes_valid_ruby_source(self):
        with tempfile.TemporaryDirectory() as directory, \
                mock.patch.object(helper, "run") as run:
            work_dir = Path(directory)
            helper.create_host_project(work_dir)
            script = (work_dir / "create_project.rb").read_text(encoding="utf-8")

        self.assertTrue(script.startswith("require 'xcodeproj'\n"))
        self.assertIn("project.save\n", script)
        self.assertNotIn('\n"\n', script)
        run.assert_called_once_with(
            ["bundle", "exec", "ruby", "create_project.rb"],
            cwd=work_dir,
        )

    def test_render_package_creates_binary_and_alias_products(self):
        manifest = self.manifest()

        package = helper.render_package(manifest)

        self.assertIn("platforms: [.iOS(.v10)]", package)
        self.assertIn('name: "Lynx-Framework"', package)
        self.assertIn('name: "Lynx"', package)
        self.assertIn('url: "https://example.com/Lynx.zip"', package)
        self.assertIn('.copy("Resources/LynxResources.bundle")', package)
        self.assertIn('.linkedFramework("WebKit")', package)
        self.assertIn('.linkedLibrary("c++")', package)

    def test_render_package_rejects_artifact_without_url(self):
        manifest = self.manifest()
        manifest["artifacts"][0].pop("url")

        with self.assertRaisesRegex(ValueError, "URL is missing"):
            helper.render_package(manifest)

    def test_render_package_rejects_duplicate_product_names(self):
        manifest = self.manifest()
        manifest["products"].append({
            "name": "Lynx",
            "spec": "Lynx/Alias",
            "targets": ["Lynx"],
        })

        with self.assertRaisesRegex(ValueError, "Duplicate SwiftPM product"):
            helper.render_package(manifest)

    def test_render_package_rejects_invalid_binary_target_name(self):
        manifest = self.manifest()
        manifest["artifacts"][0]["target"] = "Lynx-Core"
        manifest["products"][0]["targets"] = ["Lynx-Core"]
        manifest["products"][1]["targets"] = ["Lynx-Core"]

        with self.assertRaisesRegex(ValueError, "Invalid SwiftPM binary target"):
            helper.render_package(manifest)

    def test_set_github_release_metadata_updates_urls(self):
        with tempfile.TemporaryDirectory() as directory:
            manifest_path = Path(directory) / "PackageArtifacts.json"
            manifest = self.manifest()
            manifest["artifacts"][0].pop("url")
            helper.write_json(manifest_path, manifest)

            helper.set_github_release_metadata(
                manifest_path,
                "lynx-family/lynx",
                "lynx-family/lynx",
                "0123456789abcdef0123456789abcdef01234567",
            )

            release_manifest = helper.read_json(manifest_path)

        self.assertEqual(
            release_manifest["artifacts"][0]["url"],
            "https://github.com/lynx-family/lynx/releases/download/"
            "1.2.3/Lynx-1.2.3.xcframework.zip",
        )
        self.assertEqual(
            release_manifest["github_repository"], "lynx-family/lynx"
        )
        self.assertEqual(
            release_manifest["source_repository"], "lynx-family/lynx"
        )
        self.assertEqual(
            release_manifest["source_revision"],
            "0123456789abcdef0123456789abcdef01234567",
        )

    def test_set_github_release_metadata_requires_complete_source(self):
        with tempfile.TemporaryDirectory() as directory:
            manifest_path = Path(directory) / "PackageArtifacts.json"
            manifest = self.manifest()
            manifest["artifacts"][0].pop("url")
            helper.write_json(manifest_path, manifest)

            with self.assertRaisesRegex(ValueError, "provided together"):
                helper.set_github_release_metadata(
                    manifest_path,
                    "lynx-family/lynx",
                    source_repository="lynx-family/lynx",
                )

    def test_verify_local_artifacts_checks_archive_checksum(self):
        with tempfile.TemporaryDirectory() as directory:
            manifest_path = Path(directory) / "PackageArtifacts.json"
            archive = Path(directory) / "Lynx-1.2.3.xcframework.zip"
            archive.write_bytes(b"archive")
            manifest = self.manifest()
            digest = hashlib.sha256(b"archive").hexdigest()
            manifest["artifacts"][0]["sha256"] = digest
            manifest["artifacts"][0]["checksum"] = digest
            helper.write_json(manifest_path, manifest)

            helper.verify_local_artifacts(manifest_path, expected_version="1.2.3")

            archive.write_bytes(b"tampered")
            with self.assertRaisesRegex(
                    RuntimeError, "Local artifact checksum mismatch"):
                helper.verify_local_artifacts(manifest_path)

    def test_validate_manifest_file_requires_repository(self):
        with tempfile.TemporaryDirectory() as directory:
            manifest_path = Path(directory) / "PackageArtifacts.json"
            helper.write_json(manifest_path, self.manifest())

            with self.assertRaisesRegex(
                    ValueError, "GitHub repository is missing"):
                helper.validate_manifest_file(manifest_path, "1.2.3")

            manifest = self.manifest()
            manifest["artifacts"][0].pop("url")
            helper.write_json(manifest_path, manifest)
            helper.set_github_release_metadata(
                manifest_path,
                "lynx-family/lynx",
                "lynx-family/lynx",
                "0123456789abcdef0123456789abcdef01234567",
            )
            helper.validate_manifest_file(
                manifest_path,
                "1.2.3",
                "lynx-family/lynx",
                "lynx-family/lynx",
                "0123456789abcdef0123456789abcdef01234567",
            )
            with self.assertRaisesRegex(ValueError, "does not match"):
                helper.validate_manifest_file(
                    manifest_path, "1.2.3", "example/lynx"
                )
            with self.assertRaisesRegex(ValueError, "source revision"):
                helper.validate_manifest_file(
                    manifest_path,
                    "1.2.3",
                    "lynx-family/lynx",
                    "lynx-family/lynx",
                    "fedcba9876543210fedcba9876543210fedcba98",
                )

    def test_generate_package_copies_resources_and_metadata(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory) / "repo"
            dist = Path(directory) / "dist"
            resource = dist / "Resources" / "LynxResources.bundle"
            resource.mkdir(parents=True)
            (resource / "lynx_core.js").write_text("core", encoding="utf-8")
            manifest_path = dist / "PackageArtifacts.json"
            helper.write_json(manifest_path, self.manifest())

            helper.generate_package(root, manifest_path)

            self.assertTrue((root / "Package.swift").is_file())
            self.assertTrue(
                (root / "Sources" / helper.PACKAGE_SUPPORT_TARGET / "Resources"
                 / "LynxResources.bundle" / "lynx_core.js").is_file()
            )
            metadata = helper.read_json(root / "spm" / "PackageArtifacts.json")
            self.assertEqual(metadata["version"], "1.2.3")

    def test_verify_remote_checks_version_and_hash(self):
        with tempfile.TemporaryDirectory() as directory:
            manifest_path = Path(directory) / "PackageArtifacts.json"
            helper.write_json(manifest_path, self.manifest())
            with mock.patch.object(
                    helper, "remote_sha256", return_value="a" * 64):
                helper.verify_remote_artifacts(manifest_path, "1.2.3")

            with self.assertRaisesRegex(ValueError, "does not match"):
                helper.verify_remote_artifacts(manifest_path, "1.2.4")

    def test_verify_remote_retries_new_release_asset(self):
        with tempfile.TemporaryDirectory() as directory:
            manifest_path = Path(directory) / "PackageArtifacts.json"
            helper.write_json(manifest_path, self.manifest())
            not_found = HTTPError(
                "https://example.com/Lynx.zip", 404, "Not Found", {}, None
            )
            with mock.patch.object(
                    helper,
                    "remote_sha256",
                    side_effect=[not_found, "a" * 64]), mock.patch.object(
                        helper.time, "sleep") as sleep:
                helper.verify_remote_artifacts(manifest_path, "1.2.3")

        sleep.assert_called_once_with(1)

    @staticmethod
    def manifest():
        digest = "a" * 64
        return {
            "schema_version": 1,
            "version": "1.2.3",
            "deployment_target": "10.0",
            "artifacts": [
                {
                    "pod": "Lynx",
                    "target": "Lynx",
                    "file": "Lynx-1.2.3.xcframework.zip",
                    "url": "https://example.com/Lynx.zip",
                    "sha256": digest,
                    "checksum": digest,
                }
            ],
            "products": [
                {"name": "Lynx", "spec": "Lynx", "targets": ["Lynx"]},
                {
                    "name": "Lynx-Framework",
                    "spec": "Lynx/Framework",
                    "targets": ["Lynx"],
                },
            ],
            "system_frameworks": ["WebKit"],
            "weak_frameworks": [],
            "system_libraries": ["stdc++"],
            "resources": ["LynxResources.bundle"],
        }


if __name__ == "__main__":
    unittest.main()
