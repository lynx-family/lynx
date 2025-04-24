# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

from enum import Enum, auto


class AutoEnum(Enum):
    def _generate_next_value_(name, start, count, last_values):
        return count


class Constants(AutoEnum):
    ANDROID_APK_INSTALL_ERR = auto()
