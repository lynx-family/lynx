# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
from generator.base_generator import *
from generator.harmony.harmony_common import *
from generator.harmony.sub_code_file_generator import *
from generator.harmony.behavior_file_generator import *

__all__ =  ["HarmonyGenerator"]

BASE_RELATIVE_PATH = "platform/harmony/lynx_harmony/src/main/ets/tasm/gen/"

class HarmonyGenerator(PlatformGenerator):
    def __init__(self, meta_data_list):
        super().__init__()
        self._register_child_generator(
            SubErrCodeFileGenerator(
                BASE_RELATIVE_PATH,
                SUB_ERR_CLASS_NAME + FILE_EXT,
                meta_data_list))
        self._register_child_generator(
            ErrBehaviorFileGenerator(
                BASE_RELATIVE_PATH,
                ERR_BEHAVIOR_CLASS_NAME + FILE_EXT))

