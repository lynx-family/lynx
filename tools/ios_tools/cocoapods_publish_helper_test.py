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

    def test_get_enable_trace_param_enables_all_version_types(self):
        for version in ('4.0.0', '4.0.0-alpha.1', '4.0.0-rc.1', '4.0.0-dev'):
            with self.subTest(version=version):
                self.assertEqual(helper.get_enable_trace_param(version), '--enable-trace')

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

    def test_upload_zip_sources_to_s3_uploads_zip_and_rewrites_source(self):
        self.write_podspec_json('Lynx', {
            'version': '1.2.3',
            'source': {'http': 'https://example.com/Lynx.zip'},
        })
        Path('Lynx.podspec').write_text('', encoding='utf8')
        Path('Lynx-1.2.3.zip').write_bytes(b'zip')
        upload_domain = 'upload-host'
        public_domain = 'public-host'

        with mock.patch.dict(os.environ, {
            helper.S3_ACCESS_KEY_ENV: 'ak',
            helper.S3_SECRET_KEY_ENV: 'sk',
            helper.S3_BUCKET_ENV: 'build-artifacts',
            helper.S3_REGION_ENV: 'ap-southeast-1',
            helper.S3_UPLOAD_DOMAIN_ENV: upload_domain,
            helper.S3_PUBLIC_DOMAIN_ENV: public_domain,
            helper.S3_PATH_PREFIX_ENV: 'ios-sdk',
        }), mock.patch.object(helper, 'upload_file_to_s3') as upload:
            helper.upload_zip_sources_to_s3(
                os.getcwd(),
                'Lynx',
            )

        upload.assert_called_once_with(
            os.path.join(os.getcwd(), 'Lynx-1.2.3.zip'),
            'build-artifacts',
            'ap-southeast-1',
            upload_domain,
            'ios-sdk/1.2.3/Lynx-1.2.3.zip',
            'ak',
            'sk',
        )
        content = self.read_podspec_json('Lynx')
        self.assertEqual(content['source'], {
            'http': f'https://build-artifacts.{public_domain}/ios-sdk/1.2.3/Lynx-1.2.3.zip',
        })

    def test_generate_zip_file_quotes_shell_arguments(self):
        Path('Lynx.podspec').write_text('', encoding='utf8')

        with mock.patch.object(helper, 'run_command') as run_command:
            helper.generate_zip_file(os.getcwd(), '1.2.3; echo injected', 'Lynx')

        run_command.assert_called_once_with(
            "export PACKAGE_ENV=prod && "
            "geniospkg --output_type both --repo Lynx "
            "--tag '1.2.3; echo injected' --cache_path ."
        )

    def test_github_release_storage_type_accepts_empty_s3_environment(self):
        with mock.patch.dict(os.environ, {}, clear=True):
            helper.validate_storage_type_options(helper.STORAGE_TYPE_GITHUB_RELEASE)

    def test_s3_storage_type_requires_s3_bucket(self):
        with mock.patch.dict(os.environ, {}, clear=True), \
                self.assertRaisesRegex(ValueError, helper.S3_BUCKET_ENV):
            helper.validate_storage_type_options(helper.STORAGE_TYPE_S3)

    def test_s3_storage_type_accepts_s3_environment(self):
        with mock.patch.dict(os.environ, {
            helper.S3_ACCESS_KEY_ENV: 'ak',
            helper.S3_SECRET_KEY_ENV: 'sk',
            helper.S3_BUCKET_ENV: 'build-artifacts',
            helper.S3_REGION_ENV: 'ap-southeast-1',
            helper.S3_UPLOAD_DOMAIN_ENV: 'upload-host',
            helper.S3_PUBLIC_DOMAIN_ENV: 'public-host',
        }, clear=True):
            helper.validate_storage_type_options(helper.STORAGE_TYPE_S3)

    def test_upload_file_to_s3_uses_s3_endpoint_and_sigv4_headers(self):
        Path('Lynx-1.2.3.zip').write_bytes(b'zip')

        class FakeResponse:
            status = 200
            reason = 'OK'

            def read(self):
                return b''

        class FakeConnection:
            instances = []

            def __init__(self, host, timeout):
                self.host = host
                self.timeout = timeout
                self.request_args = None
                FakeConnection.instances.append(self)

            def request(self, method, path, body=None, headers=None):
                self.request_args = {
                    'method': method,
                    'path': path,
                    'body': body.read(),
                    'headers': headers,
                }

            def getresponse(self):
                return FakeResponse()

            def close(self):
                pass

        with mock.patch.object(helper.http.client, 'HTTPSConnection', FakeConnection):
            helper.upload_file_to_s3(
                'Lynx-1.2.3.zip',
                'build-artifacts',
                'ap-southeast-1',
                'upload-host',
                'ios-sdk/1.2.3/Lynx-1.2.3.zip',
                'ak',
                'sk',
            )

        connection = FakeConnection.instances[0]
        request = connection.request_args
        self.assertEqual(connection.host, 'build-artifacts.upload-host')
        self.assertEqual(connection.timeout, 120)
        self.assertEqual(request['method'], 'PUT')
        self.assertEqual(request['path'], '/ios-sdk/1.2.3/Lynx-1.2.3.zip')
        self.assertEqual(request['body'], b'zip')
        self.assertEqual(request['headers']['Host'], connection.host)
        self.assertEqual(request['headers']['Content-Type'], 'application/zip')
        self.assertEqual(request['headers']['Content-Length'], '3')
        self.assertEqual(
            request['headers']['x-amz-content-sha256'],
            '4a70fe9aa6436e02c2dea340fbd1e352e4ef2d8ce6ca52ad25d4b95471fc8bf2',
        )
        self.assertIn(
            'Credential=ak/',
            request['headers']['Authorization'],
        )
        self.assertNotIn('sk', request['headers']['Authorization'])

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
