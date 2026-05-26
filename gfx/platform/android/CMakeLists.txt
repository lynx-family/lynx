# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

cmake_minimum_required(VERSION 3.4.1)

include(${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists_impl/${FLAVOR_NAME}${BUILD_TYPE}${ANDROID_ABI}/CMakeLists.txt)
