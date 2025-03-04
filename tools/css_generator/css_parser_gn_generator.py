# Copyright 2019 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

#!/usr/bin/python
# -*- coding: UTF-8 -*-
import json
import string
import utils
import os


def genFilenames():
  parser_path = utils.getParserPath()
  files = os.listdir(parser_path)
  s = '  \"parser/'
  new_files = []
  for i in range(len(files)):
    # ignore the unittest files and its build.gn file
    if(files[i].find('unittest') != -1 or files[i].find('BUILD.gn') != -1):
      continue
    new_files.append(s + files[i])
  new_files.sort()
  return '\",\n'.join(new_files) + '\",\n'


def genGNSources():
  gn_path = utils.getHomePath() + "Lynx/css/BUILD.gn"
  # with open(gn_path, 'r') as fp:
  #   lines = []
  #   insert_start_line_number = -1
  #   ignore_line_start = False
  #   for index, line in enumerate(fp):
  #     if "# AUTO INSERT END, DON'T CHANGE IT!" in line:
  #       ignore_line_start = False
  #     if not ignore_line_start:
  #       lines.append(line)
  #     if "# AUTO INSERT, DON'T CHANGE IT!" in line:
  #       insert_start_line_number = index
  #       ignore_line_start = True

  # lines.insert(insert_start_line_number + 1, genFilenames())
  # content = ''.join(lines)
  # with open(gn_path, 'w') as fp:
  #   fp.write(content)
