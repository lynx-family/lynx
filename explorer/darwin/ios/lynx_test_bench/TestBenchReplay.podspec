# Copyright 2024 The Lynx Authors. All rights reserved. 
# Licensed under the Apache License Version 2.0 that can be found in the 
# LICENSE file in the root directory of this source tree.
# coding: utf-8

Pod::Spec.new do |spec|
  spec.name = 'TestBenchReplay'
  spec.version = '0.0.1'
  spec.license = { :type => 'MIT' }
  spec.homepage = 'https://github.com/lynx-family/lynx'
  spec.summary = 'Lynx TestBenchReplay module'
  spec.authors = 'Lynx'
  spec.source = { :git => 'https://github.com/lynx-family/lynx' }
  spec.module_name = 'TestBenchReplay'
  spec.header_dir = 'TestBenchReplay'
  spec.source_files = '*.{h,cc,m,mm}'
  spec.compiler_flags = '-Wno-unreachable-code'

  spec.user_target_xcconfig = { "HEADER_SEARCH_PATHS" => "\"$(PODS_ROOT)/TestBenchReplay\"" }

  spec.pod_target_xcconfig = {
    "DEFINES_MODULE" => "YES",
  }
  spec.ios.deployment_target = "10.0"
end
