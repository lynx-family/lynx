#!/usr/bin/env python3
# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import unittest
from unittest import mock

import main as api_main


class APIMainTest(unittest.TestCase):
    def test_update_api_metadata_for_platforms_runs_all_platforms(self):
        platform_results = {
            "ios": True,
            "android": False,
            "harmony": True,
        }

        with mock.patch.object(
            api_main,
            "_update_platform_api_metadata",
            side_effect=lambda platform: platform_results[platform],
        ) as update_platform:
            result = api_main._update_api_metadata_for_platforms(
                ["ios", "android", "harmony"]
            )

        self.assertEqual(result, platform_results)
        update_platform.assert_has_calls(
            [mock.call("ios"), mock.call("android"), mock.call("harmony")],
            any_order=True,
        )


if __name__ == "__main__":
    unittest.main()
