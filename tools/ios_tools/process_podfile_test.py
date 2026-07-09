#!/usr/bin/env python3
# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import os
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import process_podfile


class ProcessPodfileTest(unittest.TestCase):
    """Covers Podfile rewrite regressions used by ios-integration-test.

    These tests ensure we do not duplicate pod sources and that Lynx-related
    pods are pinned to the local CocoaPods source instead of falling back to
    trunk for spec resolution.
    """

    def setUp(self):
        self.temp_dir = tempfile.TemporaryDirectory()
        self.podfile_path = os.path.join(self.temp_dir.name, 'Podfile')

    def tearDown(self):
        self.temp_dir.cleanup()

    def write_podfile(self, content):
        Path(self.podfile_path).write_text(content, encoding='utf8')

    def read_podfile(self):
        return Path(self.podfile_path).read_text(encoding='utf8')

    def test_insert_source_to_podfile_skips_duplicate_source(self):
        source = 'file:///Users/runner/.cocoapods/repos/local_pod'
        self.write_podfile(
            f"source '{source}'\n"
            "source 'https://cdn.cocoapods.org/'\n"
            "target 'Hello-Lynx-OC' do\n"
            "  pod 'Lynx', '0.0.1-alpha.1'\n"
            "end\n"
        )

        process_podfile.insert_source_to_podfile(source, self.podfile_path)

        self.assertEqual(self.read_podfile().count(source), 1)

    def test_pin_pod_source_adds_source_to_matching_component(self):
        source = 'file:///Users/runner/.cocoapods/repos/local_pod'
        self.write_podfile(
            "source 'https://cdn.cocoapods.org/'\n"
            "target 'Hello-Lynx-OC' do\n"
            "  pod 'Lynx', '0.0.1-alpha.1', :subspecs => ['Framework']\n"
            "  pod 'LynxService', '0.0.1-alpha.1', :subspecs => ['Image']\n"
            "  pod 'SDWebImage', '5.15.5'\n"
            "end\n"
        )

        process_podfile.pin_pod_source('Lynx', source, self.podfile_path)
        process_podfile.pin_pod_source('LynxService', source, self.podfile_path)

        content = self.read_podfile()
        self.assertIn(
            f"pod 'Lynx', '0.0.1-alpha.1', :source => '{source}', :subspecs => ['Framework']",
            content,
        )
        self.assertIn(
            f"pod 'LynxService', '0.0.1-alpha.1', :source => '{source}', :subspecs => ['Image']",
            content,
        )
        self.assertIn("pod 'SDWebImage', '5.15.5'", content)

    def test_pin_pod_source_replaces_existing_source(self):
        source = 'file:///Users/runner/.cocoapods/repos/local_pod'
        self.write_podfile(
            "target 'Hello-Lynx-OC' do\n"
            "  pod 'XElement', '0.0.1-alpha.1', :source => 'https://cdn.cocoapods.org/'\n"
            "end\n"
        )

        process_podfile.pin_pod_source('XElement', source, self.podfile_path)

        self.assertIn(
            f"pod 'XElement', '0.0.1-alpha.1', :source => '{source}'",
            self.read_podfile(),
        )


if __name__ == '__main__':
    unittest.main()
