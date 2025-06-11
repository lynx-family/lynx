#!/usr/bin/env python3
# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import sys
import os
import subprocess
import platform
system = platform.system()

ndk_version = '21.1.6352462'
android_platform = 'android-33'
sdk_build_tools_version = '33.0.1'

def install_sdk_component(sdk_path, sdk_manager, component, version):
  component_path = os.path.join(sdk_path, component, version)
  if os.path.exists(component_path):
    print(f"'{component};{version}' already installed at {component_path} - skipping installation.")
    return 0
  try:
    cmd = f'{sdk_manager} --sdk_root={sdk_path} --install "{component};{version}"'
    subprocess.run(cmd, check=True, shell=True)
  except subprocess.CalledProcessError as e:
    print(f"Error installing {component};{version}: {e}")
    return -1
  return 0

def main():
  sdk_path = os.getenv("ANDROID_HOME")
  if not sdk_path:
    print("Error: Please configure the ANDROID_HOME environment variable first.")
    return -1
  buildtools_dir = os.path.join(os.path.dirname(__file__), '..', '..', '..', 'buildtools')
  if not os.path.exists(buildtools_dir):
    print("Error: buildtools directory not found. Please run `tools/hab sync . -f` first.")
    return -1
  
  sdk_manager_dir = os.path.join(buildtools_dir, 'android_sdk_manager')
  sdk_manager_path = os.path.join(sdk_manager_dir, 'bin')

  if not os.path.exists(sdk_manager_path):
    print("SDK manager not found, please run `tools/hab sync . -f` first.")
    return -1

  sdk_manager = os.path.join(sdk_manager_path, 'sdkmanager.bat' if system == 'Windows' else 'sdkmanager')

  r = 0
  r |= install_sdk_component(sdk_path, sdk_manager, "ndk", ndk_version)
  r |= install_sdk_component(sdk_path, sdk_manager, "platforms", android_platform)
  r |= install_sdk_component(sdk_path, sdk_manager, "build-tools", sdk_build_tools_version)
  if r != 0:
    return r

  print("\n====> SUCCESS!!! <====\nAndroid environment setup completed.")
  print("Now you can run `cd explorer/android` and `./gradlew :LynxExplorer:assembleNoAsanDebug` to build LynxExplorer APP.")

if __name__ == "__main__":
  sys.exit(main())