#!/#!/usr/bin/env python3
# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

from utils import *

definition_dir = os.path.join(base_dir, 'definition_yaml_files')

def generate_ts(class_name, definition, definitions, file_imports):
    ts_code = f'export interface {class_name} '

    if 'allOf' in definition:
        for item in definition['allOf']:
            if '$ref' in item:
                ref_class_name = item['$ref'].split('#/')[-1].split('/')[-1]
                import_statement = f'import {{ {ref_class_name} }} from "./{ref_class_name}"'
                if import_statement not in file_imports:
                    file_imports.append(import_statement)
                ts_code += f'extends {ref_class_name} '
    ts_code += '{\n'

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

    for prop, value in properties.items():
        if "$ref" in value:
            # Dealing with reference types
            ref_type = value["$ref"].split('/')[-1]
            if ref_type == 'FrameworkPipelineTiming':
                # Special handling of inconsistent properties on multiple platforms, and no update of import files
                prop_type = ref_type + f'[keyof {ref_type}]'
            else:
                prop_type = ref_type
                # Update import files
                ref_class_name = value['$ref'].split('#/')[-1].split('/')[-1]
                import_statement = f'import {{ {ref_class_name} }} from "./{ref_class_name}"'
                if not import_statement in file_imports:
                    file_imports.append(import_statement)
        else:
            prop_type = value.get("type", "string")  # Default is of type "string".
            if prop_type == "integer" or prop_type == "number" or prop_type == "timestamp":
                prop_type = "number"
            elif prop_type == "string":
                prop_type = "string"
            elif prop_type == "map":
                prop_type = "Record<string, number>"
        ts_code += f'    {prop}: {prop_type};\n'

    ts_code += '}\n'
    return ts_code
