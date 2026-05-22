#!/usr/bin/env python3
# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import json
import os
import subprocess
import sys
import tempfile
import unittest
from unittest import mock
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import cocoapods_publish_helper as helper


class CocoapodsPublishHelperTest(unittest.TestCase):
    def setUp(self):
        self.cwd = os.getcwd()
        self.temp_dir = tempfile.TemporaryDirectory()
        os.chdir(self.temp_dir.name)

    def tearDown(self):
        os.chdir(self.cwd)
        self.temp_dir.cleanup()

    def write_podspec_json(self, component, content):
        with open(f'{component}.podspec.json', 'w', encoding='utf8') as f:
            json.dump(content, f)

    def read_podspec_json(self, component):
        with open(f'{component}.podspec.json', 'r', encoding='utf8') as f:
            return json.load(f)

    def test_use_local_pod_source_keeps_http_source_format(self):
        self.write_podspec_json('Lynx', {
            'version': '1.2.3',
            'source': {'git': 'https://example.com/old.git'},
        })

        helper.use_local_pod_source('Lynx')

        content = self.read_podspec_json('Lynx')
        self.assertEqual(
            content['source'],
            {'http': f'file:{os.getcwd()}/Lynx-1.2.3.zip'},
        )

    def test_use_git_pod_source_writes_commit_source_and_preserves_prepare_command(self):
        self.write_podspec_json('Lynx', {
            'version': '1.2.3',
            'source': {'http': 'https://example.com/Lynx.zip'},
            'prepare_command': 'python3 tools/js_tools/build.py --platform ios',
        })

        helper.use_git_pod_source(
            'Lynx',
            'https://github.com/lynx-family/lynx.git',
            'commit',
            'abcdef123456',
        )

        content = self.read_podspec_json('Lynx')
        self.assertEqual(content['source'], {
            'git': 'https://github.com/lynx-family/lynx.git',
            'commit': 'abcdef123456',
        })
        self.assertEqual(
            content['prepare_command'],
            'python3 tools/js_tools/build.py --platform ios',
        )

    def test_missing_git_source_ref_fails_with_clear_error(self):
        with self.assertRaisesRegex(ValueError, '--git-source-ref is required'):
            helper.validate_git_source_options('commit', '')

    def test_zip_source_type_accepts_empty_git_options(self):
        helper.validate_source_type_options('zip', '', '', '')

    def test_missing_git_source_url_fails_with_clear_error(self):
        with self.assertRaisesRegex(ValueError, '--git-source-url is required'):
            helper.validate_source_type_options('git', None, 'commit', 'abcdef123456')

    def test_missing_git_source_ref_type_fails_with_clear_error(self):
        with self.assertRaisesRegex(ValueError, '--git-source-ref-type is required'):
            helper.validate_source_type_options(
                'git',
                'https://github.com/lynx-family/lynx.git',
                None,
                'abcdef123456',
            )

    def test_publish_component_skips_existing_pod_version(self):
        self.write_podspec_json('Lynx', {'version': '1.2.3'})

        with mock.patch.object(helper, 'is_pod_version_published', return_value=True), \
                mock.patch.object(helper, 'run_command') as run_command:
            helper.publish_component('Lynx', None)

        run_command.assert_not_called()

    def test_publish_component_treats_confirmed_version_after_failure_as_success(self):
        self.write_podspec_json('Lynx', {'version': '1.2.3'})

        with mock.patch.object(helper, 'is_pod_version_published',
                               side_effect=[False, True]), \
                mock.patch.object(helper, 'run_command',
                                  side_effect=subprocess.CalledProcessError(1, 'pod trunk push')), \
                mock.patch.object(helper.time, 'sleep') as sleep:
            helper.publish_component('Lynx', None)

        sleep.assert_not_called()

    def test_publish_component_retries_with_exponential_backoff(self):
        self.write_podspec_json('Lynx', {'version': '1.2.3'})

        with mock.patch.object(helper, 'is_pod_version_published', return_value=False), \
                mock.patch.object(
                    helper,
                    'run_command',
                    side_effect=[
                        subprocess.CalledProcessError(1, 'pod trunk push'),
                        subprocess.CalledProcessError(1, 'pod trunk push'),
                        None,
                    ],
                ) as run_command, \
                mock.patch.object(helper.time, 'sleep') as sleep:
            helper.publish_component('Lynx', None)

        self.assertEqual(run_command.call_count, 3)
        sleep.assert_has_calls([
            mock.call(helper.PUBLISH_RETRY_INITIAL_DELAY_SECONDS),
            mock.call(helper.PUBLISH_RETRY_INITIAL_DELAY_SECONDS * 2),
        ])

    def test_publish_component_raises_after_retry_attempts(self):
        self.write_podspec_json('Lynx', {'version': '1.2.3'})

        with mock.patch.object(helper, 'is_pod_version_published', return_value=False), \
                mock.patch.object(helper, 'run_command',
                                  side_effect=subprocess.CalledProcessError(1, 'pod trunk push')), \
                mock.patch.object(helper.time, 'sleep') as sleep, \
                self.assertRaises(subprocess.CalledProcessError):
            helper.publish_component('Lynx', None)

        self.assertEqual(sleep.call_count, helper.PUBLISH_RETRY_ATTEMPTS - 1)

    def test_build_publish_command_quotes_shell_arguments(self):
        command = helper.build_publish_command(
            'Lynx Kit',
            'trunk,private specs; echo injected',
        )

        self.assertIn(
            "pod trunk push 'Lynx Kit.podspec.json' --verbose",
            command,
        )
        self.assertIn(
            " --sources='trunk,private specs; echo injected'",
            command,
        )

    def test_is_pod_version_published_ignores_decode_failure(self):
        bad_encoding = UnicodeDecodeError(
            'utf-8',
            b'\xff',
            0,
            1,
            'invalid start byte',
        )

        with mock.patch.object(helper, 'urlopen') as urlopen, \
                mock.patch.object(helper.json, 'load', side_effect=bad_encoding):
            urlopen.return_value.__enter__.return_value = object()

            self.assertFalse(helper.is_pod_version_published('Lynx', '1.2.3'))


if __name__ == '__main__':
    unittest.main()
