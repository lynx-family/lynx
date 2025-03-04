# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
from common import *
from generator.base_generator import *
from generator.native.native_common import *
from generator.native.sub_code_header_generator import *
from generator.native.sub_code_src_generator import *

__all__ = ["NativeGenerator"]

BASE_RELATIVE_PATH = "core/build/gen/"

class NativeGenerator(PlatformGenerator):
    def __init__(self):
        super().__init__()
        file_name = to_lower_snake(SUB_ERR_CLASS_NAME)
        self._register_child_generator(
            NativeSubCodeHeaderFileGenerator(
                BASE_RELATIVE_PATH,
                "{0}{1}".format(file_name, HEADER_FILE_EXT)))
        self._register_child_generator(
            NativeSubCodeSrcGenerator(
                BASE_RELATIVE_PATH,
                "{0}{1}".format(file_name, SOURCE_FILE_EXT)))