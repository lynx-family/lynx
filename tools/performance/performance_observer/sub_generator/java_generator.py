#!/#!/usr/bin/env python3
# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

from utils import *

base_package_name = 'com.lynx.tasm.performance.performanceobserver'

def generate_java_converter(entry_mapping, file_imports):
    java_code = f'public class PerformanceEntryConverter {{\n' \
    f'    static public PerformanceEntry makePerformanceEntry(ReadableMap map) {{\n' \
    f'        HashMap<String, Object> originalEntryMap = map.asHashMap();\n' \
    f'        String name = (String) originalEntryMap.get("name");\n' \
    f'        String type = (String) originalEntryMap.get("entryType");\n' \
    f'        PerformanceEntry entry;\n'

    # Add default import
    file_imports.append(f'package {base_package_name};')
    file_imports.append('import com.lynx.react.bridge.ReadableMap;')
    file_imports.append('import java.util.HashMap;')
    file_imports.append(f'import {base_package_name}.PerformanceEntry;')
    # Generate import for all converted entries.
    for type_name, class_name in entry_mapping.items():
        import_statement = f'import {base_package_name}.{class_name};'
        if import_statement not in file_imports:
            file_imports.append(import_statement)

    # Generate branch statements for the conversion class.
    branch_codes = []
    for type_name, class_name in entry_mapping.items():
        entry_type, entry_name = type_name.split(".")
        if not branch_codes:
            branch_statement = f'        if (type.equals("{entry_type}") && name.equals("{entry_name}")) {{\n' \
                            f'            entry = new {class_name}(map.asHashMap());\n' \
                            f'        }}'
        else:
            branch_statement = f'        else if (type.equals("{entry_type}") && name.equals("{entry_name}")) {{\n' \
                            f'            entry = new {class_name}(map.asHashMap());\n' \
                            f'        }}'
        branch_codes.append(branch_statement)
    # default convert
    default_branch = f'        else {{\n' \
                    f'            entry = new PerformanceEntry(map.asHashMap());\n' \
                    f'        }}'
    branch_codes.append(default_branch)
    
    return_code = f'        return entry;\n' \
                f'    }}\n' \
                f'}}\n'
    java_code += '\n'.join(branch_codes) + '\n' + return_code

    return java_code

def generate_java(class_name, definition, definitions, file_imports):
    java_code = f'public class {class_name} '

    if 'allOf' in definition:
        for item in definition['allOf']:
            if '$ref' in item:
                ref_class_name = item['$ref'].split('#/')[-1].split('/')[-1]
                ref_file_name = item['$ref'].split('#/')[0]
                import_statement = f'import {base_package_name}.{ref_class_name};'
                if import_statement not in file_imports:
                    file_imports.append(import_statement)
                java_code += f'extends {ref_class_name} '
    java_code += '{\n'

    properties = {}
    if 'properties' in definition:
        properties.update(definition['properties'])

    if 'allOf' in definition:
        for item in definition['allOf']:
            if 'properties' in item:
                properties.update(item['properties'])
            if '$ref' in item:
                ref_class = resolve_ref(item['$ref'], definitions, definition_dir)
                if isinstance(ref_class, dict) and 'properties' in ref_class:
                    properties.update(ref_class['properties'])
    # Generate type definition.
    for prop, value in properties.items():
        if "$ref" in value:
            # use ref class
            ref_type = value["$ref"].split('/')[-1]
            if ref_type == 'FrameworkPipelineTiming':
                # Special handling of inconsistent attributes across multiple platforms without updating the reference file.
                prop_type = "HashMap<String, Object>"
            else:
                prop_type = ref_type
                ref_class_name = value['$ref'].split('#/')[-1].split('/')[-1]
                import_statement = f'import {base_package_name}.{ref_class_name};'
                if not import_statement in file_imports:
                    file_imports.append(import_statement)
        else:
            prop_type = value.get("type", "String")  # Default is "string".
            if prop_type == "integer":
                prop_type = "long"
            elif prop_type == "number":
                prop_type = "double"
            elif prop_type == "timestamp":
                prop_type = "double"
            elif prop_type == "string":
                prop_type = "String"
            elif prop_type == "map":
                prop_type = "HashMap<String, Double>"
        java_code += f'    public {prop_type} {prop};\n'
    # If this is a base class, add a rawMap to store the original data.
    if class_name == "PerformanceEntry":
        java_code += '    public HashMap<String,Object> rawMap;\n'

    # generate constructor
    java_code += f'    public {class_name}(HashMap<String, Object> props) {{\n'
    import_hashmap_statement = f'import java.util.HashMap;'
    file_imports.append(import_hashmap_statement)
    # If a base class exists, the base class needs to be constructed first
    if 'allOf' in definition:
        java_code += '        super(props);\n';
    for prop, value in properties.items():
        if "$ref" in value:
            # use ref class
            ref_type = value["$ref"].split('/')[-1]
            if ref_type == 'FrameworkPipelineTiming':
                prop_type = "HashMap<String, Object>"
                java_code += f'        this.{prop} = props.get("{prop}") != null ? (HashMap<String, Object>) props.get("{prop}") : new HashMap<>();\n'
            else:
            # Reference types need to be constructed manually.
                prop_type = "HashMap<String, Object>"
                java_code += f'        this.{prop} = props.get("{prop}") != null ? new {ref_type}(({prop_type}) props.get("{prop}")) : new {ref_type}(new HashMap<>());\n'
        else:
            prop_type = value.get("type", "String")  # Default is "String".
            if prop_type == "integer":
                default_value = "-1L"
                prop_type = "long"
            elif prop_type == "number" or prop_type == "timestamp":
                default_value = "-1.0"
                prop_type = "double"
            elif prop_type == "string":
                default_value = "\"\""
                prop_type = "String"
            elif prop_type == "map":
                default_value = "new HashMap<>()"
                prop_type = "HashMap<String, Double>"
            java_code += f'        this.{prop} = props.get("{prop}") != null ? ({prop_type}) props.get("{prop}") : {default_value};\n'
    # If this is a base class, add a rawMap to store the original data.
    if class_name == "PerformanceEntry":
        java_code += '        this.rawMap = props;\n'
    java_code += '    }\n'
    # Add toHashMap method
    if class_name == "PerformanceEntry":
        java_code += '    public HashMap<String, Object> toHashMap() {\n'
        java_code += '        return this.rawMap;\n'
        java_code += '    }\n'
    
    java_code += '}\n'
    return java_code
