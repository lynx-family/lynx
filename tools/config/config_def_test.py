# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import unittest

from config_def import Config


class ConfigPlatformTagsTest(unittest.TestCase):
    def create_config(self, support_platform):
        return Config(
            name="testConfig",
            desc="Test config.",
            default_value="true",
            js_default_value="undefined",
            value_type="bool",
            js_value_type="undefined",
            since="4.2",
            deprecated=[],
            support_platform=support_platform,
            sync_to=[],
            version_overrides=[],
            author="test",
            code_gen=["ALL"],
            name_as={},
            bind_member_to="",
            read_settings=False,
            read_native=False,
            export=True,
        )

    def test_clay_mobile_platform_tags(self):
        config = self.create_config(["ClayAndroid", "ClayIOS"])

        self.assertTrue(config._check_support_platform())
        self.assertEqual(config.platform_tags, ["@ClayAndroid", "@ClayIOS"])


if __name__ == "__main__":
    unittest.main()
