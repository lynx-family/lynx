# Copyright 2019 The Lynx Authors. All rights reserved. 
# Licensed under the Apache License Version 2.0 that can be found in the 
# LICENSE file in the root directory of this source tree.
# coding: utf-8

$is_debug = ENV['ENABLE_DEBUG'] == 'true'

Pod::Spec.new do |p|
  p.author                      = "Lynx"
  p.compiler_flags              = "-Wall", "-Wextra", "-Wno-unused-parameter", "-Wshorten-64-to-32", "-fno-rtti"
  p.default_subspec             = "Framework"
  p.homepage                    = "https://xxx.xxx.xxx/lynx/Test"
  p.ios.deployment_target       = "9.0"
  p.ios.framework               = "WebKit", "AudioToolbox"
  p.license                     = "MIT"
  p.name                        = "gnToPodspecTest"
  p.pod_target_xcconfig         = {
    "CLANG_CXX_LANGUAGE_STANDARD" => "gnu++17",
    "GCC_PREPROCESSOR_DEFINITIONS" => "HOST_OSTYPE=HOST_IOS \
                                      LYNX_DEBUG=#{$is_debug}",
    "OTHER_CPLUSPLUSFLAGS" => "-fno-aligned-allocation"
  }
  p.prepare_command             = <<-CMD
    echo "test bundle pre command"
    echo "test bundle pre build"
  CMD
  p.requires_arc                = true
  p.source                      = {
    :git => "git@xxx.xxx.xxx:lynx/Test.git"
  }
  p.summary                     = "The framework of gnToPodspecTest."
  p.version                     = "1.4.22"

  p.subspec "test_subspec" do |sp|
    sp.compiler_flags             = "-Wall", "-Wextra", "-Wno-unused-parameter"
    sp.frameworks                 = "Accelerate", "AVFoundation"
    sp.header_mappings_dir        = "tools/gn_tools"
    sp.libraries                  = "stdc++", "log"
    sp.vendored_libraries         = "tools/gn_tools/Lynx/help.a"

    sp.vendored_frameworks        = "tools/gn_tools/Lynx/Help.framework"

    sp.public_header_files        = "tools/gn_tools/test/gn_test_shared.h"

    sp.private_header_files       = "tools/gn_tools/**/*.{h,hpp}", 
                                    "tools/gn_tools/test/*.{h,hpp}", 
                                    "tools/gn_tools/test/gn_test_source.h"

    sp.source_files               = "tools/gn_tools/**/*.h", 
                                    "tools/gn_tools/test/**/*.m", 
                                    "tools/gn_tools/test/gn_test_shared.cc", 
                                    "tools/gn_tools/test/gn_test_shared.h", 
                                    "tools/gn_tools/test/gn_test_source.cc", 
                                    "tools/gn_tools/test/gn_test_source.h", 
                                    "tools/gn_tools/test/gn_test_source_flatten_subspec.cc", 
                                    "tools/gn_tools/test/gn_test_source_flatten_subspec.h"

    sp.exclude_files              = "tools/to/file/**/*"

    sp.pod_target_xcconfig        = {
      "CLANG_CXX_LANGUAGE_STANDARD" => "gnu++17",
      "GCC_PREPROCESSOR_DEFINITIONS" => "IS_DEBUG=#{$is_debug} \
                                        IS_FLATTEN=true \
                                        OS_IOS=1",
      "HEADER_SEARCH_PATHS" => "\"${PODS_ROOT}/tools/gn_tools\" \
                               \"${PODS_TARGET_SRCROOT}/tools/gn_tools/flatten\" \
                               \"${PODS_TARGET_SRCROOT}/tools/gn_tools\" \
                               \"${PODS_TARGET_SRCROOT}/tools/gn_tools\" \
                               \"${TARGET_BUILD_DIR}/tools/gn_tools\""
    }
    sp.dependency                 "test_dep/Framework", ">=1.0.5"
    sp.dependency                 "test_dep/Native"
    sp.dependency                 "test_dep/SubspecNative"
    if $is_debug==1
    sp.subspec "test_subspec_condition_subspec" do |ssp|
      ssp.private_header_files      = "tools/gn_tools/test/*.{h,hpp}"

      ssp.source_files              = "tools/gn_tools/test/gn_test_source_condition_subspec.cc", 
                                      "tools/gn_tools/test/gn_test_source_condition_subspec.h"

      ssp.pod_target_xcconfig       = {
        "GCC_PREPROCESSOR_DEFINITIONS" => "IS_DEBUG=#{$is_debug}",
        "HEADER_SEARCH_PATHS" => "\"${PODS_TARGET_SRCROOT}/tools/gn_tools\""
      }
    end
    end

    sp.subspec "test_subspec_subspec" do |ssp|
      ssp.private_header_files      = "tools/gn_tools/test/*.{h,hpp}"

      ssp.source_files              = "tools/gn_tools/test/gn_test_source_subspec.cc", 
                                      "tools/gn_tools/test/gn_test_source_subspec.h"

      ssp.pod_target_xcconfig       = {
        "CLANG_CXX_LANGUAGE_STANDARD" => "gnu++17",
        "GCC_PREPROCESSOR_DEFINITIONS" => "IS_DEBUG=#{$is_debug}"
      }
    end
    sp.resource_bundles           = {
      "LynxResources" => [
                           "darwin/ios/JSAssets/release/lepus_bridge.js",
                           "darwin/ios/JSAssets/release/lynx_core.js"
                         ]
    }
  end
  if $is_debug==1
  p.subspec "ConditionSub" do |sp|
    sp.private_header_files       = "tools/gn_tools/test/*.{h,hpp}"

    sp.source_files               = "tools/gn_tools/test/gn_test_source_flatten_subspec_top.cc", 
                                    "tools/gn_tools/test/gn_test_source_flatten_subspec_top.h"

    sp.pod_target_xcconfig        = {
      "GCC_PREPROCESSOR_DEFINITIONS" => "IS_FLATTEN=false",
      "HEADER_SEARCH_PATHS" => "\"${PODS_TARGET_SRCROOT}/tools/gn_tools/top/flatten\""
    }
  end
  end
end
