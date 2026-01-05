#!/usr/bin/env python3
# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

from utils import *

# Object-C Header Generator
# @class_name is the name of the class
# @definition is the definition of the class
# @file_imports is a list, used to store import statements.
def generate_objc_interface(class_name, definition, file_imports):
    lynx_class_name = objc_lynx_prefix + class_name
    file_imports.append('#import <Foundation/Foundation.h>')
    objc_header = 'NS_ASSUME_NONNULL_BEGIN\n\n'
    if 'x-deprecated' in definition:
        msg = str(definition["x-deprecated"]).replace('"', '\\"')
        objc_header += f'/// Deprecated: {definition["x-deprecated"]}\n'
        objc_header += f'__attribute__((deprecated("{msg}")))\n'
    objc_header += f'@interface {lynx_class_name} : NSObject\n'

    # Handle inheritance relationships and add references
    if 'allOf' in definition:
        for item in definition['allOf']:
            if '$ref' in item:
                ref_class_name = item['$ref'].split('#/')[-1].split('/')[-1]
                ref_lynx_class_name = objc_lynx_prefix + ref_class_name
                import_statement = f'#import "{ref_lynx_class_name}.h"'
                if import_statement not in file_imports:
                    file_imports.append(import_statement)
                objc_header = 'NS_ASSUME_NONNULL_BEGIN\n\n'
                if 'x-deprecated' in definition:
                    msg = str(definition["x-deprecated"]).replace('"', '\\"')
                    objc_header += f'/// Deprecated: {definition["x-deprecated"]}\n'
                    objc_header += f'__attribute__((deprecated("{msg}")))\n'
                objc_header += f'@interface {lynx_class_name} : {ref_lynx_class_name}\n'

    # Collect all properties
    properties = {}
    if 'properties' in definition:
        properties.update(definition['properties'])

    if 'allOf' in definition:
        for item in definition['allOf']:
            if 'properties' in item:
                properties.update(item['properties'])

    # Determine the properties of the type and add references
    for prop, value in properties.items():
        origin_type = value.get('type')
        if origin_type == 'map':
            key_type = value.get('keyType')
            key_type = map_type_to_objc(key_type)
            value_type = value.get('valueType')
            if isinstance(value_type, dict):
                ref_value = value_type.get('$ref') 
                value_type = value_type.get('$ref').split('#/')[-1].split('/')[-1]
                value_type = f'{objc_lynx_prefix}{value_type}'
                import_statement = f'#import "{value_type}.h"'
                file_imports.append(import_statement)
            else:
                value_type = map_type_to_objc(value_type)

            prop_type = f'NSDictionary<{key_type}*, {value_type}*>*'

        elif origin_type is None:
            prop_type = value.get('$ref')
            if (prop_type is not None):
                ref_type = prop_type.split('/')[-1]
                if ref_type == 'FrameworkRenderingTiming':
                    prop_type = 'NSDictionary*'
                else:
                    prop_type = objc_lynx_prefix + ref_type + '*'
                    update_import_statements(file_imports, ref_type)
                    
        else:
            prop_type = process_primary_type(origin_type)[0]
        property_attribute = 'nonatomic, strong'
        if prop_type == 'BOOL':
            property_attribute = 'nonatomic, assign'
        else:
            if bool(value.get('x-optional')):
                property_attribute = f'{property_attribute}, nullable'

        deprecated_attr = ''
        if 'x-deprecated' in value:
            msg = str(value['x-deprecated']).replace('"', '\\"')
            objc_header += f'/// Deprecated: {value["x-deprecated"]}\n'
            deprecated_attr = f' __attribute__((deprecated("{msg}")))'
        objc_header += f'@property({property_attribute}) {prop_type} {prop}{deprecated_attr};\n'
    # If this is a base class, add a rawDictionary to store the original data.
    if class_name == 'PerformanceEntry':
        objc_header += f'@property(nonatomic, strong) NSDictionary* rawDictionary;\n'
    objc_header += f'- (instancetype)initWithDictionary:(NSDictionary*)dictionary;\n'
    if class_name == 'PerformanceEntry':
        objc_header += f'- (NSDictionary*)toDictionary;\n'
    objc_header += '@end\n\nNS_ASSUME_NONNULL_END\n'
    return objc_header

def _escape_objc_string(value):
    return str(value).replace('\\', '\\\\').replace('"', '\\"')


def _format_objc_default_value(origin_type, value):
    if origin_type == 'string':
        return f'@"{_escape_objc_string(value)}"'
    if origin_type == 'boolean':
        return 'YES' if bool(value) else 'NO'
    return f'@({value})'


# Object-C Implementation Generator
# @class_name is the name of the class
# @definition is the definition of the class
# @file_imports is a list, used to store import statements.
def generate_objc_implementation(class_name, definition, file_imports):
    lynx_class_name = objc_lynx_prefix + class_name
    file_imports.append('#import <Foundation/Foundation.h>')
    file_imports.append(f'#import "{lynx_class_name}.h"')

    objc_implementation = ''
    if 'x-deprecated' in definition:
        objc_implementation += '#pragma clang diagnostic push\n'
        objc_implementation += '#pragma clang diagnostic ignored "-Wdeprecated-implementations"\n'

    objc_implementation += f'@implementation {lynx_class_name}\n\n'

    # Collect all properties
    properties = {}
    if 'properties' in definition:
        properties.update(definition['properties'])

    if 'allOf' in definition:
        for item in definition['allOf']:
            if 'properties' in item:
                properties.update(item['properties'])

    # Generate implementation constructor
    objc_implementation += f'- (instancetype)initWithDictionary:(NSDictionary*)dictionary {{\n'
    # If a base class exists, the base class needs to be constructed first
    if 'allOf' in definition:
        objc_implementation += f'    self = [super initWithDictionary:dictionary];\n'
    else:
        objc_implementation += f'    self = [super init];\n'
    objc_implementation += f'    if (self) {{\n'
    # Generate constructors for all properties and update references
    for prop, value in properties.items():
        origin_type = value.get('type')
        if origin_type == 'map':
            # process type of key and value
            # only support <primary type, primary type and ref>
            keyType = value.get('keyType')
            keyType = process_primary_type(keyType)[0]

            valueType = value.get('valueType')
            if '$ref' in valueType:
                # The ref type needs to construct all valueObjects, which will be saved in a temporary NSMutableDictionary.
                # Finally, it is transferred to this.prop
                refType = valueType['$ref'].split('#/')[-1]
                objc_implementation += f'''        NSDictionary *originMap = dictionary[@"{prop}"] ? : @{{}};
        NSMutableDictionary *details = [[NSMutableDictionary alloc] init];
        for (NSString *key in originMap) {{
            NSDictionary *entryValue = originMap[key];
            {objc_lynx_prefix}{refType} *item = [[{objc_lynx_prefix}{refType} alloc] initWithDictionary:entryValue];
            [details setObject:item forKey:key];
        }}
        self.{prop} = details;\n'''
                # update import
                update_import_statements(file_imports, refType)
            else:
                prop_type = 'NSDictionary*'
                default_value = '@{}'
                objc_implementation += f'        self.{prop} = dictionary[@"{prop}"]?: {default_value};\n'
        elif origin_type is None:
            prop_type = value.get('$ref')
            if (prop_type is not None):
                ref_type = prop_type.split('/')[-1]
                if ref_type == 'FrameworkRenderingTiming':
                    prop_type = 'NSDictionary*'
                    default_value = '@{}'
                    objc_implementation += f'        self.{prop} = dictionary[@"{prop}"]?: {default_value};\n'
                else:
                    default_value = f'[[{objc_lynx_prefix}{ref_type} alloc] initWithDictionary:@{{}}]'
                    objc_implementation += f'        self.{prop} = dictionary[@"{prop}"] ? [[{objc_lynx_prefix}{ref_type} alloc] initWithDictionary:dictionary[@"{prop}"]] : {default_value};\n'
                    update_import_statements(file_imports, ref_type)
        else:
            prop_type, default_value = process_primary_type(origin_type)
            yaml_default = value.get('default')
            if yaml_default is not None and default_value is not None:
                if origin_type == 'boolean':
                    default_value = _format_objc_default_value(origin_type, yaml_default)
                elif origin_type in ('integer', 'long', 'number', 'timestamp'):
                    default_value = _format_objc_default_value(origin_type, yaml_default)
                elif origin_type == 'string':
                    default_value = _format_objc_default_value(origin_type, yaml_default)
            if prop_type is not None:
                if (prop_type == 'BOOL'):
                    objc_implementation += f'        id value_{prop} = dictionary[@"{prop}"];\n'
                    objc_implementation += f'        self.{prop} = value_{prop} != nil ? [value_{prop} boolValue] : {default_value};\n'
                else:
                    objc_implementation += f'        self.{prop} = dictionary[@"{prop}"]?: {default_value};\n'
    # If this is a base class, add a rawDictionary to store the original data.
    if class_name == 'PerformanceEntry':
        objc_implementation += f'        self.rawDictionary = dictionary;\n'
    objc_implementation += f'    }}\n'
    objc_implementation += f'    return self;\n'
    objc_implementation += f'}}\n\n'
    if class_name == 'PerformanceEntry':
        objc_implementation += '- (NSDictionary *)toDictionary {\n'
        objc_implementation += '  return self.rawDictionary;\n'
        objc_implementation += '}\n'
    objc_implementation += f'@end\n'
    if 'x-deprecated' in definition:
        objc_implementation += '\n#pragma clang diagnostic pop\n'
    return objc_implementation

# Object-C PerformanceEntryConverter Header Generator
def generate_objc_converter_header():
    header_code = '#import <Foundation/Foundation.h>\n' \
                  '#import "LynxPerformanceEntry.h"\n\n' \
                  '@interface LynxPerformanceEntryConverter : NSObject\n' \
                  '+ (LynxPerformanceEntry *)makePerformanceEntry:(NSDictionary *)dict;\n' \
                  '@end\n'

    return header_code

# Object-C PerformanceEntryConverter Generator
# @entry_mapping is a dict, key is 'PerformanceEntry.entryType' + '.' + 'PerformanceEntry.name', value is class name.
# @file_imports is a list, used to store import statements.
def generate_objc_converter(entry_mapping, file_imports):
    oc_code = ''
    oc_code += '#pragma clang diagnostic push\n'
    oc_code += '#pragma clang diagnostic ignored "-Wdeprecated-declarations"\n'

    oc_code += '@implementation LynxPerformanceEntryConverter\n' \
              '+ (LynxPerformanceEntry *)makePerformanceEntry:(NSDictionary *)dict {\n' \
              '    NSString *name = dict[@"name"];\n' \
              '    NSString *type = dict[@"entryType"];\n' \
              '    LynxPerformanceEntry *entry;\n'

    # Add default import
    file_imports.append(f'#import <Foundation/Foundation.h>')
    file_imports.append(f'#import "LynxPerformanceEntryConverter.h"')
    # Generate import for all converted entries.
    for type_name, class_name in entry_mapping.items():
        lynx_class_name = objc_lynx_prefix + class_name
        import_statement = f'#import "{lynx_class_name}.h"'
        if import_statement not in file_imports:
            file_imports.append(import_statement)

    branch_codes = []
    for type_name, class_name in entry_mapping.items():
        lynx_class_name = objc_lynx_prefix + class_name
        entry_type, entry_name = type_name.split('.')
        if not branch_codes:
            branch_statement = f'    if ([type isEqualToString:@"{entry_type}"] && [name isEqualToString:@"{entry_name}"]) {{\n' \
                               f'        entry = [[{lynx_class_name} alloc] initWithDictionary:dict];\n' \
                               f'        entry.typeResolved = YES;\n' \
                               f'    }}'
        else:
            branch_statement = f'    else if ([type isEqualToString:@"{entry_type}"] && [name isEqualToString:@"{entry_name}"]) {{\n' \
                               f'        entry = [[{lynx_class_name} alloc] initWithDictionary:dict];\n' \
                               f'        entry.typeResolved = YES;\n' \
                               f'    }}'
        branch_codes.append(branch_statement)
    default_branch = '    else {\n' \
                     '        entry = [[LynxPerformanceEntry alloc] initWithDictionary:dict];\n' \
                     '        entry.typeResolved = NO;\n' \
                     '    }'
    branch_codes.append(default_branch)

    return_code = '    return entry;\n' \
                  '}\n' \
                  '@end\n' \
                  '#pragma clang diagnostic pop\n'
    oc_code += '\n'.join(branch_codes) + '\n' + return_code

    return oc_code

# Tool Functions
def process_primary_type(type_name):
    if type_name == 'integer' or type_name == 'long' or type_name == 'number' or type_name == 'timestamp':
        default_value = '@(-1)'
        prop_type = 'NSNumber*'
    elif type_name == 'string':
        default_value = '@\"\"'
        prop_type = 'NSString*'
    elif type_name == 'boolean':
        default_value = 'NO'
        prop_type = 'BOOL'
    elif type_name == 'map':
        default_value = None
        prop_type = 'NSDictionary*'
    elif type_name == 'array':
        default_value = None
        prop_type = 'NSArray*'
    else:
        prop_type = None
        default_value = None
    return prop_type, default_value

def update_import_statements(file_imports, class_name):
    lynx_class_name = objc_lynx_prefix + class_name
    import_statement = f'#import "{lynx_class_name}.h"'
    if not import_statement in file_imports:
        file_imports.append(import_statement)

def map_type_to_objc(type_name):
    OBJC_TYPE_MAPPING = {
        'integer': 'NSNumber',
        'number': 'NSNumber',
        'timestamp': 'NSNumber',
        'string': 'NSString',
        'boolean': 'BOOL',
        'map': 'NSDictionary',
        'array': 'NSArray'
    }
    return OBJC_TYPE_MAPPING.get(type_name)