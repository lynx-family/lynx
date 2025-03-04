# Copyright 2023 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
import os
import sys
from base import *

def generate(all_features, code_file):
  if code_file == None:
    print("typescript code file is not specified")
    sys.exit(1)
  print(("typescript feature file: ", code_file))
  
  os.makedirs(os.path.dirname(code_file), exist_ok=True)
  with open(code_file, 'w') as file:
    generate_head(file)
    generate_code(file, all_features)
    generate_footer(file)

def generate_head(file):
  header = """// Copyright 2023 The Lynx Authors. All rights reserved.

//!!!!! DO NOT MODIFY
//!!!!! See `tools/feature_count/README.md`

"""
  file.write(header)

def generate_footer(file):
  footer = """//!!!!! DO NOT MODIFY
//!!!!! See `tools/feature_count/README.md`
"""
  file.write(footer)

def generate_code(file, all_features):
  content = ""
  for info in all_features:
    if info.language == Language.TypeScript:
      enum = info.enum.title().replace("_", "")
      content += "  {0} = {1},\n".format(enum, info.value)
  if content != "":
    # add enum define header
     content = "export const enum LynxFeature {\n" + content
     content += "}\n"
     file.write(content)


