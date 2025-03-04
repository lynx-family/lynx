# Copyright 2023 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import os
import re
import yaml
from enum import Enum

dir_name = os.path.dirname(os.path.abspath(__file__))
kFeatureUnknownValue = -1

class Language(Enum):
  Unknown = "Unknown"
  Cpp = "cpp"
  TypeScript = "typescript"
  Java = "java"
  Objc = "objc"

class Feature:
    def __init__(self, enum, language, value):
        self.enum = enum
        self.value = int(value)
        self.name = enum.lower()
        if isinstance(language, Language):
          self.language = language
        else:
          for l in Language:
            if l.value == language:
              self.language = l
              break
    
    def isEmpty(self):
      return self.enum == "" and self.language == Language.Unknown and self.name == ""
    
    # reset Feature.name and Feature.language, keep Feature.value
    def reset(self):
      self.enum = ""
      self.language = Language.Unknown
      self.name = ""

def createEmptyFeature(value):
  return Feature("", Language.Unknown, value)

# ******************************* process feature.h *******************************

# return [
#   Feature("feature_1", Language.Cpp, 1),
#   Feature("feature_3", Language.Java, 3),
# ]
def get_last_features_frome_cxx_feature_h():
  all_features = []
  code_file = os.path.abspath(os.path.join(dir_name, "../../core/services/feature_count/feature.h"))
  with open(code_file, 'r') as file:
    code = file.read()
    pattern = r"(([A-Z]+)\w+)\s*=\s*(\d+),"
    matches = re.findall(pattern, code)
    for match in matches:
      feature = Feature(match[0],  match[1].lower(),  match[2]);
      all_features.append(feature)
    return all_features

# before: [
#   Feature("feature_1", Language.Cpp, 1),
#   Feature("feature_3", Language.Java, 3),
# ]
#
# after: [
#   Feature("", Language.Unknown, 0),     <- Add
#   Feature("feature_1", Language.Cpp, 1),
#   Feature("", Language.Unknown, 2),     <- Add
#   Feature("feature_3", Language.Java, 3),
# ]
def add_empty_feature_to_last_features(last_features):
  if len(last_features) == 0:
    return last_features
  
  new_features = []
  first_feature = last_features[0]
  if first_feature.value > 0:
    for i in range(first_feature.value):
      new_features.append(createEmptyFeature(i))

  for index in range(len(last_features)):
    feature = last_features[index]
    new_features.append(feature)
    next_index = index + 1
    if next_index != len(last_features):
      next_feature = last_features[next_index]
      next_value = feature.value + 1
      if next_value != next_feature.value:
        for i in range(next_value, next_feature.value):
          new_features.append(createEmptyFeature(i))
          pass
        pass
      pass
    pass
  return new_features

# Check index == feature.value
def check_elements(last_features):
  for index in range(len(last_features)):
    feature = last_features[index]
    assert feature.value == index

  # last_features: [
  #   Feature("", Language.Unknown, 0),    
  #   Feature("feature_1", Language.Cpp, 1),
  #   Feature("", Language.Unknown, 2),    
  #   Feature("feature_3", Language.Java, 3),
  # ]
def get_last_features():
  # 1. 
  # last_features: [
  #   Feature("feature_1", Language.Cpp, 1),
  #   Feature("feature_3", Language.Java, 3),
  # ]
  last_features = get_last_features_frome_cxx_feature_h()

  # 2.
  # last_features: [
  #   Feature("", Language.Unknown, 0),     <- Add
  #   Feature("feature_1", Language.Cpp, 1),
  #   Feature("", Language.Unknown, 2),     <- Add
  #   Feature("feature_3", Language.Java, 3),
  # ]
  last_features = add_empty_feature_to_last_features(last_features)

  # 3.
  # Check index == feature.value
  check_elements(last_features)
  return last_features

# ******************************* process specification.yaml *******************************

def check_sections_valid(sections):
  if sections != None:
    assert isinstance(sections, list)
    assert len(sections) > 0

def check_spec_valid(spec):
  check_sections_valid(spec.get(Language.Cpp.value))
  check_sections_valid(spec.get(Language.TypeScript.value))
  check_sections_valid(spec.get(Language.Objc.value))
  check_sections_valid(spec.get(Language.Java.value))

def check_spec_features_valid(spec_features):
  if len(spec_features) != 0:
    for key,feature in spec_features.items():
      assert isinstance(feature, Feature)
      assert feature.name == key

# 
# if language is Language.Cpp, return value as follow:
# {
#   "feature_1" : Feature("feature_1", Language.Cpp, kFeatureUnknownValue),
#   "feature_3" : Feature("feature_3", Language.Cpp, kFeatureUnknownValue),
# }
def get_language_features(spec,language):
  all_features = {}
  sections = spec.get(language.value)
  if sections != None:
    assert isinstance(sections, list)
    assert len(sections) > 0
    for enum in sections:
      feature = Feature((language.value + "_"+ enum).upper(), language.value, kFeatureUnknownValue)
      all_features[feature.name] = feature
  return all_features
      
# 
# return value as follow:
# {
#   "feature_0" : Feature("feature_0", Language.Objc, kFeatureUnknownValue),
#   "feature_1" : Feature("feature_1", Language.Cpp, kFeatureUnknownValue),
#   "feature_2" : Feature("feature_2", Language.TypeScript, kFeatureUnknownValue),
#   "feature_3" : Feature("feature_3", Language.Java, kFeatureUnknownValue),
# }
def get_all_features_from_spec(spec_file):
  with open(spec_file, 'r') as file:
    spec = yaml.safe_load(file)
    check_spec_valid(spec)
    all_features = {}
    all_features.update(get_language_features(spec, Language.Cpp))
    all_features.update(get_language_features(spec, Language.TypeScript))
    all_features.update(get_language_features(spec, Language.Objc))
    all_features.update(get_language_features(spec, Language.Java))
    check_spec_features_valid(all_features)
    return all_features

# ******************************* process spec + last features *******************************

# 1. reset elements in last_features that are not in spec_features .
# 2. remove elements in spec_features that are not in last_features.
# before:
#   last_features: [
#     Feature("", Language.Unknown, 0),    
#     Feature("feature_1", Language.Cpp, 1),
#     Feature("", Language.Unknown, 2),    
#     Feature("feature_3", Language.Java, 3),
#     Feature("feature_4", Language.Java, 4),
#   ]
#
#   spec_features: {
#     "feature_0" : Feature("feature_0", Language.Objc, kFeatureUnknownValue),
#     "feature_1" : Feature("feature_1", Language.Cpp, kFeatureUnknownValue),
#     "feature_2" : Feature("feature_2", Language.TypeScript, kFeatureUnknownValue),
#     "feature_3" : Feature("feature_3", Language.Java, kFeatureUnknownValue),
#   }
#
# after:
#   last_features: [
#     Feature("", Language.Unknown, 0),    
#     Feature("feature_1", Language.Cpp, 1),
#     Feature("", Language.Unknown, 2),    
#     Feature("feature_3", Language.Java, 3),
#     Feature("", Language.Unknown, 4),     <---- reset Feature.name and Feature.language, keep Feature.value
#   ]
#
#   spec_features: {
#     "feature_0" : Feature("feature_0", Language.Objc, kFeatureUnknownValue),
#     "feature_2" : Feature("feature_2", Language.TypeScript, kFeatureUnknownValue),
#   }
def reset_and_remove_features_if_need(last_features, spec_features):
  for feature in last_features:
    if feature.name in spec_features:
      del spec_features[feature.name]
    else:
      feature.reset()

# 
# last_features: [
#   Feature("", Language.Unknown, 0),    
#   Feature("feature_1", Language.Cpp, 1),
#   Feature("", Language.Unknown, 2),    
#   Feature("feature_3", Language.Java, 3),
#   Feature("", Language.Unknown, 4),
# ]
# spec_features: {
#   "feature_0" : Feature("feature_0", Language.Objc, kFeatureUnknownValue),
#   "feature_2" : Feature("feature_2", Language.TypeScript, kFeatureUnknownValue),
# }
# 
# return new features as follow:
# [  
#   Feature("feature_0", Language.Objc, 0),    
#   Feature("feature_1", Language.Cpp, 1),
#   Feature("feature_2", Language.TypeScript, 2),    
#   Feature("feature_3", Language.Java, 3),
# ]
def merge_to_new_features(last_features, spec_features):
  new_features = []
  for feature in last_features:
    if feature.isEmpty() and (len(spec_features) != 0):
      sf = spec_features.popitem()[1]
      sf.value = feature.value
      new_features.append(sf)
    else:
      new_features.append(feature)
  for key, feature in spec_features.items():
    if len(new_features) == 0:
      feature.value = 0
    else: 
      last_feature = new_features[-1]
      feature.value = last_feature.value + 1
    new_features.append(feature)
  remove_empty_features(new_features)
  return new_features

# Before:
# [
#   Feature("", Language.Unknown, 0),    
#   Feature("feature_1", Language.Cpp, 1),
#   Feature("", Language.Unknown, 2),    
#   Feature("feature_3", Language.Java, 3),
#   Feature("feature_4", Language.Objc, 4),
# ]
# After:
# [  
#   Feature("feature_4", Language.Objc, 0),    
#   Feature("feature_1", Language.Cpp, 1),
#   Feature("feature_3", Language.TypeScript, 2),    
# ]
def remove_empty_features(last_features):
  index = 0
  while index < len(last_features) :
    feature = last_features[index]
    if feature.isEmpty():
      last_feature = last_features.pop()
      while last_feature.value > feature.value and last_feature.isEmpty():
        last_feature = last_features.pop()
      if index != last_feature.value:
        last_feature.value = feature.value
        last_features[index] = last_feature
    index += 1

# return all features as follow:
# [  
#   Feature("feature_0", Language.Objc, 0),    
#   Feature("feature_1", Language.Cpp, 1),
#   Feature("feature_2", Language.TypeScript, 2),    
#   Feature("feature_3", Language.Java, 3),
# ]
def get_all_features(spec_file):
  # 1. 
  # last_features: [
  #   Feature("", Language.Unknown, 0),    
  #   Feature("feature_1", Language.Cpp, 1),
  #   Feature("", Language.Unknown, 2),    
  #   Feature("feature_3", Language.Java, 3),
  #   Feature("feature_4", Language.Java, 4),
  # ]
  last_features = [] # Now we do not need preserve last feature enum value.

  # 2. 
  # {
  #   "feature_0" : Feature("feature_0", Language.Objc, kFeatureUnknownValue),
  #   "feature_1" : Feature("feature_1", Language.Cpp, kFeatureUnknownValue),
  #   "feature_2" : Feature("feature_2", Language.TypeScript, kFeatureUnknownValue),
  #   "feature_3" : Feature("feature_3", Language.Java, kFeatureUnknownValue),
  # }
  spec_features = get_all_features_from_spec(spec_file)
  expected_count = len(spec_features)

  # 3. 
  # last_features: [
  #   Feature("", Language.Unknown, 0),    
  #   Feature("feature_1", Language.Cpp, 1),
  #   Feature("", Language.Unknown, 2),    
  #   Feature("feature_3", Language.Java, 3),
  #   Feature("", Language.Unknown, 4),     <---- reset Feature.name and Feature.language, keep Feature.value
  # ]
  # spec_features: {
  #   "feature_0" : Feature("feature_0", Language.Objc, kFeatureUnknownValue),
  #   "feature_2" : Feature("feature_2", Language.TypeScript, kFeatureUnknownValue),
  # }
  reset_and_remove_features_if_need(last_features, spec_features)

  # 4.
  # new features as follow:
  # [  
  #   Feature("feature_0", Language.Objc, 0),    
  #   Feature("feature_1", Language.Cpp, 1),
  #   Feature("feature_2", Language.TypeScript, 2),    
  #   Feature("feature_3", Language.Java, 3),
  # ]
  all_features = merge_to_new_features(last_features, spec_features)
  assert expected_count == len(all_features)
  return all_features
