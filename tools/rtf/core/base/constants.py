# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

from enum import Enum, auto


class AutoEnum(Enum):
    def _generate_next_value_(name, start, count, last_values):
        return count


class Constants(AutoEnum):
    _ = auto()
    BUILDER_BUILD_ERR = auto()
    COVERAGE_CHECKER_BUILD_ERR = auto()
    COVERAGE_CHECKER_RUN_ERR = auto()
    COVERAGE_CHECKER_FAILED = auto()
    COVERAGE_BUILD_ERR = auto()
    COVERAGE_GENERATE_ERR = auto()
    TARGET_BUILD_ERR = auto()
    TARGET_RUN_ERR = auto()
    TARGET_RUN_TIMEOUT_ERR = auto()
    CALL_COMMAND_ERR = auto()
    ANDROID_APK_INSTALL_ERR = auto()
    ANDROID_EMULATOR_PREPARE_ERR = auto()
    ANDROID_REAL_DEVICE_PREPARE_ERR = auto()
    PLUGIN_BUILD_ERR = auto()
    TARGET_SELECTION_ERR = auto()
