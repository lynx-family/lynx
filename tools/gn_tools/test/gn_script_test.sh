#!/bin/bash
# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

# generate cmake 
gn gen out/gn_cmake_test --args="enable_unittests=true use_flutter_cxx=false" --ide=cmake --cmake-target="//tools/gn_tools/test:cmake_test"
# generate podspec 
gn gen out/gn_podspec_test --args="enable_unittests=true use_flutter_cxx=false" --ide=podspec --podspec-target="//tools/gn_tools/test:podspec_test"

# print cmake diff
sed -i '8d' 'tools/gn_tools/test/CMakeLists_impl/gn_cmake_test/CMakeLists.txt'
if test "$(git diff --name-only tools/gn_tools/test/CMakeLists_impl/gn_cmake_test/CMakeLists.txt | wc -l)" -gt 0 ; then
  git diff tools/gn_tools/test/CMakeLists_impl/gn_cmake_test/CMakeLists.txt > ./out/gn_cmake_test/cmake_diff.log;
  cat ./out/gn_cmake_test/cmake_diff.log;
fi

# print podspec diff
if test "$(git diff --name-only tools/gn_tools/test/gn_test.podspec | wc -l)" -gt 0 ; then
  git diff tools/gn_tools/test/gn_test.podspec > ./out/gn_podspec_test/podspec_diff.log;
  cat ./out/gn_podspec_test/podspec_diff.log;
fi

if test "$(git diff --name-only tools/gn_tools/test/CMakeLists_impl/gn_test/CMakeLists.txt | wc -l)" -gt 0 ; then
  exit 1;
fi

if test "$(git diff --name-only tools/gn_tools/test/gn_test.podspec | wc -l)" -gt 0 ; then
  exit 1;
fi
