#!/usr/bin/env python3
# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import json
import subprocess
import tempfile
import unittest
from pathlib import Path

from sparkling_source_validation import (
    SPARKLING_SOURCE_PODS,
    load_manifest,
    validate_checkout,
    validate_source_destination,
)


class SparklingSourceValidationTest(unittest.TestCase):
    def test_manifest_validation_has_one_shared_schema(self):
        with tempfile.TemporaryDirectory() as temporary_directory:
            manifest_path = Path(temporary_directory) / "sparkling-source.json"
            manifest_path.write_text(
                json.dumps(
                    {
                        "repository": "https://example.com/sparkling.git",
                        "commit": "a" * 40,
                        "removal_condition": "Use a released source package.",
                        "pods": {
                            pod: f"packages/{pod}" for pod in SPARKLING_SOURCE_PODS
                        },
                    }
                ),
                encoding="utf-8",
            )

            manifest, errors = load_manifest(manifest_path)

            self.assertEqual(errors, [])
            self.assertEqual(set(manifest["pods"]), SPARKLING_SOURCE_PODS)

    def test_package_manager_destination_is_rejected(self):
        with tempfile.TemporaryDirectory() as temporary_directory:
            project_dir = Path(temporary_directory)
            source_root, errors = validate_source_destination(
                project_dir / "node_modules" / "sparkling",
                project_dir,
                {"HOME": str(project_dir / "home")},
            )

            self.assertIn("node_modules", str(source_root))
            self.assertTrue(any("package-manager owned" in error for error in errors))

    def test_clean_detached_checkout_matches_manifest(self):
        with tempfile.TemporaryDirectory() as temporary_directory:
            source_root = Path(temporary_directory) / "sparkling"
            source_root.mkdir()
            self._git(source_root, "init", "--quiet")
            self._git(
                source_root,
                "config",
                "user.email",
                "sparkling-validation@example.com",
            )
            self._git(source_root, "config", "user.name", "Sparkling Validation")
            repository = "https://example.com/sparkling.git"
            self._git(source_root, "remote", "add", "origin", repository)

            pods = {}
            for pod in SPARKLING_SOURCE_PODS:
                relative_path = Path("packages") / pod
                pods[pod] = str(relative_path)
                pod_directory = source_root / relative_path
                pod_directory.mkdir(parents=True)
                (pod_directory / f"{pod}.podspec").write_text(
                    f"Pod::Spec.new do |spec|\n  spec.name = '{pod}'\nend\n",
                    encoding="utf-8",
                )
            self._git(source_root, "add", ".")
            self._git(source_root, "commit", "--quiet", "-m", "test fixture")
            commit = self._git(source_root, "rev-parse", "HEAD").stdout.strip()
            self._git(source_root, "checkout", "--quiet", "--detach", commit)

            errors = validate_checkout(
                source_root,
                {
                    "repository": repository,
                    "commit": commit,
                    "removal_condition": "Use a released source package.",
                    "pods": pods,
                },
            )

            self.assertEqual(errors, [])

    @staticmethod
    def _git(source_root, *arguments):
        return subprocess.run(
            ["git", "-C", str(source_root), *arguments],
            check=True,
            capture_output=True,
            text=True,
        )


if __name__ == "__main__":
    unittest.main()
