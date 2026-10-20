#!/usr/bin/env python3
# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import os
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parent))

import prepare_cocoapods_sources


class PrepareCocoapodsSourcesTest(unittest.TestCase):
    """Covers how Podfile.flatten entries map to podspec download URLs."""

    def setUp(self):
        self.temp_dir = tempfile.TemporaryDirectory()

    def tearDown(self):
        self.temp_dir.cleanup()

    def load_pods(self, content):
        source_list = os.path.join(self.temp_dir.name, 'Podfile.flatten')
        Path(source_list).write_text(content, encoding='utf8')
        return prepare_cocoapods_sources.load_pods(source_list)

    def test_github_specs_repo_entry_downloads_from_repo_root(self):
        pods = self.load_pods('https://github.com/lynx-family/Specs.git:Lynx:3.5.1 pod\n')

        self.assertEqual(
            'https://raw.githubusercontent.com/lynx-family/Specs/HEAD/'
            'Lynx/3.5.1/Lynx.podspec.json',
            pods[0].source.spec_url('Lynx', '3.5.1', 'podspec.json'),
        )

    def test_cdn_entry_downloads_from_prefixed_specs_dir(self):
        pods = self.load_pods('https://cdn.cocoapods.org/:SnapKit:5.7.1 pod\n')
        source = pods[0].source

        with mock.patch.object(source, '_load_prefix_lengths', return_value=[1, 1, 1]):
            url = source.spec_url('SnapKit', '5.7.1', 'podspec.json')

        self.assertEqual(
            'https://cdn.cocoapods.org/Specs/1/f/6/SnapKit/5.7.1/SnapKit.podspec.json',
            url,
        )

    def test_non_github_git_specs_repo_is_rejected(self):
        with self.assertRaisesRegex(ValueError, 'Unsupported git Specs repo'):
            self.load_pods('https://gitlab.com/acme/Specs.git:Foo:1.0.0 pod\n')


if __name__ == '__main__':
    unittest.main()
