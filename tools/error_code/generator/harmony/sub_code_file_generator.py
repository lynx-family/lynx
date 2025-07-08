# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
from common import *
from generator.base_generator import *
from generator.harmony.harmony_common import *

__all__ = ["SubErrCodeFileGenerator"]

class SubErrCodeFileGenerator(FileGenerator):
    def __init__(self, relative_path, file_name, meta_data_list):
        super().__init__(relative_path, file_name)
        self._format_off = "\n"
        self._format_on = "\n"
        # registration must be done in the order of product codes
        self._register_child_generator(SubErrCodeDefGenerator("\t"))
        self._register_child_generator(MetaDataEnumGenerator())
        self._register_child_generator(MetaDataClassGenerator())
        self._register_child_generator(MetaDataFactoryGenerator("\t", meta_data_list))

    def _generate_file_header(self):
        self._append("\n")


class SubErrCodeDefGenerator(SubCodeGenerator):
    def before_generate(self):
        super().before_generate()
        self._append(
            "export class {0} {{\n".format(SUB_ERR_CLASS_NAME))

    def after_generate(self):
        super().after_generate()
        self._append("}\n\n")

    def on_next_sub_code(self, code, behavior, section):
        super().on_next_sub_code(code, behavior, section)
        code_name = get_sub_code_name(code, behavior, section)
        sub_code = self.get_sub_code(code, behavior, section)
        # generate sub code definition
        self._append_with_indent(
            "public static readonly {0}: number = {1};\n".format(
                code_name, sub_code))

class MetaDataFactoryGenerator(ModuleGenerator):
    def __init__(self, base_indent, meta_data_list):
        super().__init__(base_indent)
        self._meta_data_list = meta_data_list

    def before_generate(self):
        self._append_with_indent("public static ")
        self._append(
            "get{0}(subCode: number): {1}{0} | null {{\n".format(
                META_DATA_CLASS_NAME, ENUM_PREFIX))
        self._append_with_indent("\tswitch (subCode) {\n")

    def after_generate(self):
        self._append_with_indent("\t\tdefault:\n")
        self._append_with_indent("\t\t\treturn null;\n")
        self._append_with_indent("\t}\n")
        self._append_with_indent("}\n")
        self._append("}\n")


    def on_next_sub_code(self, code, behavior, section):
        code_name = get_sub_code_name(code, behavior, section)
        self._append_with_indent(
            "\t\tcase {0}.{1}:\n".format(SUB_ERR_CLASS_NAME, code_name))
        self._append_with_indent(
            "\t\t\treturn new {0}{1}(".format(
                ENUM_PREFIX, META_DATA_CLASS_NAME))
        for meta_data in self._meta_data_list:
            if meta_data != self._meta_data_list[0]:
                self._append(", ")
            self._gen_meta_data_value(meta_data, code, behavior)   
        self._append(");\n")

    def _gen_meta_data_value(self, meta_data, code, behavior):
        data_keyword = meta_data[KEY_KEYWORD]
        data_value = meta_data_value_for_sub_code(
                            meta_data, code, behavior)
        immediate_p = self._value_to_immediate_param(meta_data, data_value)
        self._append(immediate_p)

    def _value_to_immediate_param(self, meta_data, data_value):
        res = ""
        data_type = meta_data[KEY_TYPE]
        is_multi_selection = True if meta_data.get("multi-selection") == True else False
        if data_type == TYPE_STR:
            res = "\"{0}\"".format(data_value)
        elif data_type == TYPE_BOOL:
            res = "true" if data_value else "false"
        elif data_type != TYPE_ENUM:
            res = data_value
        elif is_multi_selection:
            res = self._value_list_code_for_enum(meta_data, data_value)
        else:
            enum_clz_name = get_enum_class_name(meta_data[KEY_NAME])
            res = "{0}.{1}".format(
                enum_clz_name, to_pascal_case(data_value))
        return res

    def _value_list_code_for_enum(self, meta_data, value_list):
        res = []
        enum_clz_name = get_enum_class_name(meta_data[KEY_NAME])
        res.append("[")
        for v in value_list:
            res.append("{0}.{1}".format(
                enum_clz_name, to_pascal_case(v)))
            if v != value_list[-1]:
                res.append(", ")
        res.append("]")
        return "".join(res)


class MetaDataEnumGenerator(MetaDataEnumGenerator):
    def on_next_meta_data(self, meta_data,is_last_element=False):
        if meta_data.get(KEY_TYPE)!= TYPE_ENUM:
            return
        enum_clz_name = get_enum_class_name(meta_data[KEY_NAME])
        self._append_with_indent(
            "export enum {0} {{\n".format(enum_clz_name))
        values = meta_data["values"]
        for value in values:
            self._append_with_indent(
                "\t{0} = \"{1}\"".format(to_pascal_case(value), value))
            if value != values[-1]:
                self._append(",\n")
        self._append_with_indent("\n}\n\n")


class MetaDataClassGenerator(ModuleGenerator):
    def __init__(self, base_indent = ""):
        super().__init__(base_indent)
        self._var_decl = []
        self._ctor_params = []
        self._ctor_body = []

    def before_generate(self):
        self._append_with_indent(
            "export class {0}{1} {{\n".format(ENUM_PREFIX, META_DATA_CLASS_NAME))
        self._ctor_params.append("\tconstructor(")

    def after_generate(self):
        self._var_decl.append("\n")
        self._ctor_params.append(") {\n")
        self._ctor_body.append("\t{0}}}\n".format(self._base_indent))
        self._append("".join(self._var_decl))
        self._append("".join(self._ctor_params))
        self._append("".join(self._ctor_body))
        self._append("\n")

    def on_next_meta_data(self, meta_data, is_last_element=False):
        name = meta_data[KEY_NAME]
        is_multi_selection = True if meta_data.get("multi-selection") == True else False
        real_type = get_real_type(meta_data[KEY_TYPE], name, is_multi_selection)
        param_name = pascal_to_camel_case(name)
        v_decl_indent = self._base_indent + "\t"
        ctor_body_indent = self._base_indent + "\t\t"
        self._var_decl.append(
            "{0}public readonly {1}: {2};\n".format(
                v_decl_indent, param_name, real_type))
        self._ctor_params.append(
            "{0}: {1}".format(param_name, real_type))
        self._ctor_body.append(
            "{0}this.{1} = {1};\n".format(ctor_body_indent, param_name))
        if not is_last_element:
            self._ctor_params.append(", ")
        

    