# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

Gem::Specification.new do |spec|
  spec.name = 'cocoapods-lynx-extension'
  spec.version = '0.1.0'
  spec.summary = 'CocoaPods autolink plugin for Lynx native extensions.'
  spec.authors = ['Lynx Authors']
  spec.license = 'Apache-2.0'
  spec.files = Dir['lib/**/*.rb']
  spec.require_paths = ['lib']
  spec.add_runtime_dependency 'cocoapods', '>= 1.11.0'
end
