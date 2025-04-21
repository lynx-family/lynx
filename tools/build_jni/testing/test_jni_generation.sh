# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

CURRENT_PATH="$(dirname "$(realpath "$0")")"
GEN_FILE=$CURRENT_PATH"/../generate_and_register_jni_files.py"
LIBRARY_ROOT_PATH=$CURRENT_PATH"/../../.."
$CURRENT_PATH/../../../.venv/bin/python3 $GEN_FILE -root $LIBRARY_ROOT_PATH -path $CURRENT_PATH/jni_configs.yml
