#!/usr/bin/env python3
# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

# """
# This script generate CMakeLists-impl.cmake from a gn cmake_target target.
# """

import sys
import json
import os
import string
import logging
from pathlib import Path

# Must be aligned with the out_gen_path of the cmake_target template in the
# tools/gn_tools/cmake_target_template.gni file. 
PROJECT_FILE_NAME = "cmake_targets"


def cmake_string_escape(a):
  """Escapes the string 'a' for use inside a CMake string.

  This means escaping
  '\' otherwise it may be seen as modifying the next character
  '"' otherwise it will end the string
  ';' otherwise the string becomes a list

  The following do not need to be escaped
  '#' when the lexer is in string state, this does not start a comment
  """
  return a.replace('\\', '\\\\').replace(';', '\\;').replace('"', '\\"')

class Project:
  def __init__(self, project_json):
    self.targets = project_json['targets']
    self.build_settings = project_json['build_settings']
    self.toolchains = project_json["toolchains"]
    self.root_path = self.build_settings['root_path']
    self.build_path = os.path.join(self.root_path,
                                     self.build_settings['build_dir'][2:])

  def get_absolute_path(self, path):
    if path.startswith("//"):
      return self.root_path + path[1:]
    return path

  def instead_source_path_prefix(self, path):
    return self.get_absolute_path(path).replace(self.root_path, "${ROOT_PATH}")

  def instead_source_path_prefix_list(self, paths_list):
    if len(paths_list) <= 0:
      return []
    return [self.instead_source_path_prefix(path) for path in paths_list]

class CMakeTargetType:
  def __init__(self, command, modifier, property_modifier, is_linkable, is_dependency_barrier):
    self.command = command
    self.modifier = modifier
    self.property_modifier = property_modifier
    self.is_linkable = is_linkable
    self.is_dependency_barrier = is_dependency_barrier
CMakeTargetType.custom = CMakeTargetType('add_custom_target', 'SOURCES',
                                         None, False, False)

# See GetStringForOutputType in gn
cmake_target_types = {
  'unknown': CMakeTargetType.custom,
  'group': CMakeTargetType.custom,
  'executable': CMakeTargetType('add_executable', None, 'RUNTIME', True, True),
  'loadable_module': CMakeTargetType('add_library', 'MODULE', 'LIBRARY', True, True),
  'shared_library': CMakeTargetType('add_library', 'SHARED', 'LIBRARY', True, True),
  'static_library': CMakeTargetType('add_library', 'STATIC', 'ARCHIVE', True, False),
  'source_set': CMakeTargetType('add_library', 'OBJECT', None, False, False),
  'copy': CMakeTargetType.custom,
  'action': CMakeTargetType.custom,
  'action_foreach': CMakeTargetType.custom,
  'bundle_data': CMakeTargetType.custom,
  'create_bundle': CMakeTargetType.custom,
}

cmake_link_flags_tags = {
  'unknown': 'CMAKE_SHARED_LINKER_FLAGS',
  'executable': 'CMAKE_EXE_LINKER_FLAGS',
  'shared_library': 'CMAKE_SHARED_LINKER_FLAGS',
  'static_library': 'CMAKE_STATIC_LINKER_FLAGS',
}

source_file_types = {
  '.cc': 'cxx',
  '.cpp': 'cxx',
  '.cxx': 'cxx',
  '.c': 'c',
  '.s': 'asm',
  '.S': 'asm',
  '.asm': 'asm',
  '.o': 'obj',
  '.obj': 'obj',
}

class Metadata:
  def __init__(self, properties):
    metadata = properties.get('metadata', {})
    self.output_path = self.get_first_var(metadata.get('output_path', []), "")
    self.is_cmake_target = self.get_first_var(metadata.get('is_cmake_target', []), False)
    self.cmake_version = self.get_first_var(metadata.get('cmake_version', []), "")
    self.sub_cmake_target = metadata.get('sub_cmake_target', [])
    self.sub_cmake_target_and_link = metadata.get('sub_cmake_target_and_link', [])
    self.find_and_link_packages = metadata.get('find_and_link_packages', [])
    self.project_name = self.get_first_var(metadata.get('project_name', []), "")
    self.file_name = self.get_first_var(metadata.get('file_name', []), "")
    
  
  def get_first_var(self, var_list, default_val):
    return var_list[0] if len(var_list) > 0 else default_val
    

class Target:
  def __init__(self, gn_name, project):
    if not gn_name in project.targets.keys():
      logging.error('Can not find target %s in your gn scripts' % (gn_name))
    self.gn_name = gn_name
    self.properties = project.targets[self.gn_name]
    self.gn_type = self.properties.get('type', None)
    self.metadata = Metadata(self.properties)
    self.output_name = self.properties.get('output_name', '')
    # gn attributes
    self.asmflags = self.properties.get('asmflags', [])
    self.cflags = self.properties.get('cflags', [])
    self.cflags_c = self.properties.get('cflags_c', [])
    self.cflags_cc = self.properties.get('cflags_cc', [])
    self.configs = self.properties.get('configs', [])
    self.defines = self.properties.get('defines', [])
    self.deps = self.properties.get('deps', [])
    self.include_dirs = self.properties.get('include_dirs', [])
    self.ldflags = self.properties.get('ldflags', [])
    self.lib_dirs = self.properties.get('lib_dirs', [])
    self.libs = self.properties.get('libs', [])
    self.sources = self.properties.get('sources', [])
    self.outputs = self.properties.get('outputs', [])
    self.script = self.properties.get('script', "")
    self.args = self.properties.get('args', [])
    self.response_file_contents = self.properties.get('response_file_contents', [])
    self.declare_path = self.get_declare_path()
    self.dep_actions = set()

    # cmake attributes
    self.file_name = self.metadata.file_name
    self.project_name = self.metadata.project_name
    self.is_cmake_target = self.metadata.is_cmake_target
    self.cmake_name = self.get_cmake_target_name()
    self.cmake_type = cmake_target_types.get(self.gn_type, None)
    self.deps_packages, self.link_modules = self.find_all_deps_packages(self.metadata.find_and_link_packages, project)
    self.output_path = self.metadata.output_path
    self.cmake_version = self.metadata.cmake_version
    self.sub_cmake_target = self.collect_sub_cmake_target()

  def get_declare_path(self):
    module_path = self.gn_name.split(':')[0]
    sub_path = module_path[2:]
    if len(sub_path) > 0:
      module_path += '/'
    return module_path


  def collect_sub_cmake_target(self):
    need_link_sub_cmake_targets = self.metadata.sub_cmake_target_and_link
    for sub in need_link_sub_cmake_targets:
      self.libs.append(Target(sub, project).output_name)
    return need_link_sub_cmake_targets + self.metadata.sub_cmake_target

  def should_check_sources_target(self, target):
    return target.gn_type in ["executable", "loadable_module", "shared_library", "static_library", "source_set", "group"]

  def is_useful_target(self, target):
    if not self.should_check_sources_target(target):
      return True
    useful = False
    if len(target.sources) > 0:
      for source in target.sources:
        if not source.endswith('.h') and not source.endswith('.hpp'):
          useful = True
          break
    return useful

  def get_directly_dependencies(self, project):
    """
    Find directly dependencies of this target
    """
    dep_targets = set()
    for dep in self.deps:
      dep_target = Target(dep, project)
      dep_targets.add(dep_target)
    return dep_targets

  def find_all_dependencies(self, project):
    """
    Find all dependencies starting with this target
    """
    all_deps_source_targets = set()
    all_deps_binary_targets = set()
    has_checked_targets = {}
    has_checked_targets[self.gn_name] = self
    if self.is_useful_target(self):
      all_deps_source_targets.add(self)
    for dep in self.deps:
      dep_target = Target(dep, project)
      if dep_target.gn_type == 'action':
        self.dep_actions.add(dep_target.cmake_name)
      self.recursive_find_dependent_targets(project, dep_target, all_deps_source_targets, all_deps_binary_targets, has_checked_targets)
      self.dep_actions = self.dep_actions.union(dep_target.dep_actions)
    return all_deps_source_targets, all_deps_binary_targets
  
  def recursive_find_dependent_targets(self, project, main_target, all_deps_source_targets, all_deps_binary_targets, has_checked_targets):
    """
    Recursive search dependencies
    """
    # Check whether the current target has been found before to avoid an infinite loop.
    if main_target.gn_name in has_checked_targets.keys():
      main_target.dep_actions = has_checked_targets[main_target.gn_name].dep_actions
      return 0
    else:
      has_checked_targets[main_target.gn_name] = main_target
    # If the current target's type is cmake_target or a dependency barrier, add it to all_deps_binary_targets, and needn't to find its dependencies. 
    # Else, add it to all_deps_source_targets, and recursively find its dependencies.
    if main_target.is_cmake_target or main_target.cmake_type.is_dependency_barrier:
      all_deps_binary_targets.add(main_target)
      return 0
    else:
      if self.is_useful_target(main_target):
        all_deps_source_targets.add(main_target)
        self.add_deps_packages(main_target)
    for dep in main_target.deps:
      dep_target = Target(dep, project)
      if dep_target.gn_type == 'action':
        main_target.dep_actions.add(dep_target.cmake_name)
      r = self.recursive_find_dependent_targets(project, dep_target, all_deps_source_targets, all_deps_binary_targets, has_checked_targets)
      main_target.dep_actions = main_target.dep_actions.union(dep_target.dep_actions)
      if r != 0:
        return r
    return 0

  def find_all_user_defines(self, target_list):
    """
    Find custom macros
    """
    all_defines = set()
    for target in target_list:
      if target.gn_type != 'shared_library':
        defines = target.defines
        all_defines.update(defines)
    return all_defines

  def add_deps_packages(self, target):
    if len(target.deps_packages.keys()) > 0:
      for package_name in target.deps_packages.keys():
        if package_name in self.deps_packages.keys():
          self.deps_packages[package_name][0] = self.deps_packages[package_name][0] or target.deps_packages[package_name][0]
          self.deps_packages[package_name][1] = list(set(self.deps_packages[package_name][1]) | set(target.deps_packages[package_name][1]))
        else:
          self.deps_packages[package_name] = target.deps_packages[package_name]
    if len(target.link_modules) > 0:
      self.link_modules = set(set(self.link_modules) | set(target.link_modules))

  def find_all_deps_packages(self, find_and_link_packages, project):
    packages = {}
    modules = set()
    if len(find_and_link_packages) > 0:
      for package in find_and_link_packages:
        package_name = package[0]
        search_paths = project.instead_source_path_prefix_list(package[1])
        configd = package[2] == "cmake::configd"
        start_index = 3 if configd else 2
        for module in package[start_index:]:
          modules.add(module)
        packages[package_name] = [configd, search_paths]
    return packages, modules

  def find_first_of(self, s, a):
    return min(s.find(i) for i in a if i in s)

  def cmake_target_escape(self, a):
    """Escapes the string 'a' for use as a CMake target name.

    CMP0037 in CMake 3.0 restricts target names to "^[A-Za-z0-9_.:+-]+$"
    The ':' is only allowed for imported targets.
    """
    def Escape(c):
      if c in string.ascii_letters or c in string.digits or c in '_.+-':
        return c
      return '__'

    return ''.join([Escape(c) for c in a])

  def get_cmake_target_name(self):
    path_separator = self.find_first_of(self.gn_name, (':', '('))
    location = None
    name = None
    toolchain = None
    if not path_separator:
      location = self.gn_name[2:]
    else:
      location = self.gn_name[2:path_separator]
      toolchain_separator = self.gn_name.find('(', path_separator)
      if toolchain_separator == -1:
        name = self.gn_name[path_separator + 1:]
      else:
        if toolchain_separator > path_separator:
          name = self.gn_name[path_separator + 1:toolchain_separator]
        assert self.gn_name.endswith(')')
        toolchain = self.gn_name[toolchain_separator + 1:-1]
    assert location or name
  
    cmake_target_name = None
    if location.endswith('/' + name):
      cmake_target_name = location
    elif location:
      cmake_target_name = location + '_' + name
    else:
      cmake_target_name = name
    if toolchain:
      cmake_target_name += '--' + toolchain
    return self.cmake_target_escape(cmake_target_name)

class Writer:
  def __init__(self, output_path):
    dir = os.path.dirname(output_path)
    if not os.path.exists(dir):
      os.makedirs(dir)
    self.out = open(output_path, 'w+')

  def write_header_info(self, target):
    self.out.write('# Automatically generated by gn_to_cmake_script.py\n')
    self.out.write('# Please modify configs of compilation in %sBUILD.gn\n' % target.declare_path)
    self.out.write('\n')
    self.out.write('# Set the minimum version of CMAKE that is required\n')
    self.out.write('cmake_minimum_required(VERSION %s)\n' % target.cmake_version)
    if target.project_name:
      self.out.write('project (%s)\n' % target.project_name)
    self.out.write('\n\n')

  def write_cmake_root_path(self, root_path):
    self.out.write('set(ROOT_PATH %s)\n\n' % root_path)

  def write_enable_asm(self):
    self.out.write('enable_language(ASM)\n\n')

  def write_prebuild_action(self, target, project):
    if type(target) != Target:
      return -1
    arguments = target.args
    if target.response_file_contents != []:
      response_file_dir = os.path.join(project.build_path, 'rsp_files')
      response_file_path = os.path.join(response_file_dir, target.cmake_name + '.rsp')
      if not os.path.exists(response_file_dir):
        os.makedirs(response_file_dir, exist_ok=True)
      with open(response_file_path, 'w+') as response_file:
        for content in target.response_file_contents:
          response_file.write(f"{content}\n")
      response_file.close()
      if '{{response_file_name}}' in arguments:
        index = arguments.index('{{response_file_name}}')
        arguments[index] = response_file_path
    self.write_single_variable('set', 'action_target', target.cmake_name)
    sources_path = []
    for source in target.sources:
      source_abs_path = project.instead_source_path_prefix(source)
      sources_path.append(source_abs_path)
    source_target_name =  '${action_target}__sources'
    self.write_variable_list('set', source_target_name, sources_path)

    outputs = []
    output_directories = set()
    for output in target.outputs:
      output_abs_path = project.instead_source_path_prefix(output)
      outputs.append(output_abs_path)
      output_directory = os.path.dirname(output_abs_path)
      if output_directory:
        output_directories.add(output_directory)
    outputs_name = '${action_target}__output'
    self.write_variable_list('set', outputs_name, outputs)
    self.out.write('add_custom_command(OUTPUT ')
    self.write_cmake_variable(outputs_name)
    self.out.write('\n')
    if output_directories:
      self.out.write('  COMMAND ${CMAKE_COMMAND} -E make_directory "')
      self.out.write('" "'.join([cmake_string_escape(d) for d in output_directories]))
      self.out.write('"\n')
    
    script = project.instead_source_path_prefix(target.script)
    self.out.write('  COMMAND python3 "')
    self.out.write(cmake_string_escape(script))
    self.out.write('"')
    if arguments:
      self.out.write('\n    "')
      self.out.write('"\n    "'.join([cmake_string_escape(a) for a in arguments]))
      self.out.write('"')
    self.out.write('\n')
    self.out.write('  DEPENDS ')
    self.write_cmake_variable(source_target_name, ' ')
    self.out.write('\n')
    build_path = project.instead_source_path_prefix(project.build_path)
    self.out.write('  WORKING_DIRECTORY "')
    self.out.write(cmake_string_escape(build_path))
    self.out.write('"\n')
    self.out.write('  COMMENT "Action: ${action_target}"\n')
    self.out.write('  VERBATIM)\n')
    self.out.write(target.cmake_type.command)
    self.out.write('(${action_target}')
    if target.cmake_type.modifier is not None:
      self.out.write(' ')
      self.out.write(target.cmake_type.modifier)
    source_target_name =  '${action_target}__sources'
    self.write_cmake_variable(source_target_name, ' ')
    self.out.write(' DEPENDS')
    self.write_cmake_variable(outputs_name, ' ')
    self.out.write(')\n\n')

    other_libraries = set()
    for dependency in target.deps:
      dep_target = Target(dependency, project)
      cmake_dependency_type = cmake_target_types.get(dep_target.gn_type, None)
      cmake_dependency_name = dep_target.cmake_name
      if cmake_dependency_type.command != 'add_library':
        other_libraries.add(cmake_dependency_name)
      elif cmake_dependency_type.modifier != 'OBJECT':
        if not target.cmake_type.is_linkable:
          other_libraries.add(cmake_dependency_name)
    if other_libraries:
      self.out.write('add_dependencies("${action_target}"')
      for other_library in other_libraries:
        self.out.write('\n  "')
        self.out.write(other_library)
        self.out.write('"')
      self.out.write(')\n\n')
    self.out.write('\n')
    
    return target.cmake_name

  def write_target_sources(self, target, project):
    if len(target.sources) > 0:
      sources_paths = project.instead_source_path_prefix_list(target.sources)
      cmake_type = target.cmake_type
      self.write_command_variable_list(cmake_type.command, target.cmake_name, 'OBJECT EXCLUDE_FROM_ALL', sources_paths)

  def write_target_defines(self, target):
    all_defines = sorted(set(target.defines))
    if len(all_defines) > 0:
      self.write_command_variable_list('target_compile_definitions', target.cmake_name, 'PRIVATE', all_defines)

  def write_target_include_dirs(self, target, project):
    if len(target.include_dirs) > 0:
      include_dirs = project.instead_source_path_prefix_list(target.include_dirs)
      self.write_command_variable_list('target_include_directories', target.cmake_name, 'PRIVATE', include_dirs)

  def write_target_compile_flags(self, target):
    if len(target.asmflags) > 0:
      self.write_compile_options_list(target.cmake_name, 'PRIVATE', 'ASM', target.asmflags)
    if len(target.cflags) > 0 or len(target.cflags_c) > 0:
      c_flags = target.cflags + target.cflags_c
      self.write_compile_options_list(target.cmake_name, 'PRIVATE', 'C', c_flags)
    if len(target.cflags) > 0 or len(target.cflags_cc) > 0:
      cc_flags = target.cflags + target.cflags_cc
      self.write_compile_options_list(target.cmake_name, 'PRIVATE', 'CXX', cc_flags)

  def write_source_target(self, target, project):
    if len(target.sources) > 0:
      self.write_target_sources(target, project)
      self.write_target_defines(target)
      self.write_target_include_dirs(target, project)
      self.write_target_compile_flags(target)
      self.write_link_package(target)
      self.write_dep_actions(target.cmake_name, target.dep_actions)

  def write_target(self, target, project):
    if target.gn_type == 'action':
      self.write_prebuild_action(target, project)
    elif len(target.sources) > 0 :
      self.write_source_target(target, project)

  def write_main_target(self, target, deps_source_targets):
    cmake_type = target.cmake_type
    self.out.write('\n# Main target\n')
    self.out.write('%s(' % cmake_type.command)
    self.out.write(target.output_name)
    if cmake_type.modifier is not None:
      self.out.write(' %s' % cmake_type.modifier)
    self.out.write('\n  ')
    if len(deps_source_targets) > 0:
      for target in deps_source_targets:
        if target.gn_type != 'action':
          self.out.write('\n  $<TARGET_OBJECTS:%s>' % target.cmake_name)
          self.out.write('\n  ')
    self.out.write(')\n\n')

  def write_dep_actions(self, target_name, target_dep_actions):
    script_names = sorted(list(target_dep_actions))
    if len(script_names) > 0:
      self.write_variable_list('add_dependencies', target_name, script_names)

  def write_lib_search_paths(self, target, project):
    if len(target.lib_dirs) > 0:
        dirs = project.instead_source_path_prefix_list(target.lib_dirs)
        self.write_current_target_link_directories(target.output_name, dirs)

  def write_deps_local_binary_libs(self, target, deps_binary_targets, project):
    target_paths = set()
    target_path = project.instead_source_path_prefix(target.gn_name.split(':')[0])
    for binary_target in deps_binary_targets:
      binary_target_path = project.instead_source_path_prefix(binary_target.gn_name.split(':')[0])
      search_path_name = binary_target.cmake_name + '_search_path'
      self.write_variable_list('set', search_path_name, ["${CMAKE_LIBRARY_OUTPUT_DIRECTORY}"])
      self.out.write("string(REPLACE \n")
      self.out.write("%s \n" % (target_path))
      self.out.write("%s \n" % (binary_target_path))
      self.out.write("%s \n" % (search_path_name))
      self.out.write("${%s}) \n\n" % (search_path_name))
      target_paths.add("${%s}" % search_path_name)
    if len(target_paths) > 0:
      self.write_current_target_link_directories(target.output_name, target_paths)

  def write_target_link_libs(self, target, deps_binary_targets):
    all_libs = set(target.libs) | set(target.link_modules)
    for binary_target in deps_binary_targets:
      all_libs.add(binary_target.output_name)
    if len(all_libs) > 0:
      all_libs_list = list(all_libs)
      all_libs_list_sorted = sorted(all_libs_list)
      self.write_variable_list('target_link_libraries', target.output_name, all_libs_list_sorted)

  def write_find_package(self, target, project):
    deps_packages = target.deps_packages
    if len(deps_packages) <= 0:
      return
    for package_name in deps_packages.keys():
      configd = deps_packages[package_name][0]
      search_paths = deps_packages[package_name][1]
      configd_str = ' REQUIRED CONFIG' if configd else ' '
      search_path_str = ' PATHS ' if len(search_paths) > 0 else ''
      self.write_variable_list('find_package', package_name + configd_str + search_path_str, search_paths)

  def write_link_package(self, target):
    if len(target.link_modules) <= 0:
      return
    for module in target.link_modules:
        self.write_single_variable('target_link_libraries', target.cmake_name, module)

  def write_subdirectory(self, target, project):
    if len(target.sub_cmake_target) > 0:
      self.out.write('\n# subdirectory\n')
    for sub in target.sub_cmake_target:
      sub_target = Target(sub, project)
      sub_target_path = project.instead_source_path_prefix(sub.split(':')[0])
      self.write_single_variable('add_subdirectory', sub_target_path, sub_target.output_name)

  def write_linker_flags(self, target):
    if (len(target.ldflags)>0):
      self.out.write('# Compiler and Linker flags\n')
      self.write_files_property(cmake_link_flags_tags.get(target.gn_type, 'CMAKE_SHARED_LINKER_FLAGS'), target.ldflags, ' ')

  # helper method
  def write_single_variable(self, type, variable_name, value):
    """Sets a CMake variable."""
    self.out.write(type)
    if variable_name:
      self.out.write('(')
      self.out.write(cmake_string_escape(variable_name))
      self.out.write(' \n  ')
    else:
      self.out.write('(')
    self.out.write(cmake_string_escape(value))
    self.out.write('\n  )\n\n')

  def write_command_variable_list(self, command_type, target_name, export_level, values):
    """Sets a CMake command's variable to a list."""
    if not (command_type and target_name and export_level and values):
      return
    self.out.write(command_type)
    self.out.write('(%s %s' % (cmake_string_escape(target_name), cmake_string_escape(export_level)))
    self.out.write('\n  ')
    self.out.write('\n  '.join([cmake_string_escape(value) for value in values]))
    self.out.write('\n  )\n\n')

  def write_compile_options_list(self, target_name, export_level, compile_language, values):
    """Sets a CMake command's variable to a list."""
    if not (target_name and export_level and compile_language and len(values) > 0):
      return
    self.out.write('target_compile_options')
    self.out.write('(%s %s' % (cmake_string_escape(target_name), cmake_string_escape(export_level)))
    self.out.write('\n  ')
    self.out.write('$<$<COMPILE_LANGUAGE:%s>: "SHELL:%s" >'% (compile_language, ' '.join([cmake_string_escape(value) for value in values])))
    self.out.write('\n  )\n\n')

  def write_variable_list(self, type, variable_name, values):
    """Sets a CMake variable to a list."""
    if not values:
      self.write_single_variable(type, variable_name, "")
      return
    if len(values) == 1:
      self.write_single_variable(type, variable_name, values[0])
      return
    self.out.write(type)
    if variable_name:
      self.out.write('(')
      self.out.write(cmake_string_escape(variable_name))
      self.out.write('\n  ')
    else:
      self.out.write('(\n  ')
    self.out.write('\n  '.join([cmake_string_escape(value) for value in values]))
    self.out.write('\n  )\n\n')

  def write_files_property(self, property_name, values, sep):
    """Given a set of source files, sets the given property on them."""
    self.out.write('set(')
    self.out.write(property_name)
    self.out.write(' "')
    self.out.write('${')
    self.out.write(property_name)
    self.out.write('} ')
    for value in values:
      self.out.write(cmake_string_escape(value))
      self.out.write(sep)
    self.out.write('")\n\n')

  def write_current_target_link_directories(self, target_name, target_paths, sep=''):
    """Given a target, sets the given link directories."""
    self.out.write('target_link_directories(%s PUBLIC ' % target_name)
    self.out.write(' ')
    for value in target_paths:
      self.out.write('\n  ')
      self.out.write(cmake_string_escape(value))
      self.out.write(sep)
    self.out.write('\n  )\n\n')

  def write_cmake_variable(self, variable_name, prepend=None):
    if prepend:
      self.out.write(prepend)
    self.out.write('${')
    self.out.write(variable_name)
    self.out.write('}')
  
def write_project(project, target):
  if type(project) != Project or type(target) != Target:
    return -1
  start_target = target
  if not start_target.cmake_type.is_linkable:
    raise Exception('The %s target specified in arguments must be a linkable target' % (start_target.gn_name))

  target_path = start_target.output_path
  cmake_secondary_dir_name = Path(project.build_path).name
  cmake_out_path = os.path.join(target_path, 'CMakeLists_impl', cmake_secondary_dir_name, target.file_name)
  
  deps_source_targets, deps_binary_targets = start_target.find_all_dependencies(project)
  deps_source_targets_list = list(deps_source_targets)
  deps_source_targets_list_sorted = sorted(deps_source_targets_list, key=lambda t:t.gn_name)
  deps_binary_targets_list = list(deps_binary_targets)
  deps_binary_targets_list_sorted = sorted(deps_binary_targets_list, key=lambda t:t.gn_name)
  
  writer = Writer(cmake_out_path)

  writer.write_header_info(start_target)
  writer.write_cmake_root_path(project.root_path)
  writer.write_enable_asm()
  writer.write_find_package(start_target, project)

  for target in deps_source_targets_list_sorted:
    writer.write_target(target, project)

  writer.write_main_target(start_target, deps_source_targets_list_sorted)
  writer.write_dep_actions(start_target.output_name, start_target.dep_actions)
  writer.write_lib_search_paths(start_target, project)
  writer.write_deps_local_binary_libs(start_target, deps_binary_targets_list_sorted, project)
  writer.write_subdirectory(start_target, project)
  writer.write_target_link_libs(start_target, deps_binary_targets_list_sorted)
  writer.write_linker_flags(start_target)

  r = 0
  for sub in start_target.sub_cmake_target:
    sub_target = Target(sub, project)
    r |= write_project(project, sub_target)
  return r

def gn_to_cmake(project_json_object, cmake_targets):
  project = Project(project_json_object)
  r = 0
  for target_name in cmake_targets:
    if target_name not in project.targets.keys():
      print("%s is not existed in GN project." % target_name)
      continue
    cmake_target = Target(target_name, project)
    r |= write_project(project, cmake_target)
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

def get_cmake_targets(cmake_targets_dir):
  lines = []
  for filename in os.listdir(cmake_targets_dir):
    file_path = os.path.join(cmake_targets_dir, filename)
    if os.path.isfile(file_path):
      with open(file_path, "r+") as file:
        lines += file.readlines()
        file.close()
  if len(lines) > 0:
    cmake_targets = [line.replace("\n", "") for line in lines]
  return cmake_targets

def main():
  print("Hello, gn to cmake!")
  if len(sys.argv) > 1:
    project_json_file = sys.argv[1]
    print(project_json_file)
    project = read_json_file(project_json_file)
    if len(project) <= 0:
      return -1

    gen_root_dir = os.path.dirname(project_json_file)
    # cmake targets
    cmake_targets = []
    if len(sys.argv) > 2:
      cmake_target = sys.argv[2]
      cmake_targets = [cmake_target]
    else :
      error_tips = "You haven't defined the cmake_target in your GN project yet."
      custom_targets_file = os.path.join(gen_root_dir, PROJECT_FILE_NAME)
      cmake_targets = get_cmake_targets(custom_targets_file)
      if len(cmake_targets) <= 0:
        print(error_tips)
        return -1
    print("cmake_targets: ", cmake_targets)
    
    
    return gn_to_cmake(project, cmake_targets)
  else:
    print("Please pass the json file to the current script to generate the corresponding Cmake files.")
    return -1

if __name__ == "__main__":
  sys.exit(main())