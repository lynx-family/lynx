# Copyright 2019 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

#!/usr/bin/python
# -*- coding: UTF-8 -*-
import json
import string
import utils

# auto generate enum
def genEnum(handler_name, css_object_jsons):
  enum_template = string.Template("""enum class ${class_name}Type : unsigned {
${enum_values}};
""")
  enum_values_template = string.Template("""  k${enum_value} = ${index},   // version:${version}
""")
  css_types_source = ''
  for css_json in css_object_jsons:
    enum_values_source = ""
    class_name = utils.underline2hump(css_json['name'])
    for i, val in enumerate(css_json['values']):
      if "align-type" in list(val.keys()) :
        continue
      enum_value = utils.underline2hump(val['value'])
      enum_values_source += enum_values_template.substitute(
          enum_value=enum_value, index=i, version=val['version'])
    css_types_source += enum_template.substitute(class_name=class_name, enum_values=enum_values_source)
  utils.genCSSType(utils.getCSSStylePath() + "auto_gen_css_type.h", ''.join(css_types_source))
  # utils.fillTemplate(utils.getCSSStylePath() + "css_type.h", ''.join(css_types_source))

# auto generate java enum
def genJavaEnum(handler_name, css_object_jsons):
  enum_values_java_template = string.Template("""  public static final int ${class_name}_${enum_value} = ${index};  // version:${version}
""")
  source = ''
  for css_json in css_object_jsons:
    class_name = utils.underline2hump(css_json['name'])
    consumption_status = css_json['consumption_status']
    if (consumption_status == 'layout-only' or handleSpecialID(class_name)) :
      continue
    enum_java_values_source = "  //" + class_name + "\n"
    for i, val in enumerate(css_json['values']):
      if "align-type" in list(val.keys()) :
        continue
      enum_value = utils.underline2hump(val['value']).upper()
      enum_java_values_source += enum_values_java_template.substitute(
          class_name=class_name.upper(), enum_value=enum_value, index=i, version=val['version'])
    enum_java_values_source += "\n"
    source += enum_java_values_source
  # utils.fillTemplate(utils.getJavaStyleConstantPath(), ''.join(source))
  utils.genJavaStyleConstants(utils.getAutoGenJavaStyleConstantPath(), ''.join(source))

def genStringToEnum(handler_name, css_object_jsons):
  source = ''
  func_statement_template = string.Template("""using starlight::${class_name}Type;
static bool To${class_name}Type(std::string_view str, int& result) {
  ${class_name}Type type = ${class_name}Type::k${default_type};
  ${if_source}${else_if_source}
  } else {
    return false;
  }
  result = static_cast<int>(type);
  return true;
}\n\n""")
  if_source_template = string.Template(
      """if (str == "${enum_value}") {
    type = ${class_name}Type::k${enum_upper_value};""")
  elseIf_source_template = string.Template("""
  } else if (str == "${enum_value}") {
    type = ${class_name}Type::k${enum_upper_value};""")
  for enum_css_obj in css_object_jsons:
    class_name = utils.underline2hump(enum_css_obj['name'])
    css_name = enum_css_obj['name']
    filename = enum_css_obj['name'].replace('-', '_')
    values = enum_css_obj['values']
    default_type = utils.underline2hump(enum_css_obj['default_value'])
    valuesLen = len(values)
    if_source = ""
    else_if_source = ""
    for i, val in enumerate(values):
      enum_value = val['value']
      enum_upper_value = utils.underline2hump(enum_value)
      if (enum_value == 'start' or enum_value == 'end') :
          if 'align-type' in list(val.keys()):
            align_value = val['align-type']
            enum_upper_value = utils.underline2hump(align_value)
      if i == 0:
        if_source = if_source_template.substitute(
            class_name=class_name,
            enum_value=enum_value, enum_upper_value=enum_upper_value
        )
      else:
        else_if_source += elseIf_source_template.substitute(
            class_name=class_name,
            enum_value=enum_value, enum_upper_value=enum_upper_value
        )
    source += func_statement_template.substitute(class_name = class_name, \
      default_type = default_type, if_source = if_source, else_if_source = else_if_source)
  utils.fillHandler(handler_name, source)

def genStatements(handler_name, css_object_jsons):
  source = ''
  case_template = string.Template("""    case kPropertyID${class_name}:
      success = To${class_name}Type(str, result);
      break;
""")
  for enum_css_obj in css_object_jsons: 
    class_name = utils.underline2hump(enum_css_obj['name'])
    source += case_template.substitute(class_name = class_name)
  utils.fillHandler(handler_name, source, 1)


# auto generate oc enum
def genOCEnum(handler_name, css_object_jsons):
  enum_template = string.Template("""typedef NS_ENUM(NSUInteger, Lynx${class_name}Type) {
${enum_values}};
\n""")
  enum_values_template = string.Template("""  Lynx${class_name}${enum_value} = ${index},   // version:${version}
""")
  source = ''
  for css_json in  css_object_jsons:
    enum_values_source = ""
    class_name = utils.underline2hump(css_json['name'])
    consumption_status = css_json['consumption_status']
    if (consumption_status == 'layout-only' or handleSpecialID(class_name)) :
      continue
    for i, val in enumerate(css_json['values']):
      if "align-type" in list(val.keys()) :
        continue
      enum_value = utils.underline2hump(val['value'])
      enum_values_source += enum_values_template.substitute(
          class_name=class_name, enum_value=enum_value, index=i, version=val['version'])
    source += enum_template.substitute(class_name=class_name, enum_values=enum_values_source)
  # utils.fillTemplate(utils.getOCStyleConstantPath(), ''.join(source))
  utils.genOCLynxCSSType(utils.getAutoGenOCStyleConstantPath(), ''.join(source))

# auto generate enum to string.
def genEnumToStringHeader(handler_name, css_object_jsons):
  source = ''
  for css_json in css_object_jsons:
    class_name = utils.underline2hump(css_json['name'])
    func_define_template = string.Template("""  static std::string To${class_name}Type(lynx::starlight::${class_name}Type type);\n\n""")
    source += func_define_template.substitute(class_name=class_name)
  # utils.fillTemplate(utils.getCSSDebugHeaderPath(), ''.join(source))
  utils.genAutoGenCSSDecoderHeader(utils.getAutoGenCSSDebugHeaderPath(), ''.join(source))


def genEnumToStringSource(handler_name, css_object_jsons):
   func_statement_template = string.Template("""std::string AutoGenCSSDecoder::To${class_name}Type(${class_name}Type type) {
  switch (type) {
${cases}  }
}\n\n""")
   cases_template = string.Template("""    case ${class_name}Type::k${enum_upper_value}:
      return "${enum_value}";\n""")
   source = ''
   for css_json in css_object_jsons:
    class_name = utils.underline2hump(css_json['name'])
    values = css_json['values']
    cases_source = ""
    for i, val in enumerate(values):
      if 'align-type' in list(val.keys()):
        continue
      enum_value = val['value']
      enum_upper_value = utils.underline2hump(enum_value)
      cases_source += cases_template.substitute(
          class_name=class_name, enum_upper_value=enum_upper_value, enum_value=enum_value)
    source += func_statement_template.substitute(
        class_name=class_name, cases=cases_source)
  #  utils.fillTemplate(utils.getCSSDebugSourcePath(),
  #                     ''.join(source))
   utils.genAutoGenCSSDecoderSource(utils.getAutoGenCSSDebugSourcePath(), ''.join(source)) 
# this function is used to mantain a map which the enum type do not want to be generated in java and OC by script
# but meanwhile the consumption_status are not layout-only
# the definition of these enum types are different between C++ and Java&OC, so that these enum types should be defined
# manually, if the new add enum type is also satisfied with these conditions, you should add to this map
def handleSpecialID(class_name):
  specialID = ['TextAlign','Direction']
  for key in specialID :
    if (key == class_name) :
      return True
  return False

def genHandler(handler_name, enum_css_object_json):
  utils.genGroupHandler(handler_name, enum_css_object_json, 2)
  genStringToEnum(handler_name, enum_css_object_json)
  genStatements(handler_name, enum_css_object_json)
  genEnum(handler_name, enum_css_object_json)
  genJavaEnum(handler_name, enum_css_object_json)
  genOCEnum(handler_name, enum_css_object_json)
  genEnumToStringHeader(handler_name, enum_css_object_json)
  genEnumToStringSource(handler_name, enum_css_object_json)
