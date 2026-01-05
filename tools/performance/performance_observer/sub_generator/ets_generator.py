#!/usr/bin/env python3
# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

from utils import *

# ETS PerformanceEntryConverter Generator
# @entry_mapping is a dict, key is 'PerformanceEntry.entryType' + '.' + 'PerformanceEntry.name', value is class name.
# @file_imports is a list, used to store import statements.
def generate_ets_converter(entry_mapping, file_imports):
    ets_code = f'export class PerformanceEntryConverter {{\n' \
    f'  static makePerformanceEntry(record: Record<string, Object>): PerformanceEntry {{\n' \
    f'    const name = record[\'name\']?.toString() ?? \'\';\n' \
    f'    const type = record[\'entryType\']?.toString() ?? \'\';\n' \
    f'    let entry: PerformanceEntry;\n'

    # Generate import for all converted entries.
    file_imports.append('import { PerformanceEntry } from \'./PerformanceEntry\';')
    for type_name, class_name in entry_mapping.items():
        import_statement = f'import {{ {class_name} }} from \'./{class_name}\';'
        if import_statement not in file_imports:
            file_imports.append(import_statement)
    # Generate branch statements for the conversion class.
    branch_codes = []
    for type_name, class_name in entry_mapping.items():
        entry_type, entry_name = type_name.split('.')
        if not branch_codes:
            branch_statement = f'    if (type === \'{entry_type}\' && name === \'{entry_name}\') {{\n' \
                            f'      entry = new {class_name}(record);\n' \
                            f'      entry.typeResolved = true;\n' \
                            f'    }}'
        else:
            branch_statement = f'    else if (type === \'{entry_type}\' && name === \'{entry_name}\') {{\n' \
                            f'      entry = new {class_name}(record);\n' \
                            f'      entry.typeResolved = true;\n' \
                            f'    }}'
        branch_codes.append(branch_statement)
    # Default convert
    default_branch = f'    else {{\n' \
                    f'      entry = new PerformanceEntry(record);\n' \
                    f'      entry.typeResolved = false;\n' \
                    f'    }}'
    branch_codes.append(default_branch)

    return_code = f'    return entry;\n' \
                f'  }}\n' \
                f'}}\n'
    ets_code += '\n'.join(branch_codes) + '\n' + return_code
    return ets_code

# ETS Class Generator
# prop generator, generate the code for properties
# @class_name is class name
# @properties is properties of class
# @java_code is the java code what we need to generate
# @file_imports is a list, used to store import statements.
def generate_ets_prop(class_name, properties, ets_code, file_imports):
    for prop, value in properties.items():
        is_optional = bool(value.get('x-optional'))
        opt = '?' if is_optional else ''
        origin_type = value.get('type')
        if origin_type == 'map':
            # process type of key and value, default is Object
            # only support <primary type, primary type and ref>
            keyType = value.get('keyType')
            keyType = process_primary_type(keyType)[0]

            valueType = value.get('valueType')
            if '$ref' in valueType:
                valueType = valueType['$ref'].split('#/')[-1]
                update_import_statements(file_imports, valueType)
            prop_type = f'Record<{keyType}, {valueType}>'
        elif origin_type is None:
            prop_type = value.get('$ref')
            if (prop_type is not None):
                ref_type = value['$ref'].split('/')[-1]
                if ref_type == 'FrameworkRenderingTiming':
                    prop_type = 'Record<string, Object>'
                else:
                    prop_type = ref_type
                    update_import_statements(file_imports, ref_type)
        else:
            prop_type = process_primary_type(origin_type)[0]
        
        deprecated_msg = value.get('x-deprecated')
        if deprecated_msg is not None:
            ets_code += f'    /** @deprecated {deprecated_msg} */\n'
        if prop_type is not None:
            ets_code += f'    public {prop}{opt}: {prop_type};\n'
    # If this is a base class, add a rawMap to store the original data.
    if class_name == 'PerformanceEntry':
        ets_code += '    public rawRecord: Record<string, Object>;\n'

    return ets_code

# constructor generator, generate the code for constructor
# @class_name is class name
# @definition is definition of class
# @properties is properties of class
# @java_code is the java code what we need to generate
# @file_imports is a list, used to store import statements.
def _escape_ets_string(value):
    return str(value).replace('\\', '\\\\').replace('\'', "\\'")


def _format_ets_default_value(origin_type, value):
    if origin_type == 'string':
        return f'\'{_escape_ets_string(value)}\''
    if origin_type == 'boolean':
        return 'true' if bool(value) else 'false'
    return str(value)


def generate_ets_constructor(class_name, definition, properties, ets_code):
    ets_code += f'    constructor(props: Record<string, Object>) {{\n'
    # If a base class exists, the base class needs to be constructed first
    if 'allOf' in definition:
        ets_code += '        super(props);\n'
    # Generate initial value setting statements for properties
    for prop, value in properties.items():
        is_optional = bool(value.get('x-optional'))
        origin_type = value.get('type')
        if origin_type == 'map':
            # process type of key and value, default is Object
            # only support <String, Object> and <String, primary type>!
            keyType = value.get('keyType')
            keyType = process_primary_type(keyType)[0]

            valueType = value.get('valueType')
            if '$ref' in valueType:
                refType = valueType['$ref'].split('#/')[-1]
                ets_code += f'''        this.detail = {{}};
        let originRecord: Record<string, Object> = (props[\'{prop}\']!== undefined)? props[\'{prop}\'] as Record<string, Object> : {{}};
        Object.entries(originRecord).forEach((entry) => {{
            this.{prop}[entry[0]] = new {refType}(entry[1] as Record<string, Object>);
        }});\n'''
            else:
                valueType = process_primary_type(valueType)[0]
                prop_type = f'Record<{keyType}, {valueType}>'
                if is_optional:
                    default_value = 'undefined'
                else:
                    default_value = '{}'
                ets_code += f'        this.{prop} = (props[\'{prop}\']!== undefined)? props[\'{prop}\'] as {prop_type} : {default_value};\n'
        elif origin_type is None:
            prop_type = value.get('$ref')
            if (prop_type is not None):
                ref_type = prop_type.split('/')[-1]
                if ref_type == 'FrameworkRenderingTiming':
                    prop_type = 'Record<string, Object>'
                    if is_optional:
                        default_value = 'undefined'
                    else:
                        default_value = '{}'
                    ets_code += f'        this.{prop} = (props[\'{prop}\']!== undefined)? props[\'{prop}\'] as {prop_type} : {default_value};\n'
                else:
                    prop_type = ref_type
                    if is_optional:
                        ets_code += f'        this.{prop} = (props[\'{prop}\'] !== undefined) ? new {prop_type}(props[\'{prop}\'] as Record<string, Object>) : undefined;\n'
                    else:
                        ets_code += f'        this.{prop} = (props[\'{prop}\'] !== undefined) ? new {prop_type}(props[\'{prop}\'] as Record<string, Object>) : new {prop_type}({{}});\n'
        else:
            prop_type, default_value = process_primary_type(origin_type)
            yaml_default = value.get('default')
            if (not is_optional) and (yaml_default is not None) and (default_value is not None):
                default_value = _format_ets_default_value(origin_type, yaml_default)
            if prop_type is not None:
                if is_optional:
                    ets_code += f'        this.{prop} = (props[\'{prop}\'] !== undefined && typeof props[\'{prop}\'] === \'{prop_type}\') ? props[\'{prop}\'] : undefined;\n'
                else:
                    ets_code += f'        this.{prop} = (props[\'{prop}\'] !== undefined && typeof props[\'{prop}\'] === \'{prop_type}\') ? props[\'{prop}\'] : {default_value};\n'
    # If this is a base class, add a rawMap to store the original data.
    if class_name == 'PerformanceEntry':
        ets_code += '        this.rawRecord = props;\n'
    ets_code += '    }\n'
    # Add toHashMap method
    if class_name == 'PerformanceEntry':
        ets_code += '    public toRecord(): Record<string, Object> {\n'
        ets_code += '        return this.rawRecord;\n'
        ets_code += '    }\n'

    return ets_code

# main generate
def generate_ets_class(class_name, definition, file_imports):
    ets_code = ''
    if 'x-deprecated' in definition:
        ets_code += '/**\n'
        ets_code += f' * @deprecated {definition["x-deprecated"]}\n'
        ets_code += ' */\n'
    # Define the class name
    ets_code += f'export class {class_name} '

    # Define the parent class type for the interface, if any
    if 'allOf' in definition:
        for item in definition['allOf']:
            if '$ref' in item:
                ref_class_name = item['$ref'].split('#/')[-1].split('/')[-1]
                import_statement = f'import {{ {ref_class_name} }} from \'./{ref_class_name}\';'
                if import_statement not in file_imports:
                    file_imports.append(import_statement)
                ets_code += f'extends {ref_class_name} '
    ets_code += '{\n'

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

    # Generate type definition.
    ets_code = generate_ets_prop(class_name, properties, ets_code, file_imports)

    # generate constructor
    ets_code = generate_ets_constructor(class_name, definition, properties, ets_code)

    ets_code += '}\n'
    return ets_code

# Tool Functions
def process_primary_type(type_name):
    if type_name == 'integer':
        default_value = '-1'
        prop_type = 'number'
    elif type_name == 'number' or type_name == 'timestamp':
        default_value = '-1.0'
        prop_type = 'number'
    elif type_name == 'boolean':
        default_value = 'false'
        prop_type = 'boolean'
    elif type_name == 'string':
        default_value = "''"
        prop_type = 'string'
    else:
        prop_type = None
        default_value = None
    return prop_type, default_value

def update_import_statements(file_imports, class_name):
    import_statement = f'import {{ {class_name} }} from \'./{class_name}\';'
    if import_statement not in file_imports:
        file_imports.append(import_statement)
