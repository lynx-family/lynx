# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
import os
from common import *
from generator.base_generator import *
from generator.darwin.darwin_base import *

__all__ = ["SubErrorCodeSrcFileGenerator"]

class SubErrorCodeSrcFileGenerator(FileGenerator):
    def __init__(self, relative_path, file_name, meta_data_list):
        super().__init__(relative_path, file_name)
        # registration must be done in the order of product codes
        self._child_generators.append(SubErrCodeDefGenerator())
        self._child_generators.append(MetaDataClassDefGenerator())
        self._child_generators.append(MetaDataFactoryDefGenerator("", meta_data_list))

    def _generate_file_header(self):
        self._append(
            "#import \"{0}{1}\"\n\n".format(SUB_ERR_CLASS_NAME, HEADER_FILE_EXT))

    def _generate_file_footer(self):
        self._append("\n")

class SubErrCodeDefGenerator(DarwinSubCodeGenerator):
    def on_next_sub_code(self, code, behavior, section):
        super().on_next_sub_code(code, behavior, section)
        code_name = get_sub_code_name(code, behavior, section)
        self._append_with_indent(
            "NSInteger const {0} = {1};\n".format(
                code_name, self.get_sub_code(code, behavior, section)))

    def after_generate(self):
        super().after_generate()
        self._append("\n")

class MetaDataClassDefGenerator(ModuleGenerator):
    def __init__(self, base_indent = ""):
        super().__init__(base_indent)
        self._ctor_header = []
        self._ctor_body = []

    def before_generate(self):
        self._append_with_indent(
            "@implementation {0}{1}\n\n".format(
                SUB_ERR_CLASS_NAME, META_DATA_CLASS_NAME))
        self._append_with_indent(
            "- (instancetype)initWith")
        self._ctor_body.append("{0}\tself = [super init];\n".format(self._base_indent))
        self._ctor_body.append("{0}\tif (self) {{\n".format(self._base_indent))

    def on_next_meta_data(self, meta_data, is_last_element=False):
        data_name = meta_data[KEY_NAME]
        data_type = meta_data[KEY_TYPE]
        is_multi_selection = False if meta_data.get("multi-selection") == None else True
        real_type = get_real_type(data_type, data_name, is_multi_selection)
        camel_case_name = to_camel_case(data_name)
        if len(self._ctor_header) == 0:
            self._ctor_header.append(to_pascal_case(data_name))
        else:
            self._ctor_header.append(camel_case_name)
        self._ctor_header.append(":({0}){1} ".format(real_type, camel_case_name))
        self._ctor_body.append(
            "{0}\t\t_{1} = {1};\n".format(self._base_indent, camel_case_name))

    def after_generate(self):
        self._ctor_header.append("{\n")
        self._ctor_body.append("{0}\t}}\n".format(self._base_indent))
        self._ctor_body.append("{0}\treturn self;\n".format(self._base_indent))
        self._append("".join(self._ctor_header))
        self._append("".join(self._ctor_body))
        self._append_with_indent("}\n\n")
        self._append("@end\n\n")

class MetaDataFactoryDefGenerator(ModuleGenerator):
    def __init__(self, base_indent, meta_data_list):
        super().__init__(base_indent)
        self._meta_data_list = meta_data_list
        self._meta_data_factory_func = []
        self._enum_to_str_func = []
        self._meta_data_class_name = "{0}{1}".format(
            SUB_ERR_CLASS_NAME, META_DATA_CLASS_NAME)

    def before_generate(self):
        self._append_with_indent(
            "@implementation {0}Utils\n\n".format(SUB_ERR_CLASS_NAME))
        self._append_with_indent(
            "+ ({0}{1}*)get{1}:(NSInteger)subCode {{\n".format(
                SUB_ERR_CLASS_NAME, META_DATA_CLASS_NAME))
        self._append_with_indent("\tswitch (subCode) {\n")

    def after_generate(self):
        # append meta data factory function
        self._append("".join(self._meta_data_factory_func))
        self._append_with_indent("\t\tdefault:\n")
        self._append_with_indent("\t\t\treturn nil;\n")
        self._append_with_indent("\t};\n")
        self._append_with_indent("}\n\n")
        # append enum to string function
        self._append("".join(self._enum_to_str_func))
        self._append_with_indent("@end\n\n")

    def on_next_sub_code(self, code, behavior, section):
        self._meta_data_factory_func.append(
            "{0}\t\tcase {1}:\n".format(
                self._base_indent, get_sub_code_name(code, behavior, section)))
        self._meta_data_factory_func.append(
            "{0}\t\t\treturn [[{1} alloc] initWith".format(
                self._base_indent, self._meta_data_class_name))
        for data in self._meta_data_list:
            data_name = data[KEY_NAME]
            data_name_in_func = ""
            if data == self._meta_data_list[0]:
                data_name_in_func = to_pascal_case(data_name)
            else:
                data_name_in_func = to_camel_case(data_name)
            self._meta_data_factory_func.append("{0}:".format(data_name_in_func))
            data_value = meta_data_value_for_sub_code(data, code, behavior)
            self._meta_data_factory_func.append(
                data_value_to_immediate_param(data, data_value))
            if data != self._meta_data_list[-1]:
                self._meta_data_factory_func.append(" ")
        self._meta_data_factory_func.append("];\n")

    def on_next_meta_data(self, meta_data, is_last_element=False):
        if meta_data.get(KEY_TYPE) != TYPE_ENUM:
            return
        data_name = meta_data[KEY_NAME]
        camel_case_data_name = to_camel_case(data_name)
        enum_class_name = get_enum_class_name(data_name)
        value_list = meta_data[KEY_VALUES]
        self._enum_to_str_func.append("{0}+ (NSString*){1}ToStr:".format(
            self._base_indent, camel_case_data_name))
        self._enum_to_str_func.append("({0}){1} {{\n".format(
            enum_class_name, camel_case_data_name))
        self._enum_to_str_func.append("{0}\tswitch({1}) {{\n".format(
            self._base_indent, camel_case_data_name))
        for v in value_list:
            self._enum_to_str_func.append("{0}\t\tcase {1}{2}:\n".format(
                self._base_indent, enum_class_name, to_pascal_case(v)))
            self._enum_to_str_func.append("{0}\t\t\treturn @\"{1}\";\n".format(
                self._base_indent, v))
        self._enum_to_str_func.append("{0}\t\tdefault:\n".format(self._base_indent))
        self._enum_to_str_func.append(
            "{0}\t\t\treturn @\"unknown_{1}\";\n".format(self._base_indent, camel_case_data_name))
        self._enum_to_str_func.append("{0}\t}}\n".format(self._base_indent))
        self._enum_to_str_func.append("{0}}}\n\n".format(self._base_indent))

            
        