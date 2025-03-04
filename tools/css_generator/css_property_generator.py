# Copyright 2019 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

#!/usr/bin/python
# -*- coding: UTF-8 -*-
import json
import string
import utils


def gen(json_obj):
  define_template = string.Template("""#define FOREACH_ALL_PROPERTY(V)  \\${css}""")
  css_template = string.Template("""
  V(${name}, "${css_name}", "${default}")  \\""")
  css_last_tempate = string.Template("""
  V(${name}, "${css_name}", "${default}")
""")
  css_java_template = string.Template("""
  public static final int ${name} = ${id};
""")
  css_str = ""
  css_java_str = ""
  css_java_constant_str = """
    public static final String[] PROPERTY_CONSTANT = new String[] { 
      "AUTO_INSERTED_BEGIN",
      //-------------------
  """
  css_java_constant_str_template = string.Template("""
    "${css_name}",""")

  i = 0
  for val in json_obj:
    template = css_template
    if (i == len(json_obj) - 1):
      template = css_last_tempate
    css_str += template.substitute(name=utils.underline2hump(
        val['name']), css_name=val['name'], default=val['default_value'])
    i = i + 1
    css_java_str += css_java_template.substitute(name=utils.underline2hump(val['name']), id=val['id'] + 1)
    css_java_constant_str += css_java_constant_str_template.substitute(css_name=val['name'])
  
  css_java_constant_str += """
      //-------------------
      "AUTO_INSERTED_END",
    };
  """
  return define_template.substitute(css=css_str), css_java_str, css_java_constant_str

def genCSSProperty(json_obj):
  css_properties, css_java_properties, css_java_constant_properties = gen(json_obj)
  utils.genPropID(utils.getCSSPropertyPath(), css_properties)
  utils.genTest(utils.getCSSPropertyTestPath(), json_obj)
  # utils.fillTemplate(utils.getJavaCSSPropertyPath(), css_java_properties)

  utils.genJavaPropId(utils.getJavaCSSPropertyPath(), css_java_properties, css_java_constant_properties)
