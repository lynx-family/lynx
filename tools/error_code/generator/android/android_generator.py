# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
from generator.base_generator import *
from generator.android.sub_code_file_generator import *
from generator.android.behavior_file_generator import *
from generator.android.android_base import *

__all__ = ["AndroidGenerator"]

BASE_RELATIVE_PATH = "platform/android/lynx_android/src/main/java/com/lynx/tasm/"

class AndroidGenerator(PlatformGenerator):
    def __init__(self, meta_data_list):
        super().__init__()
        self._register_child_generator(
            SubErrorCodeFileGenerator(
                BASE_RELATIVE_PATH, 
                SUB_ERR_CLASS_NAME + ANDROID_FILE_EXT, 
                meta_data_list))
        self._register_child_generator(
            ErrorBehaviorFileGenerator(
                BASE_RELATIVE_PATH,
                ERR_BEHAVIOR_CLASS_NAME + ANDROID_FILE_EXT))      

        