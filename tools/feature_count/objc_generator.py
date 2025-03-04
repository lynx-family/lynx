# Copyright 2023 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import os
import sys
from base import *

def generate(all_features, header_file):
  if header_file == None:
    print("objc code header file is not specified")
    sys.exit(1)
  print(("objc feature header file: ", header_file))

  os.makedirs(os.path.dirname(header_file), exist_ok=True)
  with open(header_file, 'w') as file:
    generate_head(file)
    generate_enum(file, all_features)
    generate_footer(file)

def generate_head(file):
  header = """// Copyright 2023 The Lynx Authors. All rights reserved.

//!!!!! DO NOT MODIFY
//!!!!! See `tools/feature_count/README.md`

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

"""
  file.write(header)

def generate_enum(file, all_features):
  content = ""
  for feature in all_features:
    if feature.language == Language.Objc:
      enum = feature.enum.title().replace("_", "")
      content += "  LynxFeature{0} = {1},\n".format(enum, feature.value)

  if content != "":
    content = "typedef NS_ENUM(uint32_t, LynxFeature) {\n" + content
    content += "};\n"
    file.write(content)

  
def generate_footer(file):
  footer = """NS_ASSUME_NONNULL_END

//!!!!! DO NOT MODIFY
//!!!!! See `tools/feature_count/README.md`
"""
  file.write(footer)
