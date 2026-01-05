#!/#!/usr/bin/env python3
# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

from utils import *

definition_dir = os.path.join(base_dir, 'definition_yaml_files')


def generate_ts_union_type(class_name, definition, definitions):
    union_type = definition['ts-discriminated-union']
    ts_code = f'export type {class_name} = '
    for item in union_type:
        if '$ref' in item:
            ref_class_name = item['$ref'].split('#/')[-1].split('/')[-1]
            ts_code += f'{ref_class_name} | '
        else:
            ts_code += f'{item} | '
    ts_code = ts_code[:-3]
    return ts_code

# TypeScript Typing Generator
# prop generator
def generate_ts_prop(properties, ts_code):
    for prop, value in properties.items():
        is_optional = bool(value.get('x-optional'))
        opt = '?' if is_optional else ''
        deprecated_msg = value.get('x-deprecated')
        if deprecated_msg is not None:
            ts_code += f'    /** @deprecated {deprecated_msg} */\n'
        if 'ts-literal-type' in value:
            ts_code += f'    {prop}{opt}: \'{value.get("ts-literal-type")}\';\n'
            continue
        prop_type = value.get('type')
        if prop_type == 'map':
            # process type of key and value, default is any
            # only support <number/string, number/string/ref>
            keyType = value.get('keyType')
            keyType = process_primary_type(keyType)
            valueType = value.get('valueType')
            if '$ref' in valueType:
                valueType = valueType['$ref'].split('#/')[-1].split('/')[-1]
            else:
                valueType = process_primary_type(valueType)
            prop_type = f'Record<{keyType}, {valueType}>'
        elif prop_type is None:
            prop_type = value.get('$ref')
            if (prop_type is not None):
                ref_type = value['$ref'].split('/')[-1]
                if ref_type == 'FrameworkRenderingTiming':
                    # Special handling of inconsistent properties on multiple platforms
                    prop_type = 'FrameworkRenderingTimings[keyof FrameworkRenderingTimings] & FrameworkRenderingTiming'
                else:
                    prop_type = ref_type
        else:
            prop_type = process_primary_type(prop_type)
        # default is any, do not handle with 'None'
        ts_code += f'    {prop}{opt}: {prop_type};\n'

    return ts_code

# main generator
def generate_ts(class_name, definition, definitions):
    if 'ts-discriminated-union' in definition:
        ts_code = generate_ts_union_type(class_name, definition, definitions)
        return ts_code

    ts_code = f''
    # Define the interface name
    if 'x-deprecated' in definition:
        ts_code += f'/**\n' \
        f' * @deprecated {definition["x-deprecated"]}\n' \
        f' **/\n' 

    # Define the interface name
    ts_code += f'export interface {class_name} '

    # Define the parent class type for the interface, if any
    if 'allOf' in definition:
        for item in definition['allOf']:
            if '$ref' in item:
                ref_class_name = item['$ref'].split('#/')[-1].split('/')[-1]
                ts_code += f'extends {ref_class_name} '
    ts_code += '{\n'

    # Retrieve all property definitions
    properties = {}
    # Directly add all properties if there are no inheritance relationships
    if 'properties' in definition:
        properties.update(definition['properties'])
    # If inheritance relationships exist, retrieve all properties from allOf
    if 'allOf' in definition:
        for item in definition['allOf']:
            if 'properties' in item:
                properties.update(item['properties'])

    # Parse property definitions and generate corresponding TypeScript structures
    ts_code = generate_ts_prop(properties, ts_code)

    ts_code += '}\n'
    return ts_code

# Tool Functions
def process_primary_type(type_name):
    if type_name == 'integer' or type_name == 'number' or type_name == 'timestamp' or type_name == 'long':
        prop_type = 'number'
    elif type_name == 'boolean':
        prop_type = 'boolean'
    elif type_name == 'string':
        prop_type = 'string'
    else:
        prop_type = 'any'
    return prop_type
