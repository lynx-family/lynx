# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
import argparse
import logging
import os
import platform
import shutil
import subprocess
import sys

machine = platform.machine().lower()
machine = "x64" if machine == "x86_64" else machine

def gn_clean(root_path):
  gn_path = os.path.join(root_path, 'buildtools', 'gn', 'gn')
  output_path = os.path.join(root_path, 'out', 'Default')
  if not os.path.exists(output_path):
    return 0
  command =  '%s clean %s' % (gn_path, output_path)
  result = subprocess.check_call(command, shell=True)
  return result

def gen_build_file(platform, arch, debug, root_path, type, sysroot):
  gn_path = os.path.join(root_path, 'buildtools', 'gn', 'gn')
  output_path = os.path.join(root_path, 'out', 'Default')
  is_debug = 'false'
  if debug:
    is_debug = 'true'

  args = 'disallow_undefined_symbol=false enable_security_protection=false use_flutter_cxx=false is_debug=%s oliver_type=\\\"%s\\\"' % (is_debug, type)
  if platform == 'linux' and len(sysroot) != 0:
    args += ' target_sysroot=\\\"%s\\\"' % os.path.join(root_path, sysroot)
  if type == 'ssr':
    args += ' build_lepus_compile=true enable_air=false'
  elif type == 'security':
    args += ' build_lepus_compile=false enable_air=false'
  elif type == 'tasm':
    args += ' build_lepus_compile=true \
      enable_air=false \
      enable_inspector=true \
      enable_inspector_test=true \
      build_lynx_lepus_node=true \
      node_headers_dst_dir=\\\"//lynx/oliver/lynx-tasm\\\" '
    if platform == 'linux':
      args += ' emsdk_dir=\\\"/root/emsdk\\\"'
  elif type == 'testing':
    args += ' build_lepus_compile=false \
      enable_air=false \
      enable_napi_binding=true \
      enable_lepusng_worklet=true \
      jsengine_type=\\\"quickjs\\\"'
    
  if len(arch) > 0:
    args += " target_cpu=\\\"%s\\\"" % (arch)

  command = "{} gen {} --args=\"{}\"".format(gn_path, output_path, args)
  print(command)
  result = subprocess.check_call(command, shell=True)
  return result

def build_by_ninja(root_path, show_log):
  ninja_path = os.path.join(root_path, 'buildtools', 'ninja', 'ninja')
  output_path = os.path.join(root_path, 'out', 'Default')

  if show_log:
    command = '%s -C %s oliver_group -v' % (ninja_path, output_path)
  else:
    command = '%s -C %s oliver_group' % (ninja_path, output_path)
  result = subprocess.check_call(command, shell=True)
  return result

def get_type_path(type):
  type_path = ''
  if type == 'ssr':
    type_path = 'lynx-ssr-runtime'
  elif type == 'security':
    type_path = 'lynx-security'
  elif type == 'tasm':
    type_path = 'lynx-tasm'
  elif type == 'testing':
    type_path = 'lynx-testing'
  return type_path

def get_output_name(type):
  output_name = ''
  if type == 'ssr':
    output_name = 'lynx_ssr.node'
  elif type == 'security':
    output_name = 'lynx_security.node'
  elif type == 'tasm':
    output_name = 'lepus.node'
  elif type == 'testing':
    output_name = 'lynx_testing.node'
  return output_name

def copy_target(folder_name, arch, debug, root_path, type):
  dst_name = 'Release'
  if debug:
    dst_name = 'Debug'
  type_path = get_type_path(type)
  output_name = get_output_name(type)
  dst_path = os.path.join(root_path, 'lynx/oliver', type_path, 'build', folder_name, dst_name)
  if len(arch) > 0:
    dst_path = os.path.join(dst_path, arch)
  if os.path.exists(os.path.join(dst_path, output_name)):
    os.remove(os.path.join(dst_path, output_name))
  src_path = os.path.join(root_path, 'out', 'Default', 'oliver', output_name)
  if not os.path.exists(dst_path):
    os.makedirs(dst_path)
  print("src_path:",src_path);
  print("dst_path:",dst_path);
  shutil.copy(src_path, dst_path)

def copy_wasm_target(root_path, type):
  type_path = get_type_path(type)
  dst_root_path = os.path.join(root_path, 'lynx/oliver', type_path)
  lib_dst_path = os.path.join(dst_root_path, 'build', 'wasm')
  if not os.path.exists(lib_dst_path):
    os.makedirs(lib_dst_path)
  output_name = ''
  if type == 'tasm':
    output_name = 'lepus'
  src_root_path = os.path.join(root_path, 'out', 'Default', 'wasm')
  lib_src_path = os.path.join(src_root_path, 'lib%s.a' % (output_name))
  js_src_path = os.path.join(src_root_path, '%s.js' % (output_name))
  shutil.copy(lib_src_path, lib_dst_path)
  shutil.copy(js_src_path, dst_root_path)

def build_target(platform, arch, debug, root_path, show_log, type, need_clean, sysroot):
  if need_clean:
    result = gn_clean(root_path)
    if result != 0:
      return result

  result = gen_build_file(platform, arch, debug, root_path, type, sysroot)
  if result != 0:
    return result

  result = build_by_ninja(root_path, show_log)
  if result != 0:
    return result

  return 0

def merge_file(folder_name, debug, root_path, type):
  dst_name = 'Release'
  if debug:
    dst_name = 'Debug'
  type_path = get_type_path(type)
  output_name = get_output_name(type)
  dst_root_path = os.path.join(root_path, 'lynx/oliver', type_path, 'build', folder_name, dst_name)
  arm64_path = os.path.join(dst_root_path, 'arm64', output_name)
  x86_64_path = os.path.join(dst_root_path, 'x64', output_name)
  dst_path = os.path.join(dst_root_path, output_name)
  command = ''
  if folder_name == 'darwin':
    command = 'lipo -create %s %s -output %s' % (arm64_path, x86_64_path, dst_path)
  else:
    return -1
  return subprocess.check_call(command, shell=True)

def build(system, debug, root_path, show_log, type, is_wasm, need_clean, is_local, sysroot):
  # TODO(wangqingyu): remove wasm build when is_local after bumping up lynx-speedy
  if is_wasm:
    system = platform.system().lower()
    result = build_target(system, 'wasm', debug, root_path, show_log, type, need_clean, sysroot)
    if result != 0:
      return result
    copy_wasm_target(root_path, type)
  else:
    if system == 'darwin':
      result = build_target(system, machine, debug, root_path, show_log, type, need_clean, sysroot)
      if result != 0:
        return result
      if is_local:
        # Copy to oliver/{type}/build/{system}/{type}.node
        # Without the machine folder
        copy_target(system, '', debug, root_path, type)
        return 0
      # Copy to oliver/{type}/build/{system}/{machine}/{type}.node
      # For merging with other targets
      copy_target(system, machine, debug, root_path, type)
      # For darwin release build, need build not only for current machine, but also for the other machine
      # TODO(wangqingyu): chaning other_machine to list if darwin has more supported machine
      other_machine = [m for m in ['x64', 'arm64'] if m != machine][0]
      result = build_target(system, other_machine, debug, root_path, show_log, type, need_clean, sysroot)
      if result != 0:
        return result
      copy_target(system, other_machine, debug, root_path, type)
      # Uses `lipo` to get merge multiple target for different machine
      # to get a universal binary that support all machine
      result = merge_file(system, debug, root_path, type)
      if result != 0:
        return result
      return 0
    elif system == 'linux':
      result = build_target(system, '', debug, root_path, show_log, type, need_clean, sysroot)
      if result != 0:
        return result
      copy_target(system, '', debug, root_path, type)
    else:
      return -1
  return 0

def main():
  parser = argparse.ArgumentParser()
  parser.add_argument('-p', '--platform', help='Target platform')
  parser.add_argument('-t', '--type', help='Oliver product type. The type contains values such as ssr, tasm, security, and testing')
  parser.add_argument('--clean', type=bool, default=True, help='run gn clean before build. Default is true')
  parser.add_argument('--debug', type=bool, default=False, help='Build product of debug type')
  parser.add_argument('--local', type=bool, default=False, help='Build for local CPU architecture. Only has effects on the Darwin system')
  parser.add_argument('--show_log', type=bool, default=False, help='Output compilation log')
  parser.add_argument('--wasm', type=bool, default=False, help='Build wasm product')
  parser.add_argument('--sysroot', type=str, default='', help='Custom sysroot for build on linux')
  args = parser.parse_args()

  platform = args.platform
  is_show_log = args.show_log
  type = args.type
  need_clean = args.clean
  is_debug = args.debug
  is_wasm = args.wasm
  is_local = args.local
  sysroot = args.sysroot

  file_path = os.path.dirname(os.path.abspath(__file__))
  root_path = os.path.join(file_path, '../..')
  result = build(platform, is_debug, root_path, is_show_log, type, is_wasm, need_clean, is_local, sysroot)
  if result != 0:
    return result
  return 0

if __name__ == '__main__':
  sys.exit(main())