#!/#!/usr/bin/env python3
# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

from utils import *

base_package_name = 'com.lynx.tasm.performance.performanceobserver'

# Java PerformanceEntryConverter Generator
# @entry_mapping is a dict, key is 'PerformanceEntry.entryType' + '.' + 'PerformanceEntry.name', value is class name.
# @file_imports is a list, used to store import statements.
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
        entry_type, entry_name = type_name.split('.')
        if not branch_codes:
            branch_statement = f'        if (type.equals("{entry_type}") && name.equals("{entry_name}")) {{\n' \
                            f'            entry = new {class_name}(map.asHashMap());\n' \
                            f'            entry.typeResolved = true;\n' \
                            f'        }}'
        else:
            branch_statement = f'        else if (type.equals("{entry_type}") && name.equals("{entry_name}")) {{\n' \
                            f'            entry = new {class_name}(map.asHashMap());\n' \
                            f'            entry.typeResolved = true;\n' \
                            f'        }}'
        branch_codes.append(branch_statement)
    # default convert
    default_branch = f'        else {{\n' \
                    f'            entry = new PerformanceEntry(map.asHashMap());\n' \
                    f'            entry.typeResolved = false;\n' \
                    f'        }}'
    branch_codes.append(default_branch)
    
    return_code = f'        return entry;\n' \
                f'    }}\n' \
                f'}}\n'
    java_code += '\n'.join(branch_codes) + '\n' + return_code

    return java_code

# Java Class Generator
# prop generator, generate the code for properties
# @class_name is class name
# @properties is properties of class
# @java_code is the java code what we need to generate
# @file_imports is a list, used to store import statements.
def generate_java_prop(class_name, properties, java_code, file_imports):
    for prop, value in properties.items():
        is_optional = bool(value.get('x-optional'))
        origin_type = value.get('type')
        if origin_type == 'map':
            # process type of key and value, default is Object
            # only support <String, Object> and <String, primary type>!
            keyType = value.get('keyType')
            keyType = convert_to_java_type(process_primary_type(keyType)[0])
            
            valueType = value.get('valueType')
            if '$ref' in valueType:
                valueType = valueType['$ref'].split('#/')[-1]
                update_import_statements(file_imports, valueType)
            else:
                valueType = convert_to_java_type(process_primary_type(valueType)[0])
            prop_type = f'HashMap<{keyType}, {valueType}>'
        elif origin_type is None:
            prop_type = value.get('$ref')
            if (prop_type is not None):
                ref_type = value['$ref'].split('/')[-1]
                if ref_type == 'FrameworkRenderingTiming':
                    # Special handling of inconsistent properties on multiple platforms, and no update of import files
                    prop_type = 'HashMap<String, Object>'
                else:
                    prop_type = ref_type
                    update_import_statements(file_imports, ref_type)
        else:
            prop_type = process_primary_type(origin_type)[0]
            if is_optional:
                prop_type = convert_to_java_type(prop_type)

        if prop_type is not None:
            nullability_annotation = None
            if not is_java_primitive(prop_type):
                if is_optional:
                    nullability_annotation = '@Nullable'
                    nullable_import = 'import androidx.annotation.Nullable;'
                    if nullable_import not in file_imports:
                        file_imports.append(nullable_import)
                else:
                    nullability_annotation = '@NonNull'
                    nonnull_import = 'import androidx.annotation.NonNull;'
                    if nonnull_import not in file_imports:
                        file_imports.append(nonnull_import)

            if 'x-deprecated' in value:
                java_code += '    /**\n'
                java_code += f'     * @deprecated {value["x-deprecated"]}\n'
                java_code += '     */\n'
                java_code += '    @Deprecated\n'
            if nullability_annotation is not None:
                java_code += f'    {nullability_annotation}\n'
            java_code += f'    public {prop_type} {prop};\n'
    # If this is a base class, add a rawMap to store the original data.
    if class_name == 'PerformanceEntry':
        java_code += '    public HashMap<String,Object> rawMap;\n'

    return java_code

def _escape_java_string(value):
    return str(value).replace('\\', '\\\\').replace('"', '\\"')


def _format_java_default_value(origin_type, value):
    if origin_type == 'string':
        return f'"{_escape_java_string(value)}"'
    if origin_type == 'boolean':
        return 'true' if bool(value) else 'false'
    return str(value)


# constructor generator, generate the code for constructor
# @class_name is class name
# @definition is definition of class
# @properties is properties of class
# @java_code is the java code what we need to generate
# @file_imports is a list, used to store import statements.
def generate_java_constructor(class_name, definition, properties, java_code, file_imports):
    java_code += f'    public {class_name}(HashMap<String, Object> props) {{\n'
    import_hashmap_statement = f'import java.util.HashMap;'
    file_imports.append(import_hashmap_statement)
    # If a base class exists, the base class needs to be constructed first
    if 'allOf' in definition:
        java_code += '        super(props);\n';
    # Generate initial value setting statements for properties 
    for prop, value in properties.items():
        is_optional = bool(value.get('x-optional'))
        origin_type = value.get('type')
        if origin_type == 'map':
            # process type of key and value
            # only support <primary type, primary type and ref>
            keyType = value.get('keyType')
            keyType = convert_to_java_type(process_primary_type(keyType)[0])
            
            valueType = value.get('valueType')
            if '$ref' in valueType:
                # ref type require construction
                refType = valueType['$ref'].split('#/')[-1]
                java_code += f'''        HashMap<String, Object> originMap = props.get("{prop}")!= null? (HashMap<String, Object>) props.get("{prop}") : new HashMap<>();
        this.{prop} = new HashMap<>();
        for (Map.Entry<String, Object> entry : originMap.entrySet()) {{
            this.{prop}.put(entry.getKey(), new {refType}((HashMap<String, Object>) entry.getValue()));
        }}\n'''
                # add Map dependency
                import_map_statement = f'import java.util.Map;'
                if import_map_statement not in file_imports:
                    file_imports.append(import_map_statement)
                continue
            else:
                valueType = convert_to_java_type(process_primary_type(valueType)[0])
                prop_type = f'HashMap<{keyType}, {valueType}>'
                default_value = 'new HashMap<>()'
                java_code += f'        this.{prop} = props.get("{prop}")!= null? ({prop_type}) props.get("{prop}") : {default_value};\n'
        elif origin_type is None:
            prop_type = value.get('$ref')
            if (prop_type is not None):
                ref_type = prop_type.split('/')[-1]
                if ref_type == 'FrameworkRenderingTiming':
                    prop_type = 'HashMap<String, Object>'
                    if is_optional:
                        java_code += f'        this.{prop} = props.get("{prop}") != null ? ({prop_type}) props.get("{prop}") : null;\n'
                    else:
                        default_value = 'new HashMap<>()'
                        java_code += f'        this.{prop} = props.get("{prop}") != null ? ({prop_type}) props.get("{prop}") : {default_value};\n'
                else:
                    prop_type = ref_type
                    if is_optional:
                        java_code += f'        this.{prop} = props.get("{prop}") != null ? new {prop_type}((HashMap<String, Object>) props.get("{prop}")) : null;\n'
                    else:
                        default_value = f'new {prop_type}(new HashMap<>())'
                        java_code += f'        this.{prop} = props.get("{prop}") != null ? new {prop_type}((HashMap<String, Object>) props.get("{prop}")) : {default_value};\n'
        else:
            prop_type, default_value = process_primary_type(origin_type)
            yaml_default = value.get('default')
            if (not is_optional) and (yaml_default is not None) and (default_value is not None):
                default_value = _format_java_default_value(origin_type, yaml_default)
            if prop_type is not None:
                if is_optional:
                    boxed = convert_to_java_type(prop_type)
                    java_code += f'        this.{prop} = props.get("{prop}") != null ? ({boxed}) props.get("{prop}") : null;\n'
                else:
                    java_code += f'        this.{prop} = props.get("{prop}") != null ? ({prop_type}) props.get("{prop}") : {default_value};\n'
    # If this is a base class, add a rawMap to store the original data.
    if class_name == 'PerformanceEntry':
        java_code += '        this.rawMap = props;\n'
    java_code += '    }\n'
    # Add toHashMap method
    if class_name == 'PerformanceEntry':
        java_code += '    public HashMap<String, Object> toHashMap() {\n'
        java_code += '        return this.rawMap;\n'
        java_code += '    }\n'

    return java_code

# main generator
# @class_name is the name of the class
# @definition is the definition of the class
# @file_imports is a list, used to store import statements.
def generate_java(class_name, definition, file_imports):
    java_code = ''
    if 'x-deprecated' in definition:
        java_code += '/**\n'
        java_code += f' * @deprecated {definition["x-deprecated"]}\n'
        java_code += ' */\n'
        java_code += '@Deprecated\n'

    # Define the class name
    java_code += f'public class {class_name} '

    # Define the parent class type for the interface, if any
    if 'allOf' in definition:
        for item in definition['allOf']:
            if '$ref' in item:
                ref_class_name = item['$ref'].split('#/')[-1].split('/')[-1]
                java_code += f'extends {ref_class_name} '
                # Add import
                import_statement = f'import {base_package_name}.{ref_class_name};'
                if import_statement not in file_imports:
                    file_imports.append(import_statement)
    java_code += '{\n'

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

    # Parse property definitions and generate corresponding Java variable
    java_code = generate_java_prop(class_name, properties, java_code, file_imports)

    # Generate Java constructor function
    java_code = generate_java_constructor(class_name, definition, properties, java_code, file_imports)
    
    java_code += '}\n'
    return java_code

# Tool Functions
def process_primary_type(type_name):
    if type_name == 'integer':
        default_value = '-1'
        prop_type = 'int'
    elif type_name == 'number' or type_name == 'timestamp':
        default_value = '-1.0'
        prop_type = 'double'
    elif type_name == 'long':
        default_value = '-1'
        prop_type = 'long'
    elif type_name == 'boolean':
        default_value = 'false'
        prop_type = 'boolean'
    elif type_name == 'string':
        default_value = '\"\"'
        prop_type = 'String'
    else:
        prop_type = None
        default_value = None
    return prop_type, default_value

def convert_to_java_type(type_name):
    if type_name == 'int':
        prop_type = 'Integer'
    elif type_name == 'double':
        prop_type = 'Double'
    elif type_name == 'boolean':
        prop_type = 'Boolean'
    elif type_name == 'long':
        prop_type = 'Long'
    elif type_name == 'string':
        prop_type = 'String'
    else:
        prop_type = type_name
    return prop_type

def is_java_primitive(type_name):
    return type_name in ('int', 'double', 'boolean', 'long', 'float', 'short', 'byte', 'char')

def update_import_statements(file_imports, class_name):
    import_statement = f'import {base_package_name}.{class_name};'
    if not import_statement in file_imports:
        file_imports.append(import_statement)
