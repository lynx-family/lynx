#!/usr/bin/env python3
# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import sys
import os
import subprocess
import platform
import argparse
system = platform.system().lower()

ndk_version = '21.1.6352462'
android_platform = 'android-33'
sdk_build_tools_version = '33.0.1'

def run_command(command):
  if system != 'windows':
    command = 'set -e\n' + command

  print(f'run command: {command}')
  res = subprocess.run(command, shell=True)

def download_android_ndk_manager_if_needed(root_path, need_download_sdk_manager):
  buildtools_dir = os.path.join(root_path, 'buildtools')
  sdk_manager_dir = os.path.join(buildtools_dir, 'android_sdk_manager')
  sdk_manager_path = os.path.join(sdk_manager_dir, 'bin')
  if not need_download_sdk_manager:
    if not os.path.exists(sdk_manager_path):
      print("Error: SDK manager not found, please run `tools/hab sync . -f --target dev --target-only` to check dependency.")
      sys.exit(-1)
    return sdk_manager_path
  if not os.path.exists(sdk_manager_path):
    tools_dir = os.path.join(root_path, 'tools')
    hab_path = os.path.join(tools_dir, 'hab') if system !='windows' else os.path.join(tools_dir, 'hab.ps1')
    run_command(f'{hab_path} sync . -f --target dev --target-only')
  return sdk_manager_path

def install_sdk_component(sdk_path, sdk_manager, component, version, is_agree_license):
  component_path = os.path.join(sdk_path, component, version)
  if os.path.exists(component_path):
    print(f"'{component};{version}' already installed at {component_path} - skipping installation.")
    return 0
  try:
    cmd = f'{sdk_manager} --sdk_root={sdk_path} --install "{component};{version}"'
    subprocess.run(cmd, check=True, shell=True, input=b'y\n' if is_agree_license else None)
  except subprocess.CalledProcessError as e:
    print(f"Error installing {component};{version}: {e}")
    return -1
  return 0

def install_android_sdk(root_path, need_download_sdk_manager=False, always_agree_license=False):
  sdk_path = os.getenv("ANDROID_HOME")
  if not sdk_path:
    print("Error: Please configure the ANDROID_HOME environment variable first.")
    return -1

  sdk_manager_path = download_android_ndk_manager_if_needed(root_path, need_download_sdk_manager)
  is_agree_license = always_agree_license
  sdk_manager = os.path.join(sdk_manager_path, 'sdkmanager.bat' if system == 'windows' else 'sdkmanager')

  r = 0
  r |= install_sdk_component(sdk_path, sdk_manager, "ndk", ndk_version, is_agree_license)
  r |= install_sdk_component(sdk_path, sdk_manager, "platforms", android_platform, is_agree_license)
  r |= install_sdk_component(sdk_path, sdk_manager, "build-tools", sdk_build_tools_version, is_agree_license)
  if r != 0:
    return r

  print("\n====> SUCCESS!!! <====\nAndroid environment setup completed.")
  print("Now you can run `cd explorer/android` and `./gradlew :LynxExplorer:assembleNoAsanDebug` to build LynxExplorer APP.")

def main():
  parser = argparse.ArgumentParser(description='')
  parser.add_argument('-a', '--always-agree-license', action='store_true', required=False, help='Whether to always accept SDK-related licenses.')
  args, unknown = parser.parse_known_args()
  root_path = os.path.join(os.path.dirname(__file__), '..', '..')
  install_android_sdk(root_path, True, args.always_agree_license)

if __name__ == "__main__":
  sys.exit(main())