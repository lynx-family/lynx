# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
from common import *
from generator.base_generator import *
from generator.android.android_base import *

__all__ = ["SubErrorCodeFileGenerator"]

CODE_NAME_PREFIX = "E_"

IMPORTS = '''import java.util.List;
import androidx.annotation.RestrictTo;\n\n'''

# common function for sub error generators
def _get_sub_code_name(code, behavior, section):
    raw_name = section[KEY_NAME] + behavior[KEY_NAME] + code[KEY_NAME]
    raw_name = raw_name.replace(VALUE_DEFAULT, "")
    return CODE_NAME_PREFIX + pascal_to_upper_snake(raw_name)

def _to_real_type(data_type, name, is_multi_selection=False):
    res_type = ""
    if is_multi_selection:
        res_type = "List<"
    if data_type == TYPE_STR:
        res_type += "String"
    elif data_type == TYPE_ENUM:
        res_type += name
    elif data_type == TYPE_BOOL:
        res_type += "boolean"
    else:
        # number type
        res_type +=data_type
    if is_multi_selection:
        res_type += ">"
    return res_type

class SubErrorCodeFileGenerator(FileGenerator):
    def __init__(self, relative_path, file_name, meta_data_list):
        super().__init__(relative_path, file_name)
        # registration must be done in the order of product codes
        self._register_child_generator(SubErrorCodeDefGenerator("\t"))
        self._register_child_generator(MetaDataEnumGenerator("\t"))
        self._register_child_generator(MetaDataClassGenerator("\t"))
        self._register_child_generator(
            MetaDataFactoryGenerator("\t", meta_data_list))

    def _generate_file_header(self):
        self._append(PACKAGE_DECL)
        self._append(IMPORTS)
        self._append("@RestrictTo(RestrictTo.Scope.LIBRARY)\n")
        self._append("public class {0} {{\n".format(SUB_ERR_CLASS_NAME))

    def _generate_file_footer(self):
        self._append("}\n\n")

class SubErrorCodeDefGenerator(SubCodeGenerator):
    def on_next_sub_code(self, code, behavior, section):
        super().on_next_sub_code(code, behavior, section)
        code_name = _get_sub_code_name(code, behavior, section)
        sub_code = self.get_sub_code(code, behavior, section)
        # generate sub code definition
        self._append_with_indent(
            "public static final int {0} = {1};\n".format(
                code_name, sub_code))

class MetaDataEnumGenerator(MetaDataEnumGenerator):
    def on_next_meta_data(self, meta_data, is_last_element=False):
        if meta_data.get(KEY_TYPE) != TYPE_ENUM:
            return
        self._append_with_indent("enum {0} {{\n".format(meta_data[KEY_NAME]))
        values = meta_data["values"]
        for value in values:
            self._append_with_indent(
                "\t{0}(\"{1}\")".format(str_to_upper_case(value), value))
            if value != values[-1]:
                self._append(",\n")
            else:
                self._append(";\n\n")
        self._append_with_indent("\tpublic final String value;\n\n")
        self._append_with_indent(
            "\t{0}(String value) {{\n".format(meta_data[KEY_NAME]))
        self._append_with_indent("\t\tthis.value = value;\n")
        self._append_with_indent("\t}\n")
        self._append_with_indent("}\n\n")

class MetaDataClassGenerator(ModuleGenerator):
    def __init__(self, base_indent = ""):
        super().__init__(base_indent)
        self._var_decl = []
        self._ctor_params = []
        self._ctor_body = []

    def before_generate(self):
        self._append_with_indent(
            "static class {0} {{\n".format(META_DATA_CLASS_NAME))
        self._ctor_params.append(
            "\t{0}{1}(".format(self._base_indent, META_DATA_CLASS_NAME))

    def after_generate(self):
        self._ctor_params.append(") {\n")
        self._var_decl.append("\n")
        self._ctor_body.append("\t{0}}}\n".format(self._base_indent))
        self._code.append("".join(self._var_decl))
        self._code.append("".join(self._ctor_params))
        self._code.append("".join(self._ctor_body))
        self._code.append("{0}}}\n\n".format(self._base_indent))
    
    def on_next_meta_data(self, meta_data, is_last_element=False):
        name = meta_data[KEY_NAME]
        is_multi_selection = True if meta_data.get("multi-selection") == True else False
        real_type = _to_real_type(meta_data[KEY_TYPE], name, is_multi_selection)
        param_name = pascal_to_camel_case(name)
        declaration_indent = self._base_indent + "\t"
        ctor_body_indent = self._base_indent + "\t\t" 
        self._var_decl.append(
            "{0}public final {1} m{2};\n".format(declaration_indent, real_type, name))
        self._ctor_params.append("{0} {1}".format(real_type, param_name))
        self._ctor_body.append(
            "{0}this.m{1} = {2};\n".format(ctor_body_indent, name, param_name))
        if not is_last_element:
            self._ctor_params.append(", ")
            

class MetaDataFactoryGenerator(ModuleGenerator):
    def __init__(self, base_indent, meta_data_list):
        super().__init__(base_indent)
        self._meta_data_list = meta_data_list

    def before_generate(self):
        self._append_with_indent(
            "protected static {0} get{0}(int code) {{\n".format(
                META_DATA_CLASS_NAME))
        self._append_with_indent("\tswitch (code) {\n")

    def after_generate(self):
        self._append_with_indent("\t\tdefault:\n")
        self._append_with_indent("\t\t\treturn null;\n")
        self._append_with_indent("\t}\n")
        self._append_with_indent("}\n")

    def on_next_sub_code(self, code, behavior, section):
        self._append_with_indent(
            "\t\tcase {0}:\n".format(_get_sub_code_name(code, behavior, section)))
        self._append_with_indent(
            "\t\t\treturn new {0}(".format(META_DATA_CLASS_NAME))
        for meta_data in self._meta_data_list:
            if meta_data != self._meta_data_list[0]:
                self._append(", ")
            self._gen_meta_data_value(meta_data, code, behavior)   
        self._append(");\n")

    def _gen_meta_data_value(self, meta_data, code, behavior):
        data_keyword = meta_data[KEY_KEYWORD]
        data_value = meta_data_value_for_sub_code(
                            meta_data, code, behavior)
        self._append(self._value_to_immediate_param(meta_data, data_value))

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
            res = "{0}.{1}".format(
                meta_data[KEY_NAME], str_to_upper_case(data_value))
        return res

    def _value_list_code_for_enum(self, meta_data, value_list):
        res = []
        res.append("List.of(")
        for v in value_list:
            res.append("{0}.{1}".format(
                meta_data[KEY_NAME], str_to_upper_case(v)))
            if v != value_list[-1]:
                res.append(", ")
        res.append(")")
        return "".join(res)