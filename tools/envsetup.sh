#!/bin/bash
# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

# using posix standard commands to acquire realpath of file
posix_absolute_path() {
  if [[ ! $# -eq 1 ]];then
    echo "illegal parameters $@"
    exit 1
  fi
  cd $(dirname $1) 1>/dev/null || exit 1
  local ABSOLUTE_PATH_OF_FILE="$(pwd -P)/$(basename $1)"
  cd - 1>/dev/null || exit 1
  echo $ABSOLUTE_PATH_OF_FILE
}


lynx_envsetup() {
  local SCRIPT_ABSOLUTE_PATH="$(posix_absolute_path $1)"
  local TOOLS_ABSOLUTE_PATH="$(dirname $SCRIPT_ABSOLUTE_PATH)"
  # shellcheck disable=SC1091
  source "${TOOLS_ABSOLUTE_PATH}/env.sh" -- || exit 1
}

function android_env_setup() {
  local SCRIPT_REAL_PATH=$(posix_absolute_path $1)
  local TOOLS_REAL_PATH=$(dirname $SCRIPT_REAL_PATH)
  echo $TOOLS_REAL_PATH
  if [ "$ANDROID_HOME" ]; then
    export ANDROID_NDK=$ANDROID_HOME/ndk/21.1.6352462
    export ANDROID_NDK_21=$ANDROID_HOME/ndk/21.1.6352462
    export ANDROID_SDK=$ANDROID_HOME
    
    local local_properties_file1="${LYNX_DIR}/platform/android/local.properties"
    local local_properties_file2="${LYNX_DIR}/explorer/android/local.properties"
    local CMAKE_DIR="${LYNX_DIR}/buildtools/cmake"
    python3 $LYNX_DIR/tools/android_tools/update_local_properties.py -f $local_properties_file1 $local_properties_file2 -p ndk.dir="$ANDROID_NDK" sdk.dir="$ANDROID_SDK" cmake.dir="$CMAKE_DIR"
  else
    echo "Please setup ANDROID_HOME for android build first."
  fi
}

HARMONY_SDK_VERSION='6.0.0.868'

function harmony_home_setup_for_ci() {
    if [ -z "${COMMANDLINE_TOOL_BASE_DIR}" ]; then
        echo "NOTICE: COMMANDLINE_TOOL_BASE_DIR is not set. If you are not doing a local build or not a harmony build, please ignore this warning."
        return
    fi

    COMMANDLINE_TOOL_DIR="${COMMANDLINE_TOOL_BASE_DIR}/${HARMONY_SDK_VERSION}/command-line-tools"
    export COMMANDLINE_TOOL_DIR

    export HARMONY_HOME="${COMMANDLINE_TOOL_DIR}/sdk"
    export PATH="${COMMANDLINE_TOOL_DIR}/bin:${PATH}"

    echo "harmony_home_setup_for_ci done"
}

lynx_envsetup "${BASH_SOURCE:-$0}"
android_env_setup "${BASH_SOURCE:-$0}"
harmony_home_setup_for_ci
