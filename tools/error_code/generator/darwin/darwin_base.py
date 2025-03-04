# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
from common import *
from generator.base_generator import *

HEADER_FILE_EXT = ".h"
SOURCE_FILE_EXT = ".m"

CODE_NAME_PREFIX = "ECLynx"
BEHAVIOR_NAME_PREFIX = "EBLynx"

ENUM_PREFIX = "ELynx"

# common function for android generators
def get_enum_class_name(name):
    return ENUM_PREFIX + to_pascal_case(name)

def get_behavior_code_name(behavior, section):
    section_name = to_pascal_case(section[KEY_NAME]).replace(VALUE_DEFAULT, "")
    behavior_name = to_pascal_case(behavior[KEY_NAME]).replace(VALUE_DEFAULT, "")
    name = "{0}{1}{2}".format(
            BEHAVIOR_NAME_PREFIX, section_name, behavior_name)
    return name

def get_real_type(data_type, data_name, is_multi_selection=False):
    res_type = ""
    if is_multi_selection:
        res_type = "NSArray*"
    elif data_type == TYPE_STR:
        res_type = "NSString*"
    elif data_type == TYPE_ENUM:
        res_type = get_enum_class_name(data_name)
    elif data_type == TYPE_BOOL:
        res_type = "BOOL"
    else:
        # number type
        res_type = data_type
    return res_type

def get_sub_code_name(code, behavior, section):
    raw_name = section[KEY_NAME] + behavior[KEY_NAME] + code[KEY_NAME]
    raw_name = raw_name.replace(VALUE_DEFAULT, "")
    return CODE_NAME_PREFIX + to_pascal_case(raw_name)

def data_value_to_immediate_param(meta_data, data_value):
    res = ""
    data_type = meta_data[KEY_TYPE]
    is_multi_selection = True if meta_data.get("multi-selection") == True else False
    if data_type == TYPE_STR:
        res = "@\"{0}\"".format(data_value)
    elif data_type == TYPE_BOOL:
        res = "YES" if data_value else "NO"
    elif data_type != TYPE_ENUM:
        res = data_value
    elif is_multi_selection:
        res = _get_value_list_code_for_enum(meta_data, data_value)
    else:
        res = "{0}{1}".format(
            get_enum_class_name(meta_data[KEY_NAME]), to_pascal_case(data_value))
    return res

def _get_value_list_code_for_enum(meta_data, value_list):
    res = []
    enum_name = get_enum_class_name(meta_data[KEY_NAME])
    res.append("@[")
    for v in value_list:
        res.append("@({0}{1})".format(enum_name, to_pascal_case(v)))
        if v != value_list[-1]:
            res.append(", ")
    res.append("]")
    return "".join(res)

        

class DarwinSubCodeGenerator(SubCodeGenerator):
    def before_gen_section(self, section):
        self._append("#pragma mark - Section: {0}".format(section[KEY_NAME]))
        super().before_gen_section(section)

class DarwinBehaviorGenerator(BehaviorGenerator):
    def before_gen_section(self, section):
        self._append("#pragma mark - Section: {0}".format(section[KEY_NAME]))
        super().before_gen_section(section)