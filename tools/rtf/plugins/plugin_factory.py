# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
from core.utils.log import Log
from plugins.android_ut_plugin import AndroidUTPlugin
from plugins.coverage_check_plugin import CoverageCheckerPlugin
from plugins.fuzzer_test_plugin import FuzzerTestPlugin
from plugins.native_ut_plugin import NativeUTPlugin
from plugins.project_init_plugin import InitPlugin


def PluginFactory(plugin_name):
    if plugin_name == "NativeUT":
        return NativeUTPlugin()
    elif plugin_name == "Init":
        return InitPlugin()
    elif plugin_name == "CoverageChecker":
        return CoverageCheckerPlugin()
    elif plugin_name == "AndroidUT":
        return AndroidUTPlugin()
    elif plugin_name == "FuzzerTest":
        return FuzzerTestPlugin()
    else:
        Log.fatal(f"{plugin_name} not found!")
