# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
from generator.base_generator import *
from generator.darwin.darwin_base import *
from generator.darwin.sub_code_header_generator import *
from generator.darwin.sub_code_src_generator import *
from generator.darwin.behavior_header_generator import *
from generator.darwin.behavior_src_generator import *

__all__ = ["DarwinGenerator"]

BASE_RELATIVE_PATH = "platform/darwin/common/lynx/"

class DarwinGenerator(PlatformGenerator):
    def __init__(self, meta_data_list):
        super().__init__()
        self._register_child_generator(
            SubErrorCodeHeaderFileGenerator(
                BASE_RELATIVE_PATH,
                SUB_ERR_CLASS_NAME + HEADER_FILE_EXT))
        self._register_child_generator(
            SubErrorCodeSrcFileGenerator(
                BASE_RELATIVE_PATH,
                SUB_ERR_CLASS_NAME + SOURCE_FILE_EXT, 
                meta_data_list))
        self._register_child_generator(
            BehaviorHeaderGenerator(
                BASE_RELATIVE_PATH,
                ERR_BEHAVIOR_CLASS_NAME + HEADER_FILE_EXT))
        self._register_child_generator(
            BehaviorSrcGenerator(
                BASE_RELATIVE_PATH,
                ERR_BEHAVIOR_CLASS_NAME + SOURCE_FILE_EXT))


    


            

