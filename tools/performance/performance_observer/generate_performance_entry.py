#!/#!/usr/bin/env python3
# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
import os
from sub_generator.ts_generator import generate_ts
from sub_generator.java_generator import generate_java, generate_java_converter
from sub_generator.oc_generator import generate_objc_interface, generate_objc_implementation, generate_objc_converter, generate_objc_converter_header
from utils import *

base_dir = os.path.dirname(os.path.abspath(__file__))

# common variable
ts_output_file_path = os.path.abspath(os.path.join(base_dir, '../../../js_libraries/types/types/background-thread'))
java_output_file_path = os.path.abspath(os.path.join(base_dir, '../../../platform/android/lynx_android/src/main/java/com/lynx/tasm/performance/performanceobserver'))
objc_header_output_file_path = os.path.abspath(os.path.join(base_dir, '../../../platform/darwin/common/lynx/public/performance/performance_observer'))
objc_impl_output_file_path = os.path.abspath(os.path.join(base_dir, '../../../platform/darwin/common/lynx/performance/performance_observer'))
converter_name = 'PerformanceEntryConverter'
entry_mapping = {}

def prepare_before_generate(yaml_files):
    for yaml_file in yaml_files:
        items = read_yaml_file(os.path.join(base_dir, yaml_file))
        for class_name, definition in list(items.items()):
            # Prepare the required data to generate PerformanceConverter
            # x-type refer to 'PerformanceEntry.entryType', it's unique.
            # x-name refer to 'PerformanceEntry.name'.
            if 'x-type' in definition:
                xtype = definition['x-type']
                if 'x-name' in definition:
                    xname = definition['x-name']
                    if (isinstance(xname, str)):
                        key = xtype + '.' + xname
                        entry_mapping[key] = class_name
                    elif (isinstance(xname, list)):
                        for name in xname:
                            key = xtype + '.' + name
                            entry_mapping[key] = class_name

def generate_java_code(yaml_files):
    remove_exist_dirs(java_output_file_path)
    # Generate Definition files
    for yaml_file in yaml_files:
        items = read_yaml_file(os.path.join(base_dir, yaml_file))

        java_codes = []
        java_imports = []
        for class_name, definition in list(items.items()):
            # Handle special characters to generate ignore rules.
            enableJava = True
            if ('x-lang' in definition):
                enableJava = 'java' in definition['x-lang']
            
            if enableJava:
                if class_name.startswith("Android"):
                    class_name = class_name[7:]

                # Generate java code
                java_code = generate_java(class_name, definition, java_imports)
                if java_code:
                    java_codes.append(java_code)
                # Write Java file
                java_output = get_license(2024)
                if(not java_imports) and (java_codes != []):
                    java_output += 'package com.lynx.tasm.performance.performanceobserver;\n\n' + '\n'.join(java_codes)
                    write_file(os.path.join(java_output_file_path, f'{class_name}.java'), java_output)
                elif (java_codes != []):
                    java_output += 'package com.lynx.tasm.performance.performanceobserver;\n\n' + '\n'.join(java_imports) + '\n\n' + '\n'.join(java_codes)
                    write_file(os.path.join(java_output_file_path, f'{class_name}.java'), java_output)
    # Generate PerformanceEntryConverter
    java_converter_imports = []
    java_converter_code = generate_java_converter(entry_mapping, java_converter_imports)
    java_output = get_license(2024)
    java_output += '\n'.join(java_converter_imports) + '\n\n' + java_converter_code
    write_file(os.path.join(java_output_file_path, f'{converter_name}.java'), java_output)

def generate_objc_code(yaml_files):
    remove_exist_dirs(objc_header_output_file_path)
    remove_exist_dirs(objc_impl_output_file_path)
    # Generate Definition files
    for yaml_file in yaml_files:
        items = read_yaml_file(os.path.join(base_dir, yaml_file))

        objc_headers = []
        objc_imports = []
        objc_implementations = []
        objc_implementations_imports = []
        for class_name, definition in list(items.items()):
            # Handle special characters to generate ignore rules.
            enableObjc= True
            if ('x-lang' in definition):
                enableObjc = 'objc' in definition['x-lang']

            if enableObjc:
                if class_name.startswith("IOS"):
                    class_name = class_name[3:]
    
                # Generate objc header code
                objc_header = generate_objc_interface(class_name, definition, objc_imports)
                if objc_header:
                    objc_headers.append(objc_header)
                # Generate objc implementation code
                objc_implementation = generate_objc_implementation(class_name, definition, objc_implementations_imports)
                if objc_implementation:
                    objc_implementations.append(objc_implementation)
                # Write header file
                objc_output = get_license(2024)
                if (not objc_imports) and (objc_headers != []):
                    objc_output += '\n'.join(objc_headers)
                    write_file(os.path.join(objc_header_output_file_path, f'{objc_lynx_prefix}{class_name}.h'), objc_output)
                elif (objc_headers != []):
                    objc_output += '\n'.join(objc_imports) + '\n\n' + '\n'.join(objc_headers)
                    write_file(os.path.join(objc_header_output_file_path, f'{objc_lynx_prefix}{class_name}.h'), objc_output)
                # Write impl file
                objc_implementation_output = get_license(2024)
                if (objc_implementations != []):
                    objc_implementation_output += '\n'.join(objc_implementations_imports) + '\n\n' + '\n'.join(objc_implementations)
                    write_file(os.path.join(objc_impl_output_file_path, f'{objc_lynx_prefix}{class_name}.m'), objc_implementation_output)
    # Generate PerformanceConverter
    objc_converter_header = get_license(2024)
    objc_converter_header += generate_objc_converter_header()
    write_file(os.path.join(objc_header_output_file_path, f'{objc_lynx_prefix}{converter_name}.h'), objc_converter_header)
    objc_converter_imports = []
    objc_converter_code = generate_objc_converter(entry_mapping, objc_converter_imports)
    objc_output = get_license(2024)
    objc_output += '\n'.join(objc_converter_imports) + '\n\n' + objc_converter_code
    write_file(os.path.join(objc_impl_output_file_path, f'{objc_lynx_prefix}{converter_name}.m'), objc_output)

def generate_ts_code(yaml_files):
    ts_interfaces = []

    for yaml_file in yaml_files:
        items = read_yaml_file(os.path.join(base_dir, yaml_file))

        for class_name, definition in list(items.items()):
            enableTs= True
            if ('x-lang' in definition):
                enableTs = 'ts' in definition['x-lang']
            
            if enableTs:
                ts_interface = generate_ts(class_name, definition, items)
                ts_interfaces.append(ts_interface)

    # Generate ts Interface
    ts_output = get_license(2024)
    ts_output += '\n'.join(ts_interfaces)
    ts_file_name = 'lynx-performance-entry.d.ts'
    write_file(os.path.join(ts_output_file_path, ts_file_name), ts_output)

def main(yaml_files_list_file):
    with open(yaml_files_list_file, "r") as file:
        yaml_files = [line.strip() for line in file if line.strip()]

    prepare_before_generate(yaml_files)
    generate_ts_code(yaml_files)
    generate_java_code(yaml_files)
    generate_objc_code(yaml_files)

if __name__ == "__main__":
    main(os.path.join(base_dir, "performance_entry_definition_files"))
