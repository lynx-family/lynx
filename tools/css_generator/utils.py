# Copyright 2019 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

#!/usr/bin/python
# -*- coding: UTF-8 -*-
import re
import string
import os
import sys
from datetime import datetime


def underline2hump(text):
  newstr = list(text)
  for index, ch in enumerate(text):
    if index == 0:
      newstr[index] = ch.upper()
    if (ch == '-'):
      if (index+1 < len(text)):
        newstr[index] = ''
        newstr[index+1] = newstr[index+1].upper()
  return ''.join(newstr)

def hump2underline(text):
    lst = []
    for index, char in enumerate(text):
        if char.isupper() and index != 0:
            lst.append("_")
        lst.append(char)
    return "".join(lst).upper()

def genFilename(css_name):
  return css_name.replace('-', '_') + "_handler"


def genCommonHandlerRegister(css_objects_json):
  property_template = string.Template(
      "  array[kPropertyID${class_name}] = &Handle;\n")
  properties_source = ""
  for val in css_objects_json:
    class_name = underline2hump(val['name'])
    properties_source += property_template.substitute(class_name=class_name)
  return properties_source


def writeToFile(filename, content):
  with open(getParserPath() + filename, 'w+', encoding="utf-8") as f:
    f.write(content)


def getHomePath():
  return '../../'

def getRawParserPath():
  return 'core/renderer/css/parser/'

def getParserPath():
  return getHomePath() + 'core/renderer/css/parser/'

def getCSSStylePath():
  return getHomePath() + "core/renderer/starlight/style/"

# TODO(zhengsenyao): Change to path
def getJavaStyleConstantPath():
  return getHomePath() + "platform/android/lynx_android/src/main/java/com/lynx/tasm/behavior/StyleConstants.java"
def getAutoGenJavaStyleConstantPath():
  return getHomePath() + "platform/android/lynx_android/src/main/java/com/lynx/tasm/behavior/AutoGenStyleConstants.java"
# TODO(zhengsenyao): Change to path
def getOCStyleConstantPath():
  return getHomePath() + "platform/darwin/common/lynx/public/base/LynxCSSType.h"
def getAutoGenOCStyleConstantPath():
  return getHomePath() + "platform/darwin/common/lynx/public/base/LynxAutoGenCSSType.h"
def getCSSPropertyPath():
  return getHomePath() + "core/renderer/css/css_property_id.h"

# TODO(zhengsenyao): Change to path
def getJavaCSSPropertyPath():
  return getHomePath() + "platform/android/lynx_processor/src/main/java/com/lynx/tasm/behavior/PropertyIDConstants.java"

def getCSSDebugHeaderPath():
  return getHomePath() + "core/renderer/css/css_decoder.h"

def getCSSDebugSourcePath():
  return getHomePath() + "core/renderer/css/css_decoder.cc"

def getAutoGenCSSDebugHeaderPath():
  return getHomePath() + "core/renderer/css/auto_gen_css_decoder.h"

def getAutoGenCSSDebugSourcePath():
  return getHomePath() + "core/renderer/css/auto_gen_css_decoder.cc"

def getCSSPropertyTestPath():
  # return getHomePath() + "core/renderer/css/css_property_unittest.cc"
  return getHomePath() + "core/renderer/css/css_property_auto_gen_unittest.cc"

def getCSSTypesPath():
  return getCSSStylePath() + "auto_gen_css_type.h"

def fillHandler(filename, contents, order=0):
  home_path = getParserPath()
  handler_path = home_path + filename
  order_acc = 0
  with open(handler_path, 'r', encoding="utf-8") as fp:
    lines = []
    insert_start_line_number = -1
    ignore_line_start = False
    for index, line in enumerate(fp):
      if "// AUTO INSERT END, DON\'T CHANGE IT!" in line:
        ignore_line_start = False
      if not ignore_line_start:
        lines.append(line)
      if "// AUTO INSERT, DON\'T CHANGE IT!" in line:
        if order_acc == order:
          insert_start_line_number = index
          ignore_line_start = True
        order_acc += 1
  lines.insert(insert_start_line_number + 1, contents)
  content = ''.join(lines)
  with open(handler_path, 'w', encoding="utf-8") as fp:
    fp.write(content)
  formatCode(handler_path)

def genGroupHandler(filename, object_jsons, order=0):
  properties = genCommonHandlerRegister(object_jsons)
  fillHandler(filename, properties, order)

def fillTemplate(handler_path, css_types):
  with open(handler_path, 'r', encoding="utf-8") as fp:
    lines = []
    insert_start_line_number = -1
    ignore_line_start = False
    for index, line in enumerate(fp):
      if "// AUTO INSERT END, DON\'T CHANGE IT!\n" in line:
        ignore_line_start = False
      if not ignore_line_start:
        lines.append(line)
      if "// AUTO INSERT, DON\'T CHANGE IT!\n" in line:
        insert_start_line_number = index
        ignore_line_start = True
  lines.insert(insert_start_line_number + 1, css_types)
  content = ''.join(lines)
  with open(handler_path, 'w', encoding="utf-8") as fp:
    fp.write(content)
  formatCode(handler_path)

def formatCode(file_path):
  os.system("clang-format -i " + file_path)

def generate_macros(file_path):
  macro_name = file_path.upper().replace('/', '_').replace('.', '_')
  ifndef_macro = f"""#ifndef {macro_name}_
  """
  define_macro = f"""#define {macro_name}_
  """
  endif_macro = f"""#endif  // {macro_name}_

"""
  return ifndef_macro, define_macro, endif_macro

def genPropID(path, css_types):
  current_year = datetime.now().year
  copyright = f"""// Copyright {current_year} The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
  """

  prefix = """
#ifndef CORE_RENDERER_CSS_CSS_PROPERTY_ID_H_
#define CORE_RENDERER_CSS_CSS_PROPERTY_ID_H_
// A macro helps to generate property enums and names.
// Warning: You can only add css property into the last line if you want to add
// new css property otherwise it will cause break change! Don't make break
// change!
// AUTO INSERT, DON'T CHANGE IT!
"""
  namespace = """

namespace lynx {
namespace tasm {
  """

  extra_prefix = """
  // This defines the maximum value for CSS Property IDs. The range 0~999 is
  // reserved for CSS Property IDs, while values greater than 999 are used for
  // other IDs, such as built-in attribute IDs. Defining these ID ranges helps
  // prevent conflicts, allowing them to coexist in the map buffer and
  // facilitating future data transmission optimizations.
  #define CSS_PROPERTY_MAX_ID 999
  
  """

  suffix = """

#define DECLARE_PROPERTY_NAME(name, c, value) \
  static constexpr const char kPropertyName##name[] = c;
FOREACH_ALL_PROPERTY(DECLARE_PROPERTY_NAME)
#undef DECLARE_PROPERTY_NAME

enum CSSPropertyID : int32_t {
  kPropertyStart = 0,
#define DECLARE_PROPERTY_ID(name, c, value) kPropertyID##name,
  FOREACH_ALL_PROPERTY(DECLARE_PROPERTY_ID)
#undef DECLARE_PROPERTY_ID
      kPropertyEnd
};
}  // namespace tasm
}  // namespace lynx
#endif  // CORE_RENDERER_CSS_CSS_PROPERTY_ID_H_
// AUTO INSERT END, DON'T CHANGE IT!

"""
  with open(path, 'w', encoding="utf-8") as file:
    file.seek(0,0)
    file.write(copyright)
    file.write(prefix)
    file.write(namespace)
    file.write(extra_prefix)
    file.write(css_types)
    file.write(suffix)

  formatCode(path)


def genJavaPropId(path, css_types, css_java_constant_properties):
  print("genJavaPropId")
  current_year = datetime.now().year
  copyright = f"""// Copyright {current_year} The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
  """

  prefix = """
  package com.lynx.tasm.behavior; 

  // AUTO INSERT, DON'T CHANGE IT!
  public final class PropertyIDConstants { 
  """

  suffix = """
  }
  // AUTO INSERT, DON'T CHANGE IT!

"""

  with open(path, 'w', encoding="utf-8") as file:
    file.seek(0, 0)
    file.write(copyright)
    file.write(prefix)
    file.write(css_java_constant_properties)
    file.write(css_types)
    file.write(suffix)

  formatCode(path)

def checkHandlerGenerated():
  return (os.path.exists(getParserPath() + "color_handler.h") and 
    os.path.exists(getParserPath() + "number_handler.h") and 
    os.path.exists(getParserPath() + "length_handler.h") and 
    os.path.exists(getParserPath() + "time_handler.h") and 
    os.path.exists(getParserPath() + "timing_function_handler.h") and
    os.path.exists(getParserPath() + "animation_property_handler.h") and 
    os.path.exists(getParserPath() + "bool_handler.h") and 
    os.path.exists(getParserPath() + "border_width_handler.h") and
    os.path.exists(getParserPath() + "border_style_handler.h") and
    os.path.exists(getParserPath() + "string_handler.h") )

def checkAllFileGenerated():
  return (checkHandlerGenerated() and
        os.path.exists(getCSSPropertyPath()) and
        os.path.exists(getCSSPropertyTestPath()) and
        os.path.exists(getJavaCSSPropertyPath()) and
        os.path.exists(getCSSTypesPath()) and
        os.path.exists(getAutoGenOCStyleConstantPath()) and
        os.path.exists(getAutoGenJavaStyleConstantPath()) and
        os.path.exists(getAutoGenCSSDebugHeaderPath()) and
        os.path.exists(getAutoGenCSSDebugSourcePath()) )

def genTest(path, json_obj):
  current_year = datetime.now().year
  copyright = f"""// Copyright {current_year} The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
  """

  prefix = """
// AUTO INSERT, DON'T CHANGE IT!

#include "core/renderer/css/css_property.h"
#include "core/renderer/css/css_property_id.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace test {

TEST(CSSProperty, Order) {
  EXPECT_EQ(kPropertyStart, 0);
"""

  suffix = """  }

  }  // namespace test
  }  // namespace tasm
  }  // namespace lynx

  // AUTO INSERT END, DON'T CHANGE IT!

"""
  define_template = string.Template("""  EXPECT_EQ(kPropertyID${name}, ${order});\n""")
  test_str = ""
  for val in json_obj:
    test_str += define_template.substitute(name=underline2hump(
        val['name']), order=val['id'] + 1)
  test_str += "  EXPECT_EQ(kPropertyEnd, " + str(len(json_obj) + 1) + ");\n"
  with open(path, 'w', encoding="utf-8") as file:
    file.seek(0,0)
    file.write(copyright)
    file.write(prefix)
    file.write(test_str)
    file.write(suffix)
  formatCode(path)

def genCSSType(path, css_types):
  current_year = datetime.now().year
  copyright = f"""// Copyright {current_year} The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
  """

  prefix = """
// AUTO INSERT, DON'T CHANGE IT!

#ifndef CORE_RENDERER_STARLIGHT_STYLE_AUTO_GEN_CSS_TYPE_H_
#define CORE_RENDERER_STARLIGHT_STYLE_AUTO_GEN_CSS_TYPE_H_

#include <limits.h>
#include <stdint.h>

namespace lynx {
namespace starlight {
  

"""

  suffix = """

 }  // namespace starlight
 }  // namespace lynx

#endif  // CORE_RENDERER_STARLIGHT_STYLE_AUTO_GEN_CSS_TYPE_H_

// AUTO INSERT END, DON'T CHANGE IT!

"""

  with open(path, 'w', encoding="utf-8") as file:
    file.seek(0,0)
    file.write(copyright)
    file.write(prefix)
    file.write(css_types)
    file.write(suffix)
  formatCode(path)

def genOCLynxCSSType(path, css_types):
  current_year = datetime.now().year
  copyright = f"""// Copyright {current_year} The Lynx Authors. All rights reserved.
  // Licensed under the Apache License Version 2.0 that can be found in the
  // LICENSE file in the root directory of this source tree.
    """
  prefix = """
  #ifndef DARWIN_COMMON_LYNX_BASE_AUTO_GEN_LYNXCSSTYPE_H_
  #define DARWIN_COMMON_LYNX_BASE_AUTO_GEN_LYNXCSSTYPE_H_

  #import <Foundation/Foundation.h>

  NS_ASSUME_NONNULL_BEGIN
  """

  suffix = """
  NS_ASSUME_NONNULL_END

  #endif  // DARWIN_COMMON_LYNX_BASE_AUTO_GEN_LYNXCSSTYPE_H_

"""

  with open(path, 'w', encoding="utf-8") as file:
    file.seek(0,0)
    file.write(copyright)
    file.write(prefix)
    file.write(css_types)
    file.write(suffix)
  formatCode(path)

def genJavaStyleConstants(path, source):
  current_year = datetime.now().year
  copyright = f"""// Copyright {current_year} The Lynx Authors. All rights reserved.
  // Licensed under the Apache License Version 2.0 that can be found in the
  // LICENSE file in the root directory of this source tree.
    """
  prefix = """package com.lynx.tasm.behavior;

import androidx.annotation.IntDef;

/**
 * auto generator by css generator.
 * Please don't modify manually
 * should equals to css_type.h's defines.
 */
public interface AutoGenStyleConstants {
 
 """
  suffix = """
  }

"""
  with open(path, 'w', encoding="utf-8") as file:
    file.seek(0,0)
    file.write(copyright)
    file.write(prefix)
    file.write(source)
    file.write(suffix)
  formatCode(path)

def genAutoGenCSSDecoderHeader(path, source):
  current_year = datetime.now().year
  copyright = f"""// Copyright {current_year} The Lynx Authors. All rights reserved.
  // Licensed under the Apache License Version 2.0 that can be found in the
  // LICENSE file in the root directory of this source tree.

    """
  
  prefix = """#ifndef CORE_RENDERER_CSS_AUTO_GEN_CSS_DECODER_H_
#define CORE_RENDERER_CSS_AUTO_GEN_CSS_DECODER_H_

#include <string>

#include "core/renderer/css/css_property.h"
#include "core/renderer/css/css_value.h"
#include "core/renderer/starlight/style/css_type.h"

namespace lynx {
namespace tasm {

class AutoGenCSSDecoder {
 public:
  // AUTO INSERT, DON'T CHANGE IT!
  
  """

  suffix = """

  // AUTO INSERT END, DON'T CHANGE IT!
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_CSS_AUTO_GEN_CSS_DECODER_H_

"""
  with open(path, 'w', encoding="utf-8") as file:
    file.seek(0,0)
    file.write(copyright)
    file.write(prefix)
    file.write(source)
    file.write(suffix)
  formatCode(path)

def genAutoGenCSSDecoderSource(path, source):
  current_year = datetime.now().year
  copyright = f"""// Copyright {current_year} The Lynx Authors. All rights reserved.
  // Licensed under the Apache License Version 2.0 that can be found in the
  // LICENSE file in the root directory of this source tree.

    """
  prefix = """#include "core/renderer/css/auto_gen_css_decoder.h"

#include <sstream>
#include <utility>

#include "core/renderer/css/css_color.h"
#include "core/runtime/vm/lepus/array.h"
#include "core/runtime/vm/lepus/table.h"
#include "core/style/timing_function_data.h"
#include "core/style/transform_raw_data.h"
#include "base/include/debug/lynx_assert.h"

namespace lynx {
namespace tasm {

using namespace lynx::starlight;  // NOLINT
using namespace lynx::tasm;       // NOLINT
// AUTO INSERT, DON'T CHANGE IT!

"""

  suffix = """
  
// AUTO INSERT END, DON'T CHANGE IT!

}  // namespace tasm
}  // namespace lynx

"""
  with open(path, 'w', encoding="utf-8") as file:
    file.seek(0,0)
    file.write(copyright)
    file.write(prefix)
    file.write(source)
    file.write(suffix)
  formatCode(path)

def genSimpleTypeHandler(raw_path, object_json, type):
  source = genCommonHandlerRegister(object_json)

  current_year = datetime.now().year
  copyright = f"""// Copyright {current_year} The Lynx Authors. All rights reserved.
  // Licensed under the Apache License Version 2.0 that can be found in the
  // LICENSE file in the root directory of this source tree.

    """
  custom_content_switcher = {
    "color": ("""
namespace ColorHandler {
    """,
    """
}  // namespace ColorHandler
    """,
    """
  // AUTO INSERT END, DON'T CHANGE IT!
}

bool Process(const lepus::Value &input, CSSValue &css_value,
             const CSSParserConfigs &configs, bool is_text_color = false);
    """),

    "number": ("""
namespace NumberHandler {
    """,
    """
}  // namespace NumberHandler
    """,
    """
  // AUTO INSERT END, DON'T CHANGE IT!
}

    """),

    "length": ("""
namespace LengthHandler {
    """,
    """
}  // namespace LengthHandler
    """,
    """
  // AUTO INSERT END, DON'T CHANGE IT!
}

bool Handle(CSSPropertyID key, const lepus::Value &input, StyleMap &output,
            const CSSParserConfigs &configs);
// help parse length css
bool Process(const lepus::Value &input, CSSValue &css_value,
             const CSSParserConfigs &configs);

    """),

    "time": ("""
namespace TimeHandler {
    """,
    """
}  // namespace TimeHandler 
    """,
    """
  // AUTO INSERT END, DON'T CHANGE IT!
}

    """),

    "string": ("""
namespace StringHandler {
    """,
    """
}  // namespace StringHandler 
    """,
    """
  // AUTO INSERT END, DON'T CHANGE IT!
}

    """),

    "bool": ("""
namespace BoolHandler {
    """,
    """
}  // namespace BoolHandler 
    """,
    """
  // AUTO INSERT END, DON'T CHANGE IT!
}

bool Process(const lepus::Value& input, CSSValue& css_value,
             const CSSParserConfigs& configs, CSSPropertyID key);
    """),

    "border-width": ("""
namespace BorderWidthHandler {
    """,
    """
}  // namespace BorderWidthHandler 
    """,
    """
  // AUTO INSERT END, DON'T CHANGE IT!
}

    """),

    "border-style": ("""
namespace BorderStyleHandler {
    """,
    """
}  // namespace BorderStyleHandler 
    """,
    """
  // AUTO INSERT END, DON'T CHANGE IT!
}

    """),

    "timing-function": ("""
namespace TimingFunctionHandler {
    """,
    """
}  // namespace TimingFunctionHandler 
    """,
    """
  // AUTO INSERT END, DON'T CHANGE IT!
}

    """),

    "animation-property": ("""
namespace AnimationPropertyHandler {
    """,
    """
}  // namespace AnimationPropertyHandler 
    """,
    """
  // AUTO INSERT END, DON'T CHANGE IT!
}

    """),
  }
  length_handler_header = """"""
  if type == "border-width":
    length_handler_header = """
#include "core/renderer/css/parser/length_handler.h"
    """

  general_prefix = """
#include "core/renderer/css/parser/handler_defines.h"

namespace lynx {
namespace tasm {"""
  
  general_suffix = """}  // namespace tasm
}  // namespace lynx

"""
  prefix_declare = """
  
HANDLER_DECLARE();

HANDLER_REGISTER_IMPL_INL() {
  // AUTO INSERT, DON'T CHANGE IT!
  """

  ifndef_macro, define_macro, endif_macro = generate_macros(raw_path)

  prefix_namespace, suffix_namespace, custom_function = custom_content_switcher.get(type, ("default", "default", "default"))
  if prefix_namespace == "default" or suffix_namespace == "default":
    prefix_namespace = """bad type"""
    suffix_namespace = """bad type"""
    custom_function = """bad type"""

  prefix = ifndef_macro + define_macro + length_handler_header + general_prefix + prefix_namespace + prefix_declare
  suffix = custom_function + suffix_namespace + general_suffix + endif_macro
  path = getHomePath() + raw_path
  with open(path, 'w', encoding="utf-8") as file:
    file.seek(0,0)
    file.write(copyright)
    file.write(prefix)
    file.write(source)
    file.write(suffix)
  formatCode(path)
