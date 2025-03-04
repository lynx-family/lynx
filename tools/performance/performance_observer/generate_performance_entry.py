#!/#!/usr/bin/env python3
# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
import os
import sys
from sub_generator.ts_generator import generate_ts
from sub_generator.java_generator import generate_java, generate_java_converter
from sub_generator.oc_generator import generate_objc_interface, generate_objc_implementation, generate_objc_converter, generate_objc_converter_header
from utils import *

base_dir = os.path.dirname(os.path.abspath(__file__))
definition_dir = os.path.join(base_dir, 'definition_yaml_files')

def main(yaml_files_list_file):
    license = '''// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.\n'''
    ts_output_file_path = os.path.abspath(os.path.join(base_dir, '../../../js_libraries/types/types/background-thread'))
    java_output_file_path = os.path.abspath(os.path.join(base_dir, '../../../platform/android/lynx_android/src/main/java/com/lynx/tasm/performance/performanceobserver'))
    objc_header_output_file_path = os.path.abspath(os.path.join(base_dir, '../../../platform/darwin/common/lynx/public/performance/performance_observer'))
    objc_impl_output_file_path = os.path.abspath(os.path.join(base_dir, '../../../platform/darwin/common/lynx/performance/performance_observer'))

    with open(yaml_files_list_file, "r") as file:
        yaml_files = [line.strip() for line in file if line.strip()]

    ts_interfaces = []
    ts_imports = []
    entry_mapping = {}

    for yaml_file in yaml_files:
        items = read_yaml_file(os.path.join(base_dir, yaml_file))

        java_codes = []
        java_imports = []
        objc_headers = []
        objc_imports = []
        objc_implementations = []
        objc_implementations_imports = []

        for class_name, definition in list(items.items()):
            # Prepare the PerformanceConverter to generate the required data.
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

            # Handle special characters to generate ignore rules.
            tsIgnore = False
            javaIgnore = False
            ocIgnore = False
            if 'x-ignore-ts' in definition:
                tsIgnore = definition['x-ignore-ts']
            if 'x-ignore-java' in definition:
                javaIgnore = definition['x-ignore-java']
            if 'x-ignore-oc' in definition:
                ocIgnore = definition['x-ignore-oc']

            if not tsIgnore:
                ts_interface = generate_ts(class_name, definition, items, ts_imports)
                ts_interfaces.append(ts_interface)

            if not javaIgnore:
                # generate java code
                java_code = generate_java(class_name, definition, items, java_imports)
                if java_code:
                    java_codes.append(java_code)
                # write file
                java_output = license
                if(not java_imports) and (java_codes != []):
                    java_output += 'package com.lynx.tasm.performance.performanceobserver;\n\n' + '\n'.join(java_codes)
                    write_file(os.path.join(java_output_file_path, f'{class_name}.java'), java_output)
                elif (java_codes != []):
                    java_output += 'package com.lynx.tasm.performance.performanceobserver;\n\n' + '\n'.join(java_imports) + '\n\n' + '\n'.join(java_codes)
                    write_file(os.path.join(java_output_file_path, f'{class_name}.java'), java_output)

            if not ocIgnore:
                # generate objc header code
                objc_header = generate_objc_interface(class_name, definition, items, objc_imports)
                if objc_header:
                    objc_headers.append(objc_header)

                # generate objc implementation code
                objc_implementation = generate_objc_implementation(class_name, definition, items, objc_implementations_imports)
                if objc_implementation:
                    objc_implementations.append(objc_implementation)
                    
                # write header file
                objc_output = license
                if (not objc_imports) and (objc_headers != []):
                    objc_output += '\n'.join(objc_headers)
                    write_file(os.path.join(objc_header_output_file_path, f'{objc_lynx_prefix}{class_name}.h'), objc_output)
                elif (objc_headers != []):
                    objc_output += '\n'.join(objc_imports) + '\n\n' + '\n'.join(objc_headers)
                    write_file(os.path.join(objc_header_output_file_path, f'{objc_lynx_prefix}{class_name}.h'), objc_output)
                # write impl file
                objc_implementation_output = license
                if (objc_implementations != []):
                    objc_implementation_output += '\n'.join(objc_implementations_imports) + '\n\n' + '\n'.join(objc_implementations)
                    write_file(os.path.join(objc_impl_output_file_path, f'{objc_lynx_prefix}{class_name}.m'), objc_implementation_output)
        
    # generate ts interface
    ts_output = license
    ts_output += '\n'.join(ts_interfaces)
    ts_file_name = 'lynx-performance-entry.d.ts'
    write_file(os.path.join(ts_output_file_path, ts_file_name), ts_output)

    # generate PerformanceConverter
    converter_name = 'PerformanceEntryConverter'
    # Java
    java_converter_imports = []
    java_converter_code = generate_java_converter(entry_mapping, java_converter_imports)
    java_output = license
    java_output += '\n'.join(java_converter_imports) + '\n\n' + java_converter_code
    write_file(os.path.join(java_output_file_path, f'{converter_name}.java'), java_output)
    # objc
    objc_converter_header = license
    objc_converter_header += generate_objc_converter_header()
    write_file(os.path.join(objc_header_output_file_path, f'{objc_lynx_prefix}{converter_name}.h'), objc_converter_header)
    objc_converter_imports = []
    objc_converter_code = generate_objc_converter(entry_mapping, objc_converter_imports)
    objc_output = license
    objc_output += '\n'.join(objc_converter_imports) + '\n\n' + objc_converter_code
    write_file(os.path.join(objc_impl_output_file_path, f'{objc_lynx_prefix}{converter_name}.m'), objc_output)

if __name__ == "__main__":
    main(os.path.join(base_dir, "performance_entry_definition_files"))
