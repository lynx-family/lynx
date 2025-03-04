#!/bin/bash
# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
set -e

current_dir=$(cd "$(dirname "$0")"; pwd)

bash "$current_dir/devtool-switch/build.sh"
bash "$current_dir/lynx-logbox/build.sh"
bash "$current_dir/images/copy_images.sh"
