# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
import os
from common import *
from generator.base_generator import *
from generator.darwin.darwin_base import *

__all__ = ["SubErrorCodeHeaderFileGenerator"]

class SubErrorCodeHeaderFileGenerator(FileGenerator):
    def __init__(self, relative_path, file_name):
        super().__init__(relative_path + "Public/", file_name)
        # registration must be done in the order of product codes
        self._register_child_generator(SubErrCodeDeclGenerator())
        self._register_child_generator(MetaDataEnumGenerator())
        self._register_child_generator(MetaDataClassDeclGenerator())
        self._register_child_generator(MetaDataFactoryDeclGenerator())

    def _generate_file_header(self):
        self._append("#import <Foundation/Foundation.h>\n\n")
        self._append("NS_ASSUME_NONNULL_BEGIN\n\n")

    def _generate_file_footer(self):
        self._append("NS_ASSUME_NONNULL_END\n")

    
class SubErrCodeDeclGenerator(DarwinSubCodeGenerator):
    def on_next_sub_code(self, code, behavior, section):
        super().on_next_sub_code(code, behavior, section)
        code_name = get_sub_code_name(code, behavior, section)
        self._append_with_indent(
            "FOUNDATION_EXPORT NSInteger const {0};\n".format(code_name)) 


class MetaDataEnumGenerator(MetaDataEnumGenerator):
    def on_next_meta_data(self, meta_data, is_last_element=False):
        if meta_data.get(KEY_TYPE) != TYPE_ENUM:
            return
        enum_name = get_enum_class_name(meta_data[KEY_NAME])
        values = meta_data["values"]
        self._append_with_indent("typedef NS_ENUM(NSInteger, ")
        self._append("{0}) {{\n".format(enum_name))
        for v in values:
            self._append_with_indent(
                "\t{0}{1}".format(enum_name, to_pascal_case(v)))
            if v == values[0]:
                self._append(" = 0")
            if v != values[-1]:
                self._append(",")
            self._append("\n")
        self._append_with_indent("};\n\n")

    
class MetaDataClassDeclGenerator(ModuleGenerator):
    def __init__(self, base_indent = ""):
        super().__init__(base_indent)
        self._var_decl = []
        self._ctor_decl = []
    
    def before_generate(self):
        self._append_with_indent("@interface {0}{1} : NSObject\n\n".format(
                SUB_ERR_CLASS_NAME, META_DATA_CLASS_NAME))
        self._ctor_decl.append(
            "{0}- (instancetype)initWith".format(self._base_indent))
        
    def on_next_meta_data(self, meta_data, is_last_element=False):
        data_name = meta_data[KEY_NAME]
        data_type = meta_data[KEY_TYPE]
        is_multi_selection = True if meta_data.get("multi-selection") == True else False
        real_type = get_real_type(data_type, data_name, is_multi_selection)
        camel_case_name = to_camel_case(data_name)
        if len(self._ctor_decl) == 1:
            self._ctor_decl.append(to_pascal_case(data_name))
        else:
            self._ctor_decl.append(camel_case_name)
        self._ctor_decl.append(":({0}){1}".format(real_type, camel_case_name))
        self._var_decl.append("{0}@property (nonatomic, readonly) {1} {2};\n".format(
            self._base_indent,
            real_type,
            camel_case_name))
        if not is_last_element:
            self._ctor_decl.append(" ")
        else: 
            self._ctor_decl.append(";\n\n")
            self._var_decl.append("\n")

    def after_generate(self):
        self._append("".join(self._var_decl))
        self._append("".join(self._ctor_decl))
        self._append_with_indent("@end\n\n")


class MetaDataFactoryDeclGenerator(ModuleGenerator):
    def before_generate(self):
        self._append_with_indent(
            "@interface {0}Utils : NSObject\n".format(SUB_ERR_CLASS_NAME))
        self._append_with_indent(
            "+ ({0}{1}*)get{1}:(NSInteger)subCode;\n".format(
                SUB_ERR_CLASS_NAME, META_DATA_CLASS_NAME))

    def on_next_meta_data(self, meta_data, is_last_element=False):
        if meta_data.get(KEY_TYPE) != TYPE_ENUM:
            return
        name = meta_data[KEY_NAME]
        camel_case_name = pascal_to_camel_case(name)
        enum_class_name = get_enum_class_name(name)
        self._append_with_indent(
            "+ (NSString*){0}ToStr:({1}){0};\n".format(
                camel_case_name, enum_class_name))    

    def after_generate(self):
        self._append_with_indent("@end\n\n")