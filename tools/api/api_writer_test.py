#!/usr/bin/env python3
# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import contextlib
import io
import tempfile
import unittest
from pathlib import Path

from api_writer import write_api_metadata
from env_setup import API_DOC_ANNOTATION


class FakeAPIObject:
    def __init__(self, api_str):
        self.api_str = api_str

    def get_api_str(self):
        return self.api_str


class APIWriterTest(unittest.TestCase):
    def test_empty_metadata_keeps_existing_file(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            api_file = Path(temp_dir) / "lynx_android.api"
            api_file.write_text("existing metadata", encoding="utf-8")

            stderr = io.StringIO()
            with contextlib.redirect_stderr(stderr):
                result = write_api_metadata(str(api_file), [], "android")

            self.assertFalse(result)
            self.assertEqual(api_file.read_text(encoding="utf-8"), "existing metadata")
            self.assertIn("generated metadata is empty", stderr.getvalue())

    def test_empty_metadata_does_not_create_new_file(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            api_file = Path(temp_dir) / "lynx_harmony.api"

            stderr = io.StringIO()
            with contextlib.redirect_stderr(stderr):
                result = write_api_metadata(str(api_file), [], "harmony")

            self.assertFalse(result)
            self.assertFalse(api_file.exists())

    def test_valid_metadata_replaces_file(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            api_file = Path(temp_dir) / "lynx_ios.api"
            api_file.write_text("old metadata", encoding="utf-8")

            result = write_api_metadata(
                str(api_file),
                [FakeAPIObject("@interface LynxView\n@end\n\n")],
                "ios",
            )

            self.assertTrue(result)
            self.assertEqual(
                api_file.read_text(encoding="utf-8"),
                API_DOC_ANNOTATION + "@interface LynxView\n@end\n\n",
            )


if __name__ == "__main__":
    unittest.main()
