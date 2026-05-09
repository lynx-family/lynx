# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

require 'tmpdir'
require 'minitest/autorun'
require_relative '../lib/lynx/extension/autolink'

class LynxExtensionAutolinkTest < Minitest::Test
  def test_scan_ios_extension_manifest
    Dir.mktmpdir do |dir|
      write_extension(dir, 'demo-ext', 'DemoExt')

      extensions = Lynx::Extension::Autolink.scan(File.join(dir, 'ios'))

      assert_equal 1, extensions.size
      assert_equal 'demo-ext', extensions.first.npm_name
      assert_equal File.join(dir, 'node_modules/demo-ext/ios/DemoExt.podspec'),
                   extensions.first.podspec_path
    end
  end

  def test_generate_registry_from_native_markers
    Dir.mktmpdir do |dir|
      package_dir = write_extension(dir, '@scope/demo-ext', 'DemoExt')
      File.write(File.join(package_dir, 'ios/DemoModule.m'), <<~OBJC)
        #import <Lynx/LynxModule.h>
        @LynxAutolinkNativeModule("NativeLocalStorage")
        @interface DemoModule : NSObject <LynxModule>
        @end
        @implementation DemoModule
        + (NSDictionary *)methodLookup { return @{}; }
        @end
      OBJC
      File.write(File.join(package_dir, 'ios/DemoUI.m'), <<~OBJC)
        #import <Lynx/LynxUI.h>
        @LynxAutolinkUI("autolink-ui")
        @implementation DemoAutolinkUI
        @end

        @implementation DemoUI
        LYNX_LAZY_REGISTER_UI("demo-ui")
        LYNX_LAZY_REGISTER_UI("demo-ui")
        @end

        @implementation DemoShadowNode
        LYNX_LAZY_REGISTER_SHADOW_NODE("demo-shadow")
        @end
      OBJC
      File.write(File.join(package_dir, 'ios/DemoService.m'), <<~OBJC)
        #import <LynxServiceAPI/ServiceAPI.h>
        @LynxAutolinkService(DemoService, LynxDemoServiceProtocol)
        @implementation DemoService
        @end

        @LynxServiceRegister(LegacyService, LynxLegacyServiceProtocol)
        @implementation LegacyService
        @end
      OBJC

      extensions = Lynx::Extension::Autolink.scan(dir)
      components = Lynx::Extension::Autolink.generate_registry(
        File.join(dir, 'generated/lynx-extension'), extensions)
      registry_dir = File.join(dir, 'generated/lynx-extension')
      header = File.read(File.join(registry_dir, 'LynxGeneratedExtensionRegistry.h'))
      implementation = File.read(File.join(registry_dir, 'LynxGeneratedExtensionRegistry.m'))
      podspec = File.read(File.join(registry_dir, 'LynxExtensionRegistry.podspec'))

      assert_equal 6, components.size
      assert_includes header, '@interface LynxGeneratedExtensionRegistry : NSObject'
      refute_includes header, '@interface ExtensionRegistry'
      assert_includes implementation, '#import "LynxGeneratedExtensionRegistry.h"'
      assert_includes implementation, '@implementation LynxGeneratedExtensionRegistry'
      refute_includes implementation, '@implementation ExtensionRegistry'
      assert_includes podspec, "s.source_files = 'LynxGeneratedExtensionRegistry.{h,m}'"
      assert_includes components,
                      Lynx::Extension::ComponentInfo.new(
                        :service, 'LynxDemoServiceProtocol', 'DemoService')
      assert_includes components,
                      Lynx::Extension::ComponentInfo.new(
                        :service, 'LynxLegacyServiceProtocol', 'LegacyService')
      assert_includes implementation,
                      '[config registerModule:NSClassFromString(@"DemoModule") withName:@"NativeLocalStorage"]'
      assert_includes implementation,
                      '[config registerUI:NSClassFromString(@"DemoAutolinkUI") withName:@"autolink-ui"]'
      assert_includes implementation,
                      '[config registerUI:NSClassFromString(@"DemoUI") withName:@"demo-ui"]'
      assert_includes implementation,
                      '[config registerShadowNode:NSClassFromString(@"DemoShadowNode") withName:@"demo-shadow"]'
      refute_includes implementation, 'registerService'
    end
  end

  def test_rfc_native_module_marker_is_ignored
    Dir.mktmpdir do |dir|
      package_dir = write_extension(dir, 'demo-ext', 'DemoExt')
      File.write(File.join(package_dir, 'ios/OldModule.m'), <<~OBJC)
        #import <Lynx/LynxModule.h>
        LynxNativeModule("OldModule")
        @interface OldModule : NSObject <LynxModule>
        @end
      OBJC

      components = Lynx::Extension::Autolink.scan_components(File.join(package_dir, 'ios'))

      assert_empty components
    end
  end

  def test_manifest_requires_podspec
    Dir.mktmpdir do |dir|
      package_dir = File.join(dir, 'node_modules/bad-ext')
      FileUtils.mkdir_p(File.join(package_dir, 'ios'))
      File.write(File.join(package_dir, 'lynx.ext.json'), '{"platforms":{"ios":{}}}')

      error = assert_raises(RuntimeError) { Lynx::Extension::Autolink.scan(dir) }
      assert_includes error.message, 'No iOS podspec found'
    end
  end

  private

  def write_extension(root, npm_name, pod_name)
    package_dir = File.join(root, 'node_modules', npm_name)
    FileUtils.mkdir_p(File.join(package_dir, 'ios'))
    File.write(File.join(package_dir, 'lynx.ext.json'), '{"platforms":{"ios":{}}}')
    File.write(File.join(package_dir, "ios/#{pod_name}.podspec"), <<~PODSPEC)
      Pod::Spec.new do |s|
        s.name = '#{pod_name}'
      end
    PODSPEC
    package_dir
  end
end
