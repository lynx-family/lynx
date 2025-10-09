# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
import os
from common import *
from generator.base_generator import *
from generator.native.native_common import *

__all__ = ["NativeSubCodeSrcGenerator"]

class NativeSubCodeSrcGenerator(FileGenerator):
    def __init__(self, relative_path, file_name, meta_data_list=None):
        super().__init__(relative_path, file_name)
        self._meta_data_list = meta_data_list or []
        self._register_child_generator(
            NativeSubCodeDefGenerator("\t"))
        if self._meta_data_list:
            self._register_child_generator(
                NativeMetaDataImplGenerator("\t", meta_data_list))

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

class NativeMetaDataImplGenerator(ModuleGenerator):
    def __init__(self, base_indent, meta_data_list):
        super().__init__(base_indent)
        self._meta_data_list = meta_data_list

    def before_generate(self):
        # Generate MetaData constructor
        self._append_with_indent("{0}::{0}(".format(META_DATA_CLASS_NAME))
        params = []
        for meta_data in self._meta_data_list:
            data_type = convert_meta_data_type(meta_data)
            param_name = to_lower_snake(meta_data[KEY_NAME])
            params.append("{0} {1}".format(data_type, param_name))
        self._append(", ".join(params))
        self._append(")\n")
        
        # Generate member initializer list
        init_list = []
        for meta_data in self._meta_data_list:
            field_name = get_field_name(meta_data)
            param_name = to_lower_snake(meta_data[KEY_NAME])
            init_list.append("{0}({1})".format(field_name, param_name))
        
        self._append_with_indent("\t: {0} {{\n".format(", ".join(init_list)))
        self._append_with_indent("}\n\n")
        
        # Generate GetMetaData function
        self._append_with_indent("std::shared_ptr<{0}> GetMetaData(int32_t code) {{\n".format(META_DATA_CLASS_NAME))
        self._append_with_indent("\tswitch (code) {\n")

    def after_generate(self):
        self._append_with_indent("\t\tdefault:\n")
        self._append_with_indent("\t\t\treturn nullptr;\n")
        self._append_with_indent("\t}\n")
        self._append_with_indent("}\n\n")

    def on_next_sub_code(self, code, behavior, section):
        code_name = get_sub_code_name(code, behavior, section)
        self._append_with_indent("\t\tcase {0}:\n".format(code_name))
        self._append_with_indent("\t\t\treturn std::make_shared<{0}>({0}(\n".format(META_DATA_CLASS_NAME))
        
        # Generate constructor parameters
        params = []
        for meta_data in self._meta_data_list:
            value = self._get_meta_data_value(meta_data, code, behavior)
            params.append("\t\t\t\t\t{0}".format(value))
        
        self._append(",\n".join(params))
        self._append("\n\t\t\t\t));\n")

    def _get_meta_data_value(self, meta_data, code, behavior):
        data_value = meta_data_value_for_sub_code(meta_data, code, behavior)
        data_type = meta_data[KEY_TYPE]
        
        if data_type == TYPE_STR:
            return '"{0}"'.format(data_value)
        elif data_type == TYPE_BOOL:
            return "true" if data_value else "false"
        elif data_type != TYPE_ENUM:
            return str(data_value)
        elif meta_data.get("multi-selection", False):
            values = []
            for v in data_value:
                values.append("{0}::{1}".format(meta_data[KEY_NAME], str_to_upper_case(v)))
            return "{{{0}}}".format(", ".join(values))
        else:
            return "{0}::{1}".format(meta_data[KEY_NAME], str_to_upper_case(data_value))
