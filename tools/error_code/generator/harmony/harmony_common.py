# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
from common import *

FILE_EXT = ".ets"

CODE_NAME_PREFIX = "E_"
BEHAVIOR_NAME_PREFIX = "EB_"

ENUM_PREFIX = "LynxError"

def get_real_type(data_type, data_name, is_multi_selection=False):
    res_type = ""
    if is_multi_selection:
        res_type = "Array<"
    if data_type == TYPE_STR:
        res_type += "string"
    elif data_type == TYPE_ENUM:
        res_type += get_enum_class_name(data_name)
    elif data_type == TYPE_BOOL:
        res_type += "bool"
    else:
        # number type 
        res_type += data_type
    if is_multi_selection:
        res_type += ">"
    return res_type

def get_enum_class_name(name):
    return ENUM_PREFIX + to_pascal_case(name)

def get_behavior_code_name(behavior, section):
    behavior_name = BEHAVIOR_NAME_PREFIX
    raw_name = section[KEY_NAME] + behavior[KEY_NAME]
    raw_name = raw_name.replace(VALUE_DEFAULT, "")
    behavior_name += pascal_to_upper_snake(raw_name)
    return behavior_name
    
def get_sub_code_name(code, behavior, section):
    raw_name = section[KEY_NAME] + behavior[KEY_NAME] + code[KEY_NAME]
    raw_name = raw_name.replace(VALUE_DEFAULT, "")
    return CODE_NAME_PREFIX + pascal_to_upper_snake(raw_name)


    
    