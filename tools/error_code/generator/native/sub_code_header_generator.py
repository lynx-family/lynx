# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
import os
from common import *
from generator.base_generator import *
from generator.native.native_common import *

__all__ = ["NativeSubCodeHeaderFileGenerator"]

class NativeSubCodeHeaderFileGenerator(FileGenerator):
    def __init__(self, relative_path, file_name, meta_data_list):
        super().__init__(relative_path, file_name)
        self._header_guard = get_include_guard(relative_path, file_name)
        self._register_child_generator(NativeSubCodeDeclGenerator())
        self._register_child_generator(MetaDataDeclGenerator("\t", meta_data_list))

    def _generate_file_header(self):
        self._append("#ifndef {0}\n".format(self._header_guard))
        self._append("#define {0}\n\n".format(self._header_guard))
        self._append("#include <cstdint>\n")
        self._append("#include <memory>\n")
        self._append("#include <string>\n")
        self._append("#include <vector>\n")
        self._append("\n")
        for ns in NAMESPACES:
            self._append("namespace {0} {{\n".format(ns))
        
    def _generate_file_footer(self):
        for ns in NAMESPACES:
            self._append("}\n")
        self._append("\n#endif")
        self._append("\t// {0}\n\n".format(self._header_guard))

class NativeSubCodeDeclGenerator(SubCodeGenerator):
    def on_next_sub_code(self, code, behavior, section):
        super().on_next_sub_code(code, behavior, section)
        code_name = get_sub_code_name(code, behavior, section)
        self._append_with_indent(
            "extern const int32_t {0};\n".format(code_name))

class MetaDataDeclGenerator(ModuleGenerator):
    def __init__(self, base_indent, meta_data_list):
        super().__init__(base_indent)
        self._meta_data_list = meta_data_list

    def before_generate(self):
        # Generate enum declarations
        for meta_data in self._meta_data_list:
            if meta_data.get(KEY_TYPE) == TYPE_ENUM:
                self._append("enum class {0} {{\n".format(meta_data[KEY_NAME]))
                values = meta_data["values"]
                for i, value in enumerate(values):
                    self._append("\t{0} = {1}".format(str_to_upper_case(value), i))
                    if value != values[-1]:
                        self._append(",")
                    self._append("\n")
                self._append("};\n\n")
        
        # Generate MetaData struct
        self._append("struct {0} {{\n".format(META_DATA_CLASS_NAME))
        for meta_data in self._meta_data_list:
            data_type = convert_meta_data_type(meta_data)
            field_name = get_field_name(meta_data)
            self._append("\t{0} {1};\n".format(data_type, field_name))
        
        self._append("\t{0}(".format(META_DATA_CLASS_NAME))
        params = []
        for meta_data in self._meta_data_list:
            data_type = convert_meta_data_type(meta_data)
            param_name = to_lower_snake(meta_data[KEY_NAME])
            params.append("{0} {1}".format(data_type, param_name))
        self._append(", ".join(params))
        self._append(");\n")
        self._append("};\n\n")
        
        # Generate GetMetaData function declaration
        self._append("std::shared_ptr<{0}> GetMetaData(int32_t code);\n\n".format(META_DATA_CLASS_NAME))
    
        


        