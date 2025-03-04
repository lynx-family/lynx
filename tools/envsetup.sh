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
  export LYNX_DIR="$(dirname $TOOLS_ABSOLUTE_PATH)"
  export LYNX_ROOT_DIR="$(dirname $LYNX_DIR)"
  export BUILDTOOLS_DIR="${LYNX_ROOT_DIR}/buildtools"
  export PATH="${BUILDTOOLS_DIR}/llvm/bin:${BUILDTOOLS_DIR}/ninja:${TOOLS_ABSOLUTE_PATH}/gn_tools:$PATH"
  # setup node version
  export PATH=${BUILDTOOLS_DIR}/node/bin:$PATH
  # setup corepack
  export COREPACK_HOME="${BUILDTOOLS_DIR}/corepack"

  export PATH="${LYNX_DIR}/tools_shared:$PATH"
}

function android_env_setup() {
  local SCRIPT_REAL_PATH=$(posix_absolute_path $1)
  local TOOLS_REAL_PATH=$(dirname $SCRIPT_REAL_PATH)
  echo $TOOLS_REAL_PATH
  if [ "$ANDROID_HOME" ]; then
    export ANDROID_NDK=$ANDROID_HOME/ndk/21.1.6352462
    export ANDROID_NDK_21=$ANDROID_HOME/ndk/21.1.6352462
    export ANDROID_SDK=$ANDROID_HOME

    ln -vsnf "$ANDROID_HOME" "$TOOLS_REAL_PATH"/android_tools/sdk
    ln -vsnf "$ANDROID_NDK" "$TOOLS_REAL_PATH"/android_tools/ndk
  else
    echo "Please setup ANDROID_HOME for android build first."
  fi
}

lynx_envsetup "${BASH_SOURCE:-$0}"
android_env_setup "${BASH_SOURCE:-$0}"
