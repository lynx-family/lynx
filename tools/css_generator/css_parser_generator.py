# Copyright 2019 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

#!/usr/bin/python
# -*- coding: UTF-8 -*-

from importlib import reload
import os
import sys
import re
import string
import json
import utils
import hashlib
import subprocess
import enum_css_generator
import css_property_generator
import css_parser_gn_generator

# TODO(wangyifei.20010605): waiting for python env.
# try:
#     import jsonschema
# except (ImportError, ModuleNotFoundError):
#     subprocess.check_call([sys.executable, "-m", "pip", "install", "jsonschema"])

# from jsonschema  import  validate
import json
from pathlib import Path

reload(sys)

def loadJson(file_path):
    with open(file_path, 'r') as file:
        data = json.load(file)
    return data


def loadCSSDefinesJson(folder_path):
  # Get all json files in the folder.
  json_files = [file for file in os.listdir(folder_path) if file.endswith('.json') and file != 'property_index.json']
  
  result = [{"count": len(json_files)}]

  for json_file in json_files:
      with open(os.path.join(folder_path, json_file), 'r', encoding='utf-8') as f:
          data = json.load(f)
          if 'id' in data and 'name' in data:
            result.append({"id": data['id'], "name": data['name']})
          else:
            print(f"Warning: JSON file {json_file} does not contain 'id' or 'name' key.")
            sys.exit(1)  # interrupt the build process

  # Sort by id in positive order.
  result = [result[0]] + sorted(result[1:], key=lambda x: x['id'])
  
  return result

def check_and_get_defines():
    # Get the intermediate json data
    mid_json = loadCSSDefinesJson('./css_defines')

    # # Write to the mid.json file (for debug)
    # with open(os.path.join('./', 'mid.json'), 'w', encoding='utf-8') as f:
    #     json.dump(mid_json, f, ensure_ascii=False, indent=4)

    with open(os.path.join('./', 'property_index.json'), 'r', encoding='utf-8') as f:
        property_index = json.load(f)

    count = property_index[0]["count"]

    # Check 1: compare the intermediate json with property_index.json
    if mid_json[1:count+1] != property_index[1:count+1]:
        raise ValueError("Failed: the benchmark property_index.json does not match the current state!")

    # Check 2: Check if the ids are incremented by 1 in order
    for i in range(2, len(mid_json)):
        if mid_json[i]["id"] != mid_json[i-1]["id"] + 1:
            name = mid_json[i]["name"]
            raise ValueError(f"Check failed: Invalid id for property '{name}', id must be strictly incremented by 1.")

    # Check 3: Check if the ids are incremented by 1 in order
    name_set = set()
    for entry in mid_json:
        name = entry.get("name")
        if name in name_set:
            raise ValueError(f"Check failed: Duplicate name value '{name}'. ")
        name_set.add(name)

    # Overwrite property_index.json when all check passed
    with open(os.path.join('./', 'property_index.json'), 'w', encoding='utf-8') as f:
        json.dump(mid_json, f, ensure_ascii=False, indent=4)

    # Return combined_data
    combined_data = []
    
    for json_file in os.listdir('./css_defines'):
      file_path = os.path.join('./css_defines', json_file)
      if os.path.isfile(file_path) and json_file.endswith('.json'):
        with open(file_path, 'r', encoding='utf-8') as f:
            data = json.load(f)
            combined_data.append(data)
    combined_data.sort(key=lambda x: x['id'])

    # # Write to the combined_data.json file (for debug)
    # with open(os.path.join('./', 'combined_data.json'), 'w', encoding='utf-8') as f:
    #     json.dump(combined_data, f, ensure_ascii=False, indent=4)
    return combined_data

def sortJson(json_obj):
  sorted_css_objects = {
      'color': [],
      'length': [],
      'time': [],
      'number': [],
      'enum': [],
      'complex': [],
      'string': [],
      'bool': [],
      'border-width': [],
      'border-style': [],
      'timing-function': [],
      'animation-property': []
  }
  for val in json_obj:
    if val['type'] not in list(sorted_css_objects.keys()):
      raise Exception("css type is invalid:" + val['type'])
    sorted_css_objects[val['type']].append(val)
  return sorted_css_objects


def genSource(sorted_css_objects):

  # enum type
  enum_css_generator.genHandler("enum_handler.cc", sorted_css_objects['enum'])
  
  # color type
  utils.genSimpleTypeHandler(utils.getRawParserPath() + "color_handler.h", sorted_css_objects['color'], "color")

  # number type
  utils.genSimpleTypeHandler(utils.getRawParserPath() + "number_handler.h", sorted_css_objects['number'], "number")

  # length type
  utils.genSimpleTypeHandler(utils.getRawParserPath() + "length_handler.h", sorted_css_objects['length'], "length")

  # time type
  utils.genSimpleTypeHandler(utils.getRawParserPath() + "time_handler.h", sorted_css_objects['time'], "time")

  # string type
  utils.genSimpleTypeHandler(utils.getRawParserPath() + "string_handler.h", sorted_css_objects['string'], "string")

  # bool type
  utils.genSimpleTypeHandler(utils.getRawParserPath() + "bool_handler.h", sorted_css_objects['bool'], "bool")

  # border-width type
  utils.genSimpleTypeHandler(utils.getRawParserPath() + "border_width_handler.h",
                        sorted_css_objects['border-width'], "border-width")
  
  # border-style type
  utils.genSimpleTypeHandler(utils.getRawParserPath() + "border_style_handler.h",
                        sorted_css_objects['border-style'], "border-style")

  # timing-function type
  utils.genSimpleTypeHandler(utils.getRawParserPath() + "timing_function_handler.h",
                        sorted_css_objects['timing-function'], "timing-function")

  # animation-property type
  utils.genSimpleTypeHandler(utils.getRawParserPath() + "animation_property_handler.h",
                        sorted_css_objects['animation-property'], "animation-property")
  # complex type
  # for val in sorted_css_objects['complex']:
  #   print(('please complete parser of css property: ' + val['name']))

def checkJsonFileFormat():
  folder_path = Path('./css_defines')

  # with open ('css_define_json_schema/css_define.schema.json') as f:
  with open('css_define_json_schema/css_define_with_doc.schema.json', encoding="utf-8") as f:
      schema = json.load(f)

  # traversal all files in the folder
  for file in folder_path.glob('*.json'):
      print(f'Processing file: {file}')
      with open(file) as g:
          instance_ = json.load(g)
      validate(instance=instance_, schema=schema)
  print("JSON validate success!")

def main_generator(json_obj):
  css_property_generator.genCSSProperty(json_obj)
  genSource(sortJson(json_obj))
  css_parser_gn_generator.genGNSources()

def generate_files(json_obj):
  # TODO(wangyifei.20010605): waiting for python env.
  # checkJsonFileFormat() 
  main_generator(json_obj)

# start execute:
os.chdir(os.path.split(os.path.abspath(__file__))[0])
json_obj = check_and_get_defines()

string_content = json.dumps(json_obj, sort_keys=True)
new_hash = hashlib.sha256(string_content.encode()).hexdigest()
script_dir = os.path.dirname(os.path.abspath(__file__))
hash_define_file = os.path.join(script_dir, 'css_defines_sha256.txt')
# Check if the hash file exists.
# If the hash file does not exist, the hash file is generated and then executed the generate logic.
if not os.path.exists(hash_define_file):
  with open(hash_define_file, 'w') as file:
      file.write(new_hash)
  print("First generate!")    
  generate_files(json_obj)
else:
  with open(hash_define_file, 'r') as file:
      existing_hash = file.read().strip()
  # If the hash file exists, the hash value is first checked to see if it matches the hash value of the source file: "css_defines.json".
  if existing_hash != new_hash:
      # If the hash value of the source file has changed, the hash value in the hash file is overwritten and the generate logic is executed.
      print("css_define.json changed! Generate!")
      generate_files(json_obj)
      with open(hash_define_file, 'w') as file:
          file.write(new_hash)
  # The generate logic is also executed if the generated object file does not exist.
  elif not utils.checkAllFileGenerated():
      print("Generated file not exist! Generate!")
      generate_files(json_obj)
  # If the hash value of the source file has not changed, the generate logic is not executed.
  else:
    print("css_defines.json don't changed!")
