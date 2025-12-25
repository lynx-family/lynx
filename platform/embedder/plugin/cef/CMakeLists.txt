# Copyright (c) 2014 The Chromium Embedded Framework Authors. All rights
# reserved. Use of this source code is governed by a BSD-style license that
# can be found in the LICENSE file.

# For VS2022 and Xcode 12+ support.
cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Only generate Debug and Release configuration types.
set(CMAKE_CONFIGURATION_TYPES Debug Release)

# Project name.
project(cef_webview)

# Use folders in the resulting project files.
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Custom variable Settings
set(HOST_EXECUTABLE_NAME "LynxExplorer")
set(PROJECT_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../..)
set(LYNX_ROOT_DIR ${PROJECT_ROOT_DIR})
set(FRAMEWORK_DST_PATH ${CMAKE_BINARY_DIR}/frameworks)

#
# CEF_ROOT setup.
# This variable must be set to locate the binary distribution.
#
set(CEF_ROOT "${PROJECT_ROOT_DIR}/third_party/cef_binary")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CEF_ROOT}/cmake")

#
# Load the CEF configuration.
#

# Execute FindCEF.cmake which must exist in CMAKE_MODULE_PATH.
find_package(CEF REQUIRED)

#
# Define CEF-based targets.
#

# Include the libcef_dll_wrapper target.
# Comes from the libcef_dll/CMakeLists.txt file in the binary distribution
# directory.
add_subdirectory(${CEF_LIBCEF_DLL_WRAPPER_PATH} libcef_dll_wrapper)

add_subdirectory(src)

# Display configuration settings.
PRINT_CEF_CONFIG()
