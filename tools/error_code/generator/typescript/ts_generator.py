# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
from common import *
from generator.base_generator import *

__all__ = ["TypescriptGenerator"]

FILE_EXT = ".ts"
CODE_NAME_PREFIX = "E_"

BASE_RELATIVE_PATH = "js_libraries/lynx-core/src/common/"

def get_sub_code_name(code, behavior, section):
    raw_name = section[KEY_NAME] + behavior[KEY_NAME] + code[KEY_NAME]
    raw_name = raw_name.replace(VALUE_DEFAULT, "")
    return CODE_NAME_PREFIX + pascal_to_upper_snake(raw_name)

class TypescriptGenerator(PlatformGenerator):
    def __init__(self):
        super().__init__()
        self._register_child_generator(
            TSSubCodeFileGenerator(
                BASE_RELATIVE_PATH,
                "subErrorCode{0}".format(FILE_EXT)))


class TSSubCodeFileGenerator(FileGenerator):
    def __init__(self, relative_path, file_name):
        super().__init__(relative_path, file_name)
        self._format_off = "\n"
        self._format_on = "\n"
        self._register_child_generator(TSSubCodeDefGenerator())


class TSSubCodeDefGenerator(SubCodeGenerator):
    def on_next_sub_code(self, code, behavior, section):
        super().on_next_sub_code(code, behavior, section)
        code_name = get_sub_code_name(code, behavior, section)
        code_num = self.get_sub_code(code, behavior, section)
        self._append_with_indent(
            "export const {0} = {1};\n".format(code_name, code_num))