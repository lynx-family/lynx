# Copyright 2023 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
import os
import sys
from base import *

def generate(all_features, code_file):
  if code_file == None:
    print("native code file is not specified")
    sys.exit(1)
  print(("native feature file: ", code_file))
  os.makedirs(os.path.dirname(code_file), exist_ok=True)
  with open(code_file, 'w') as file:
    generate_head(file, all_features)
    # print(spec)
    generate_enum_header(file)
    generate_enum_section(file, all_features)
    generate_enum_footer(file)

    generate_to_string_header(file)
    generate_to_string_case(file, all_features)
    generate_to_string_footer(file)

    generate_footer(file)

def generate_head(file, all_features):
  header = """// Copyright 2023 The Lynx Authors. All rights reserved.

#ifndef CORE_SERVICES_FEATURE_COUNT_FEATURE_H_
#define CORE_SERVICES_FEATURE_COUNT_FEATURE_H_

//!!!!! DO NOT MODIFY
//!!!!! See `tools/feature_count/README.md`
#include <cstdint>
namespace lynx {{
namespace tasm {{
namespace report {{
static constexpr const uint32_t kAllFeaturesCount = {0};
""".format(len(all_features))
  file.write(header)

def generate_footer(file):
  footer = """
}  // namespace report
}  // namespace tasm
}  // namespace lynx
//!!!!! DO NOT MODIFY
//!!!!! See `tools/feature_count/README.md`

#endif  // CORE_SERVICES_FEATURE_COUNT_FEATURE_H_
"""
  file.write(footer)


def generate_enum_header(file):
  header = """
enum LynxFeature : uint32_t {
"""
  file.write(header)

def generate_enum_section(file, all_features):
  for feature in all_features:
    file.write("  {0} = {1},\n".format(feature.enum, feature.value))

def generate_enum_footer(file):
  header = """};
"""
  file.write(header)

def generate_to_string_header(file):
  to_string = """
inline const char* LynxFeatureToString(LynxFeature feature) {
  switch (feature) {
"""
  file.write(to_string)

def generate_to_string_case(file, all_features):
  for feature in all_features:
    case_str = """    case {0}: {{
      return "{1}";
    }}
""".format(feature.enum, feature.name)
    file.write(case_str)

def generate_to_string_footer(file):
  to_string = """    default: {
      return "";
    }
  };
}
"""
  file.write(to_string)


