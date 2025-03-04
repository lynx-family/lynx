#!/bin/bash
# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

ndk_version=21.1.6352462
android_platform=android-33
sdk_build_tools_version=33.0.1

# font color
RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m' # No Color

sdk_path=$(eval echo "$ANDROID_HOME")

if [ -z "$sdk_path" ]; then
  echo -e "${RED}Error: Please configure the ANDROID_HOME environment variable first.${NC}"
  return 1
fi

if [ -z "$BUILDTOOLS_DIR" ]; then
  echo -e "${RED}Error: BUILDTOOLS_DIR environment variable is not set. please run \`source tools/envsetup.sh && tools/hab sync . -f\` first.${NC}"
  return 1
fi

sdk_manager_dir=$BUILDTOOLS_DIR/android_sdk_manager

sdk_manager_path=$sdk_manager_dir/bin
if [ ! -d $sdk_manager_path ]; then
  echo "sdk manager not found, please run \`tools/hab sync . -f\` first."
  return 1
fi

function install_sdk_component {
  local component_path=$sdk_path/$1/$2
  if [ -d "$component_path" ]; then
    echo "'$1;$2' already installed at $component_path - skipping installation."
  else
    $sdk_manager_path/sdkmanager --sdk_root=$sdk_path --install "$1;$2"
  fi
}

# check and install ndk
install_sdk_component "ndk" "$ndk_version"

# check and install platform
install_sdk_component "platforms" "$android_platform"

# check and install build-tools
install_sdk_component "build-tools" "$sdk_build_tools_version"

echo "${GREEN}Android environment setup completed.${NC}"
echo "${GREEN}Now you can run \`cd explorer/android && ./gradlew :LynxExplorer:assembleNoAsanDebug\` to build LynxExplorer APP.${NC}"