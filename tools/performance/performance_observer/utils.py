#!/#!/usr/bin/env python3
# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import os
import yaml
import sys

# common params
sys.path.append(os.path.dirname(__file__) + "/../../lynx/tools/performance/lynx_performance_entry_generator")
base_dir = os.path.dirname(os.path.abspath(__file__))
definition_dir = os.path.join(base_dir, 'definition_yaml_files')
objc_lynx_prefix = "Lynx"

### File Control Tools
# Reading YAML Files
def read_yaml_file(file_path):
    if not os.path.exists(file_path):
        print("The file does not exist.")
        return None
    with open(file_path, 'r') as file:
        return yaml.safe_load(file)

# Writing to a file
def write_file(file_path, content):
    directory = os.path.dirname(file_path)
    if not os.path.exists(directory):
        os.makedirs(directory)
    with open(file_path, 'w') as file:
        file.write(content)

### Code generation tools
# Resolving $ref references
def resolve_ref(ref, definitions, base_dir):
    ref_path = os.path.join(base_dir, ref.split('#')[0])
    if ref_path in definitions:
        return definitions[ref_path]
    data = read_yaml_file(ref_path)
    definitions[ref_path] = data
    return data
