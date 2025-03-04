#!/usr/bin/env python3
# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

# """
# This script generate podspec scripts from the gn podspec_target targets.
# """

import argparse
import json
import logging
import os
import posixpath
import sys
import re

# Must be aligned with the out_gen_path of the podspec_target template in the
# tools/gn_tools/podspec_target_template.gni file. 
PROJECT_FILE_NAME = "podspec_targets"

SPEC_HEAD_LENGTH = 30

PODFILE_LICENSE = '''# Copyright 2019 The Lynx Authors. All rights reserved. 
# Licensed under the Apache License Version 2.0 that can be found in the 
# LICENSE file in the root directory of this source tree.
# coding: utf-8
'''

# The platforms currently supported by Cocoapods
pod_support_platforms = [
  'ios',
  'osx',
  'macos',
  'tvos'
]

# The parameters added by starting with a colon
should_use_colon_specification = [
  'source',
  'license',
  'scheme'
]

class Project:
  def __init__(self, project_json):
    self.targets = project_json['targets']
    self.build_settings = project_json['build_settings']
    self.root_path = self.build_settings['root_path']
    self.build_path = os.path.join(self.root_path,
                                     self.build_settings['build_dir'][2:])

class Metadata:
  def __init__(self, properties, type):
    metadata = properties.get('metadata', {})
    if type.__name__ == 'PodspecTarget':
      # podspec target metadata
      self.condition_deps = metadata.get("condition_deps", [])
      self.global_variables = metadata.get("global_variables", [])
      self.output_name = self.get_first_var(metadata.get("output_name", []), "")
      self.output_path = self.get_first_var(metadata.get("output_path", []), "")
      self.root_specification = self.format_root_specification(self.get_first_var(metadata.get("root_specification", []), {}))
    elif type.__name__ == 'SubspecTarget':
      # subspec target metadata
      self.condition_deps = metadata.get("condition_deps", [])
      self.output_name = self.get_first_var(metadata.get("output_name", []), "")
      self.test_subspec = self.get_first_var(metadata.get("test_subspec", []), False)
      self.header_mappings_dir = self.get_first_var(metadata.get("header_mappings_dir", []), "")
      self.vendored_frameworks = metadata.get("vendored_frameworks", [])
      self.vendored_libraries = metadata.get("vendored_libraries", [])
      self.compiler_flags = metadata.get("compiler_flags", [])
      self.public_header_files = metadata.get("public_header_files", [])
      self.header_dir = self.get_first_var(metadata.get("header_dir", []), "")
      self.frameworks = metadata.get("frameworks", [])
      self.libraries = metadata.get("libraries", [])
      self.resource_bundles = metadata.get("resource_bundles", [])
      self.dependency = metadata.get("dependency", [])
      self.pattern_source_files = metadata.get("pattern_source_files", [])
      self.pattern_exclude_files = metadata.get("pattern_exclude_files", [])
      self.pod_target_xcconfig = self.get_first_var(metadata.get("pod_target_xcconfig", []), {})
      self.is_subspec_target = self.get_first_var(metadata.get("is_subspec_target", []), False)
      self.flatten_deps = metadata.get("flatten_deps", [])
  
  def get_first_var(self, var_list, default_val):
    return var_list[0] if len(var_list) > 0 else default_val

  def format_root_specification(self, root_specification):
    platform_names = set()
    for key in root_specification.keys():
      if key in pod_support_platforms:
        platform_names.add(key)
    for platform in platform_names:
      platform_args = root_specification.pop(platform)
      for arg_key in platform_args:
        arg_new_name = "%s.%s" % (platform, arg_key)
        root_specification[arg_new_name] = platform_args[arg_key]
    return root_specification

class PodspecTarget:
  """
  The podspec object corresponding to the gn target
  """
  def __init__(self, gn_target_name, project):
    self.gn_name = gn_target_name
    self.project = project
    self.properties = project.targets[self.gn_name]
    metadata = Metadata(self.properties, type(self))

    self.deps = sorted(self.properties.get("deps", []))
    self.condition_deps = metadata.condition_deps
    self.global_variables = metadata.global_variables
    self.output_name = metadata.output_name
    self.output_path = metadata.output_path
    self.root_specification = metadata.root_specification

SubspecTargetExcludeFlattenVar = [
  'gn_name',
  'project',
  'properties',
  'output_name',
  'test_subspec',
  'flatten_deps'
]

SubspecTargetFlattenStringVar = [
  'header_dir',
  'header_mappings_dir'
]

class SubspecTarget:
  """
  The subspec object corresponding to the gn target
  """
  def __init__(self, gn_target_name, project):
    if gn_target_name not in project.targets:
      raise Exception("%s is an illegal gn target for generate subspec" % gn_target_name)
    self.gn_name = gn_target_name
    self.project = project
    self.properties = project.targets[self.gn_name]
    metadata = Metadata(self.properties, type(self))
    self.deps = sorted(self.properties.get("deps", []))
    self.source_files = self.format_root_path(self.properties.get("sources", []) + metadata.pattern_source_files)
    self.exclude_files = self.format_root_path(self.properties.get("exclude_sources", []) + metadata.pattern_exclude_files)
    self.output_name = metadata.output_name
    self.test_subspec = metadata.test_subspec
    self.vendored_frameworks = self.format_root_path(metadata.vendored_frameworks)
    self.vendored_libraries = self.format_root_path(metadata.vendored_libraries)
    self.public_header_files = self.format_root_path(metadata.public_header_files)
    self.header_dir = metadata.header_dir
    self.header_mappings_dir = metadata.header_mappings_dir
    self.frameworks = metadata.frameworks
    self.libraries = metadata.libraries
    self.dependency, self.dependency_versions = self.parse_dependency(metadata.dependency)
    self.compiler_flags = metadata.compiler_flags
    self.resource_bundles = self.get_resource_bundles()
    self.pod_target_xcconfig = metadata.pod_target_xcconfig
    self.subspec_targets_list = self.find_all_dependencies()
    self.condition_deps = metadata.condition_deps
    self.private_header_files = self.get_private_header_files()
    self.flatten_deps = self.get_flatten_deps(metadata.flatten_deps)

  def format_root_path(self, paths):
    if len(paths) <= 0:
      return paths
    return sorted([path.replace('//', '') for path in paths])

  def string_to_int(self, version_str):
    version_int = re.findall(r'\d+', version_str)
    return int("".join(version_int))

  def parse_dependency(self, dependencies):
    dependency_list = set()
    dependency_versions = {}
    for dependency in dependencies:
      if isinstance(dependency, list):
        dependency_name = dependency[0]
        dependency_version = dependency[1]
        dependency_list.add(dependency_name)
        if len(dependency) == 2:
          if dependency_name in dependency_versions.keys():
            # If the version number of the dependency already exists, then keep the larger one. 
            new_version_int = self.string_to_int(dependency_version)
            old_version_int = self.string_to_int(dependency_versions[dependency_name])
            if new_version_int > old_version_int:
              dependency_versions[dependency_name] = dependency_version
          else:
            dependency_versions.update({dependency_name: dependency_version})
      else:
        dependency_list.add(dependency)
    return dependency_list, dependency_versions

  def find_all_dependencies(self):
    subspec_targets_set = set()
    for dep in self.deps:
      dep_target = self.project.targets[dep]
      metadata = Metadata(dep_target, type(self))
      if metadata.is_subspec_target:
        subspec_targets_set.add(dep)
      else:
        self.source_files.extend(self.format_root_path(dep_target.get('sources', [])))
        self.source_files = list(set(self.source_files))
        self.source_files.sort()
    if len(subspec_targets_set) > 0:
      subspec_targets_list = sorted(subspec_targets_set)
      return subspec_targets_list
    else: 
      return []

  def get_resource_bundles(self):
    resource_bundles = {}
    resource_bundles_raw = Metadata(self.properties, type(self)).resource_bundles
    if len(resource_bundles_raw) == 0:
      return resource_bundles
    for resource_bundle_name in resource_bundles_raw:
      resource_bundle = self.project.targets[resource_bundle_name]
      sources = self.format_root_path(resource_bundle.get('sources', []))
      name = resource_bundle_name.split(':')[1]
      resource_bundles[name] = sources
    return resource_bundles


  def get_private_header_files(self):
    sources = self.source_files
    public_headers = self.public_header_files
    test_subspec = self.test_subspec

    public_paths = set()
    for header in public_headers:
      path = os.path.dirname(header)
      public_paths.add(path)
    private_paths = set()
    private_headers = []
    for source in sources:
      if not (source.endswith('.h') or source.endswith('.hpp')):
        continue
      if source in public_headers:
        continue
      private_headers.append(source)
    private_headers.sort()
    return private_headers

  def get_flatten_deps(self, flatten_deps):
    flatten_targets_set = set()
    flatten_deps = sorted(flatten_deps)
    for dep in flatten_deps:
      dep_target = self.project.targets[dep]
      metadata = Metadata(dep_target, type(self))
      if metadata.is_subspec_target:
        flatten_targets_set.add(dep)
      else:
        exception_msg = "%s is an illegal gn target for %s to flatten deps, you can only depend on the target of the subspec_target type." % (dep, self.gn_name)
        raise Exception(exception_msg)
    if len(flatten_targets_set) > 0:
      self.flatten_subspec_targets(flatten_targets_set)
      return flatten_targets_set
    else: 
      return []

  def add_flatten_subspec_parm(self, flatten_value, para_name):
    if len(flatten_value) <= 0:
      return
    self_var = getattr(self, para_name)
    if len(self_var) <= 0:
      setattr(self, para_name, flatten_value)
      return
    var_add = set(flatten_value) | set(self_var)
    setattr(self, para_name, sorted(var_add))
  
  def para_competition(self, flatten_value, para_name):
    self_var = getattr(self, para_name)
    if self_var == '':
      setattr(self, para_name, flatten_value)

  def add_dict_params(self, param_name, flatten_value, compare_lambda, default_value):
    if flatten_value == {}:
      return 
    var = getattr(self, param_name)
    if var == {}:
      setattr(self, param_name, flatten_value)
      return
    merged_dict = {}
    for key in var.keys() | flatten_value.keys():
      value1 = var.get(key, default_value)
      value2 = flatten_value.get(key, default_value)
      merged_dict[key] = compare_lambda(value1, value2)
    setattr(self, param_name, merged_dict)

  def flatten_subspec_targets(self, subspec_targets_set):
    for subspec_target in subspec_targets_set:
      flatten_sub = SubspecTarget(subspec_target, self.project)
      for key, value in vars(flatten_sub).items():
        if key == 'pod_target_xcconfig':
          self.add_dict_params(key, value, lambda v1, v2: set(v1) |set (v2), [])
          continue
        if key == 'dependency_versions':
          self.add_dict_params(key, value, lambda v1, v2: v1 if self.string_to_int(v1) > self.string_to_int(v2) else v2, 1000000)
          continue
        if key in SubspecTargetFlattenStringVar:
          self.para_competition(value, key)
          continue
        if key not in SubspecTargetExcludeFlattenVar:
          self.add_flatten_subspec_parm(value, key)

class Writer:
  """
  Format content and then write formatted content to .podspec file
  """
  def __init__(self, target):
    self.project = target.project
    path = os.path.join(target.output_path, target.output_name)
    print("generate podspec file: %s" % path)
    self.out = open(path, 'w+')

  def get_pod_header(self, level, test_subspec=False):
    header = 's' * level + 'p'
    if test_subspec:
      header = 'test_spec'
    return header

  def format_head_str(self, str, level):
    space = self.get_pre_spaces(level)
    head_str = '%s%s' % (space, str)
    head_str = head_str.ljust(SPEC_HEAD_LENGTH + len(space), ' ')
    return head_str

  def format_include_headers(self, include_dirs):
    format_includes = []
    for include in include_dirs:
      dir = ''
      if include.startswith('//PODS_ROOT'):
        dir = include.replace('//PODS_ROOT', '${PODS_ROOT}')
      elif include.startswith('//TARGET_BUILD_DIR'):
        dir = include.replace('//TARGET_BUILD_DIR', '${TARGET_BUILD_DIR}')
      elif include.startswith('//PODS_TARGET_SRCROOT'):
        dir = include.replace('//PODS_TARGET_SRCROOT', '${PODS_TARGET_SRCROOT}')
      else:
        dir = include.replace('//', '${PODS_TARGET_SRCROOT}/')
      dir_str = '\\\"' + dir + '\\\"'
      format_includes.append(dir_str)
    format_includes.sort()
    return format_includes

  def get_pre_spaces(self, level):
    return '  ' * level

  def write_header(self):
    self.out.write(PODFILE_LICENSE)

  def write_global_variables(self, global_variables):
    if global_variables == None or len(global_variables) == 0:
      self.out.write('\n')
      return
    self.out.write('\n')
    for variable in global_variables:
      self.out.write(variable)
      self.out.write('\n')
    self.out.write('\n')

  def write_podspec_title(self, level):
    self.out.write('Pod::Spec.new do |%s|\n' % self.get_pod_header(level))

  def write_root_spec_str_internal(self, key, symbol, value, level, key_has_colon=False):
    space = self.get_pre_spaces(level)
    title_line = '%s"%s" %s "%s"' % (space, key, symbol, value)
    if key_has_colon:
      title_line = '%s:%s %s "%s"' % (space, key, symbol, value)
    self.out.write(title_line)

  def write_root_spec_list_internal(self, key, symbol, values, level, key_has_colon=False):
    space = self.get_pre_spaces(level)
    title_line = '%s"%s" %s ' % (space, key, symbol)
    if key_has_colon:
      title_line = '%s:%s %s ' % (space, key, symbol)
    self.out.write(title_line)
    title_line_len = len(title_line)
    i = 0
    self.out.write('"')
    sub_space = ' ' * title_line_len
    values_sorted = sorted(values)
    for value in values_sorted:
      i += 1
      if i == 1:
        self.out.write(value)
      else:
        self.out.write(sub_space + value)
      if i < len(values_sorted):
        self.out.write(' \\')
        self.out.write('\n')
    self.out.write('"')

  def write_root_spec_normal(self, header, key, symbol, value, level):
    if isinstance(value, bool):
      value = 'true' if value else 'false'
    key_line = self.format_head_str('%s.%s' % (header, key), level)
    title_line = '%s%s %s' % (key_line, symbol, value)
    self.out.write(title_line)

  def write_root_spec_str(self, header, key, symbol, value, level):
    key_line = self.format_head_str('%s.%s' % (header, key), level)
    title_line = '%s%s "%s"' % (key_line, symbol, value)
    self.out.write(title_line)

  def write_root_spec_list(self, header, key, symbol, values, level, should_new_line=False):
    key_line = self.format_head_str('%s.%s' % (header, key), level)
    title_line = '%s%s ' % (key_line, symbol)
    self.out.write(title_line)
    title_line_len = len(title_line)
    i = 0
    for value in values:
      i += 1
      if should_new_line:
        sub_space = ' ' * title_line_len if i != 1 else ''
        self.out.write('%s"%s"' % (sub_space, value))
      else:
        self.out.write('"%s"' % value)
      if i < len(values):
        self.out.write(', ')
      if should_new_line:
        self.out.write('\n')

  def write_root_spec_dict(self, header, key, symbol, value, level):
    key_line = self.format_head_str('%s.%s' % (header, key), level)
    title_line = '%s%s {\n' % (key_line, symbol)
    self.out.write(title_line)
    symbol_internal = '=>'
    key_has_colon = False
    if key in should_use_colon_specification:
      key_has_colon = True

    i = 0
    dict_key = sorted(value.keys())
    for k in dict_key:
      next_level = level + 1
      i += 1
      value_impl = value[k]
      if k == 'HEADER_SEARCH_PATHS':
        value_impl = self.format_include_headers(value[k])
      if isinstance(value_impl, list) or isinstance(value_impl, set):
        self.write_root_spec_list_internal(k, symbol_internal, value_impl, next_level, key_has_colon)
      else:
        self.write_root_spec_str_internal(k, symbol_internal, value_impl, next_level, key_has_colon)
      if i < len(value):
        self.out.write(',')
      self.out.write('\n')
    
    self.out.write(self.get_pre_spaces(level) +'}')
    return 0

  def write_prepare_command(self, header, key, symbol, values, level):
    key_line = self.format_head_str('%s.%s' % (header, key), level)
    title_line = '%s%s <<-CMD\n' % (key_line, symbol)
    for value in values:
      next_level = level + 1
      title_line += self.get_pre_spaces(next_level) +'%s\n' % (value)
    title_line += self.get_pre_spaces(level) + 'CMD'
    self.out.write(title_line)

  def write_root_specification(self, root_specification, level):
    if len(root_specification) <= 0:
      return 0
    if not isinstance(root_specification, dict):
      raise Exception('root_specification should be a dictionary type. Please check your GN configuration.')
    
    header = self.get_pod_header(level)
    symbol = '='
    next_level = level + 1
    root_specification_key_sorted = sorted(root_specification.keys())
    for key in root_specification_key_sorted:
      value = root_specification[key]
      if(key == 'prepare_command'):
        self.write_prepare_command(header, key, symbol, value, next_level)
        self.out.write('\n')
      else:
        if isinstance(value, dict):
          self.write_root_spec_dict(header, key, symbol, value, next_level)
        elif isinstance(value, list):
          self.write_root_spec_list(header, key, symbol, value, next_level)
        elif isinstance(value, str):
          self.write_root_spec_str(header, key, symbol, value, next_level)
        else:
          self.write_root_spec_normal(header, key, symbol, value, next_level)
        self.out.write('\n')

  def write_subspec_title(self, header, sub_header, spec_name, level, test_subspec):
    space = self.get_pre_spaces(level)
    self.out.write('\n')
    if test_subspec:
      self.out.write('%s%s.test_spec \"%s\" do |%s|\n' % (space, header, spec_name, sub_header))
    else:
      self.out.write('%s%s.subspec \"%s\" do |%s|\n' % (space, header, spec_name, sub_header))

  def write_list_content(self, key, value, header, space):
    list_value = ''
    for v in value:
      list_value += '\"%s\", ' % (v)
    list_value = list_value[:len(list_value)-2]
    full_str = self.format_head_str(key, header, space) + ' = %s \n' %(list_value)
    self.out.write(full_str)

  def write_specification_list(self, header, target, level, specification_name, is_new_line=False):
    var = getattr(target, specification_name)
    if len(var) <= 0:
      return
    self.write_root_spec_list(header, specification_name, '=', var, level, is_new_line)
    self.out.write('\n')

  def write_specification_str(self, header, target, level, specification_name):
    var = getattr(target, specification_name)
    if len(var) <= 0:
      return
    self.write_root_spec_str(header, specification_name, '=', var, level)
    self.out.write('\n')


  def write_xc_configs(self, header, target, level):
    pod_target_xcconfig = target.pod_target_xcconfig
    if len(pod_target_xcconfig) <= 0:
      return
    self.write_root_spec_dict(header, 'pod_target_xcconfig', '=', pod_target_xcconfig, level)
    self.out.write('\n')

  def write_dependencies(self, header, target, level):
    dependencies = target.dependency
    if len(dependencies) == 0:
      return
    dependencies_sorted = sorted(dependencies)
    for dependency in dependencies_sorted:
      key_line = self.format_head_str('%s.%s' % (header, 'dependency'), level)
      self.out.write(key_line)
      if dependency in target.dependency_versions.keys():
        self.out.write('"%s", "%s"' % (dependency, target.dependency_versions[dependency]))
      else:
        self.out.write('"%s"' % dependency)
      self.out.write('\n')

  def write_scheme(self, level, test_subspec):
    if test_subspec:
      header = self.get_pod_header(level, test_subspec)
      self.out.write(self.format_head_str('%s.%s' % (header, 'scheme'), level))
      self.out.write('= { :code_coverage => true }')
      self.out.write('\n')

  def write_bundle_sources(self, resource_name, sources, level):
    space = self.get_pre_spaces(level)
    title_line = '%s"%s" => ' % (space, resource_name)
    self.out.write(title_line + '[\n')
    title_line_len = len(title_line)
    i = 0
    sub_space = ' ' * title_line_len
    for value in sources:
      i += 1
      self.out.write(sub_space + '  "%s"' % value)
      if i < len(sources):
        self.out.write(',')
      self.out.write('\n')
    self.out.write(sub_space + ']')

  def write_bundle_data(self, header, target, level):
    # bundle_data will be converted to resource_bundles
    resource_bundles = target.resource_bundles
    if len(resource_bundles) == 0:
      return
    self.out.write(self.format_head_str('%s.%s' % (header, 'resource_bundles'), level) + '= {\n')
    next_level = level + 1
    i = 0
    for key in resource_bundles.keys():
      self.write_bundle_sources(key, resource_bundles[key], next_level)
      i += 1
      if i < len(resource_bundles.keys()):
        self.out.write(',')
      self.out.write('\n')
    self.out.write(self.get_pre_spaces(level) + '}\n')

  def write_end(self, level):
    space = self.get_pre_spaces(level)
    self.out.write('%send\n' % space)


  def write_single_subspec(self, subspec_target, level):
    """
    Use arguments to assemble a complete subspec.
    This method will recursively convert the gn target to subspec.
    """
    test_subspec = subspec_target.test_subspec
    parent_header = self.get_pod_header(level - 1)
    next_level = level + 1
    cur_header = self.get_pod_header(level, test_subspec)

    subspec_name = subspec_target.output_name
    
    self.write_subspec_title(parent_header, cur_header, subspec_name, level, test_subspec)
    self.write_specification_list(cur_header, subspec_target, next_level, 'compiler_flags')
    self.write_specification_list(cur_header, subspec_target, next_level, 'frameworks')
    self.write_specification_str(cur_header, subspec_target, next_level, 'header_mappings_dir')
    self.write_specification_str(cur_header, subspec_target, next_level, 'header_dir')
    self.write_specification_list(cur_header, subspec_target, next_level, 'libraries')
    self.write_specification_list(cur_header, subspec_target, next_level, 'vendored_libraries', True)
    self.write_specification_list(cur_header, subspec_target, next_level, 'vendored_frameworks', True)
    self.write_specification_list(cur_header, subspec_target, next_level, 'public_header_files', True)
    self.write_specification_list(cur_header, subspec_target, next_level, 'private_header_files', True)
    self.write_specification_list(cur_header, subspec_target, next_level, 'source_files', True)
    self.write_specification_list(cur_header, subspec_target, next_level, 'exclude_files', True)
    self.write_xc_configs(cur_header, subspec_target, next_level)
    self.write_dependencies(cur_header, subspec_target, next_level)
    self.write_scheme(next_level, test_subspec)
    self.write_condition_subspec_targets(subspec_target.condition_deps, next_level)
    self.write_subspec_targets(subspec_target.subspec_targets_list, next_level)
    self.write_bundle_data(cur_header, subspec_target, next_level)
    self.write_end(level)

    return 0
  
  def write_single_condition_subspec(self, subspec_target, condition, level):
    space = self.get_pre_spaces(level)
    self.out.write('%sif $%s==1' % (space, condition))
    self.write_single_subspec(subspec_target, level)
    self.write_end(level)

  def write_subspec_targets(self, subspec_targets, level):
    for subspec_name in subspec_targets:
      subspec = SubspecTarget(subspec_name, self.project)
      self.write_single_subspec(subspec, level)

  def write_condition_subspec_targets(self, condition_deps_list, level):
    if len(condition_deps_list) <= 0:
      return
    for condition_subspec in condition_deps_list:
      subspec_name = condition_subspec[0]
      condition = condition_subspec[1]
      subspec = SubspecTarget(subspec_name, self.project)
      self.write_single_condition_subspec(subspec, condition, level)
  

  def write_podspec_target(self, podspec_target):
    first_level = 0
    second_level = first_level + 1
    self.write_podspec_title(first_level)
    self.write_root_specification(podspec_target.root_specification, first_level)

    self.write_subspec_targets(podspec_target.deps, second_level)
    self.write_condition_subspec_targets(podspec_target.condition_deps, second_level)
    self.write_end(first_level)

def write_podspec(podspec_target):
  if type(podspec_target) != PodspecTarget:
    return -1

  writer = Writer(podspec_target)
  writer.write_header()
  writer.write_global_variables(podspec_target.global_variables)
  writer.write_podspec_target(podspec_target)
  return 0

def gn_to_podspec(project_json, podspec_targets):
  project = Project(project_json)
  r = 0
  for podspec_target_name in podspec_targets:
    if podspec_target_name not in project.targets.keys():
      print("%s is not existed in GN project." % podspec_target_name)
      continue
    podspec_target = PodspecTarget(podspec_target_name, project)
    r |= write_podspec(podspec_target)
  return r


def read_json_file(json_file):
  json_object = {}
  if not os.path.exists(json_file):
    print("The json file %s is not existed." % json_file)
    return json_object
  with open(json_file, "r+") as file:
    json_object = json.loads(file.read())
    file.close()
  return json_object

def get_podspec_targets(podspec_targets_dir):
  lines = []
  for filename in os.listdir(podspec_targets_dir):
    file_path = os.path.join(podspec_targets_dir, filename)
    if os.path.isfile(file_path):
      with open(file_path, "r+") as file:
        lines += file.readlines()
        file.close()
  if len(lines) > 0:
    podspec_targets = [line.replace("\n", "") for line in lines]
  return podspec_targets

def main():
  print("hello, gn to podspec")
  if len(sys.argv) > 1:
    project_json_file = sys.argv[1]
    print(project_json_file)
    project = read_json_file(project_json_file)
    if len(project) <= 0:
      return -1

    gen_root_dir = os.path.dirname(project_json_file)
    # podspec targets
    podspec_targets = []
    if len(sys.argv) > 2:
      podspec_target = sys.argv[2]
      podspec_targets = [podspec_target]
    else:
      error_tips = "You haven't defined the podspec_target in your GN project yet."
      custom_targets_file = os.path.join(gen_root_dir, PROJECT_FILE_NAME)
      podspec_targets = get_podspec_targets(custom_targets_file)
      if len(podspec_targets) <= 0:
        print(error_tips)
        return -1
    print("podspec_targets: ", podspec_targets)
    
    return gn_to_podspec(project, podspec_targets)

  else:
    print("Please pass the json file to the current script to generate the corresponding Podspec files.")
    return -1

if __name__ == "__main__":
  sys.exit(main())