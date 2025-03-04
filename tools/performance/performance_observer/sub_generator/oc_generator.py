#!/usr/bin/env python3
# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

from utils import *

def generate_objc_interface(class_name, definition, definitions, file_imports):
    lynx_class_name = objc_lynx_prefix + class_name
    file_imports.append('#import <Foundation/Foundation.h>')
    objc_header = f'@interface {lynx_class_name} : NSObject\n'

    if 'allOf' in definition:
        for item in definition['allOf']:
            if '$ref' in item:
                ref_class_name = item['$ref'].split('#/')[-1].split('/')[-1]
                ref_file_name = item['$ref'].split('#/')[0]
                ref_lynx_class_name = objc_lynx_prefix + ref_class_name
                import_statement = f'#import "{ref_lynx_class_name}.h"'
                if import_statement not in file_imports:
                    file_imports.append(import_statement)
                objc_header = f'@interface {lynx_class_name} : {ref_lynx_class_name}\n'

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
            ref_type = value["$ref"].split('/')[-1] + "*"
            if ref_type == 'FrameworkPipelineTiming*':
                prop_type = "NSDictionary*"
            else:
                prop_type = objc_lynx_prefix + ref_type
                ref_class_name = value['$ref'].split('#/')[-1].split('/')[-1]
                ref_lynx_class_name = objc_lynx_prefix + ref_class_name
                import_statement = f'#import "{ref_lynx_class_name}.h"'
                if not import_statement in file_imports:
                    file_imports.append(import_statement)
        else:
            prop_type = value.get("type", "NSString*")
            if prop_type == "integer" or prop_type == "number" or prop_type == "timestamp":
                prop_type = "NSNumber*"
            elif prop_type == "string":
                prop_type = "NSString*"
            elif prop_type == "map":
                prop_type = "NSDictionary*"
        objc_header += f'@property(nonatomic, strong) {prop_type} {prop};\n'
    # If this is a base class, add a rawDictionary to store the original data.
    if class_name == "PerformanceEntry":
        objc_header += f'@property(nonatomic, strong) NSDictionary* rawDictionary;\n'
    objc_header += f'- (instancetype)initWithDictionary:(NSDictionary*)dictionary;\n'
    if class_name == "PerformanceEntry":
        objc_header += f'- (NSDictionary*)toDictionary;\n'
    objc_header += '@end\n'
    return objc_header

def generate_objc_implementation(class_name, definition, definitions, file_imports):
    lynx_class_name = objc_lynx_prefix + class_name
    file_imports.append('#import <Foundation/Foundation.h>')
    file_imports.append(f'#import "{lynx_class_name}.h"')
    objc_implementation = f'@implementation {lynx_class_name}\n\n'

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

    objc_implementation += f'- (instancetype)initWithDictionary:(NSDictionary*)dictionary {{\n'
    if 'allOf' in definition:
        objc_implementation += f'    self = [super initWithDictionary:dictionary];\n'
    else:
        objc_implementation += f'    self = [super init];\n'
    objc_implementation += f'    if (self) {{\n'
    for prop, value in properties.items():
        if "$ref" in value:
            ref_type = value["$ref"].split('/')[-1]
            if ref_type == 'FrameworkPipelineTiming':
                objc_implementation += f'        self.{prop} = dictionary[@"{prop}"];\n'
            else:
                objc_implementation += f'        self.{prop} = dictionary[@"{prop}"] ? [[{objc_lynx_prefix}{ref_type} alloc] initWithDictionary:dictionary[@"{prop}"]] : nil;\n'
        else:
            prop_type = value.get("type", "string")
            if prop_type == "integer" or prop_type == "number" or prop_type == "timestamp":
                default_value = "@(-1)"
            elif prop_type == "string":
                default_value = '@""'
            elif prop_type == "map":
                default_value = "@{}"
            else:
                default_value = 'nil'
            objc_implementation += f'        self.{prop} = dictionary[@"{prop}"] ?: {default_value};\n'
    # If this is a base class, add a rawDictionary to store the original data.
    if class_name == "PerformanceEntry":
        objc_implementation += f'        self.rawDictionary = dictionary;\n'
    objc_implementation += f'    }}\n'
    objc_implementation += f'    return self;\n'
    objc_implementation += f'}}\n\n'
    if class_name == "PerformanceEntry":
        objc_implementation += '- (NSDictionary *)toDictionary {\n'
        objc_implementation += '  return self.rawDictionary;\n'
        objc_implementation += '}\n'
    objc_implementation += f'@end\n'
    return objc_implementation

def generate_objc_converter_header():
    header_code = '#import <Foundation/Foundation.h>\n' \
                  '#import "LynxPerformanceEntry.h"\n\n' \
                  '@interface LynxPerformanceEntryConverter : NSObject\n' \
                  '+ (LynxPerformanceEntry *)makePerformanceEntry:(NSDictionary *)dict;\n' \
                  '@end\n'

    return header_code

def generate_objc_converter(entry_mapping, file_imports):
    oc_code = '@implementation LynxPerformanceEntryConverter\n' \
              '+ (LynxPerformanceEntry *)makePerformanceEntry:(NSDictionary *)dict {\n' \
              '    NSString *name = dict[@"name"];\n' \
              '    NSString *type = dict[@"entry_type"];\n' \
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
        entry_type, entry_name = type_name.split(".")
        if not branch_codes:
            branch_statement = f'    if ([type isEqualToString:@"{entry_type}"] && [name isEqualToString:@"{entry_name}"]) {{\n' \
                               f'        entry = [[{lynx_class_name} alloc] initWithDictionary:dict];\n' \
                               f'    }}'
        else:
            branch_statement = f'    else if ([type isEqualToString:@"{entry_type}"] && [name isEqualToString:@"{entry_name}"]) {{\n' \
                               f'        entry = [[{lynx_class_name} alloc] initWithDictionary:dict];\n' \
                               f'    }}'
        branch_codes.append(branch_statement)
    default_branch = '    else {\n' \
                     '        entry = [[LynxPerformanceEntry alloc] initWithDictionary:dict];\n' \
                     '    }'
    branch_codes.append(default_branch)

    return_code = '    return entry;\n' \
                  '}\n' \
                  '@end\n'
    oc_code += '\n'.join(branch_codes) + '\n' + return_code

    return oc_code
