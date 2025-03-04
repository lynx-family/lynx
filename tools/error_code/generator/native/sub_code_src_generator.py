# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
import os
from common import *
from generator.base_generator import *
from generator.native.native_common import *

__all__ = ["NativeSubCodeSrcGenerator"]

class NativeSubCodeSrcGenerator(FileGenerator):
    def __init__(self, relative_path, file_name):
        super().__init__(relative_path, file_name)
        self._register_child_generator(
            NativeSubCodeDefGenerator("\t"))

    def _generate_file_header(self):
        header_path = get_include_file_path(
            self._relative_path, self._file_name)
        self._append(
            "#include \"{0}\"\n\n".format(header_path))
        for ns in NAMESPACES:
            self._append("namespace {0} {{\n".format(ns))

    def _generate_file_footer(self):
        for ns in NAMESPACES:
            self._append("}\n")
        self._append("\n")


class NativeSubCodeDefGenerator(SubCodeGenerator):
    def on_next_sub_code(self, code, behavior, section):
        super().on_next_sub_code(code, behavior, section)
        code_name = get_sub_code_name(code, behavior, section)
        code_num = self.get_sub_code(code, behavior, section)
        self._append_with_indent(
            "const int32_t {0} = {1};\n".format(code_name, code_num))