# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
import os
from common import *
from generator.base_generator import *

HEADER_FILE_EXT = ".h"
SOURCE_FILE_EXT = ".cc"

CODE_NAME_PREFIX = "E_"

NAMESPACES = ["lynx", "error"]

def get_include_guard(relative_path, file_name):
    r_file_path = relative_path + file_name
    guard = r_file_path.replace("/", "_").replace(".", "_")
    guard = pascal_to_upper_snake(guard) + "_"
    return guard

def get_include_file_path(relative_path, file_name):
    file_name_prefix, _ = os.path.splitext(file_name)
    return relative_path + file_name_prefix + HEADER_FILE_EXT

def get_sub_code_name(code, behavior, section):
    raw_name = section[KEY_NAME] + behavior[KEY_NAME] + code[KEY_NAME]
    raw_name = raw_name.replace(VALUE_DEFAULT, "")
    return CODE_NAME_PREFIX + pascal_to_upper_snake(raw_name)

def get_field_name(meta_data):
    return to_lower_snake(meta_data[KEY_NAME]) + "_"

def convert_meta_data_type(meta_data):
    data_type = meta_data[KEY_TYPE]
    data_name = meta_data[KEY_NAME]

    if data_type == TYPE_STR:
        real_type = "std::string"
    elif data_type == TYPE_BOOL:
        real_type = "bool"
    elif data_type == TYPE_ENUM:
        real_type = data_name
    else:
        real_type = data_type

    if meta_data.get("multi-selection", False):
        return "std::vector<{0}>".format(real_type)
    else:
        return real_type
