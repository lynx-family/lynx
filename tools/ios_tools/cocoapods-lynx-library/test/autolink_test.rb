# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

require 'tmpdir'
require 'minitest/autorun'
require_relative '../lib/lynx/library/autolink'

class LynxLibraryAutolinkTest < Minitest::Test
  def test_scan_ios_library_manifest
    Dir.mktmpdir do |dir|
      write_library(dir, 'demo-lib', 'DemoLib')

      libraries = Lynx::Library::Autolink.scan(File.join(dir, 'ios'))

      assert_equal 1, libraries.size
      assert_equal 'demo-lib', libraries.first.npm_name
      assert_equal File.realpath(File.join(dir, 'node_modules/demo-lib/ios/DemoLib.podspec')),
                   libraries.first.podspec_path
    end
  end

  def test_generate_registry_from_native_markers
    Dir.mktmpdir do |dir|
      package_dir = write_library(dir, '@scope/demo-lib', 'DemoLib')
      File.write(File.join(package_dir, 'ios/DemoModule.m'), <<~OBJC)
        #import <Lynx/LynxModule.h>
        @LynxNativeModuleRegister("NativeLocalStorage")
        @interface DemoModule : NSObject <LynxModule>
        @end
        @implementation DemoModule
        + (NSDictionary *)methodLookup { return @{}; }
        @end
      OBJC
      File.write(File.join(package_dir, 'ios/DemoUI.m'), <<~OBJC)
        #import <Lynx/LynxUI.h>
        @LynxUIRegister("marked-ui")
        @implementation DemoMarkedUI
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
        @LynxServiceRegister(DemoService, LynxDemoServiceProtocol)
        @implementation DemoService
        @end

        @LynxServiceRegister(LegacyService, LynxLegacyServiceProtocol)
        @implementation LegacyService
        @end
      OBJC

      libraries = Lynx::Library::Autolink.scan(dir)
      components = Lynx::Library::Autolink.generate_registry(
        File.join(dir, 'generated/lynx-library'), libraries)
      registry_dir = File.join(dir, 'generated/lynx-library')
      header = File.read(File.join(registry_dir, 'LynxGeneratedLibraryRegistry.h'))
      implementation = File.read(File.join(registry_dir, 'LynxGeneratedLibraryRegistry.m'))
      podspec = File.read(File.join(registry_dir, 'LynxLibraryRegistry.podspec'))

      assert_equal 6, components.size
      assert_includes header, '@interface LynxGeneratedLibraryRegistry : NSObject'
      refute_includes header, '@interface LibraryRegistry'
      assert_includes implementation, '#import "LynxGeneratedLibraryRegistry.h"'
      assert_includes implementation, '@implementation LynxGeneratedLibraryRegistry'
      refute_includes implementation, '@implementation LibraryRegistry'
      assert_includes podspec, "s.source_files = 'LynxGeneratedLibraryRegistry.{h,m}'"
      assert_includes components,
                      Lynx::Library::ComponentInfo.new(
                        :service, 'LynxDemoServiceProtocol', 'DemoService')
      assert_includes components,
                      Lynx::Library::ComponentInfo.new(
                        :service, 'LynxLegacyServiceProtocol', 'LegacyService')
      assert_includes implementation,
                      '[config registerModule:NSClassFromString(@"DemoModule") withName:@"NativeLocalStorage"]'
      assert_includes implementation,
                      '[config registerUI:NSClassFromString(@"DemoMarkedUI") withName:@"marked-ui"]'
      assert_includes implementation,
                      '[config registerUI:NSClassFromString(@"DemoUI") withName:@"demo-ui"]'
      assert_includes implementation,
                      '[config registerShadowNode:NSClassFromString(@"DemoShadowNode") withName:@"demo-shadow"]'
      refute_includes implementation, 'registerService'
    end
  end

  def test_rfc_native_module_marker_is_ignored
    Dir.mktmpdir do |dir|
      package_dir = write_library(dir, 'demo-lib', 'DemoLib')
      File.write(File.join(package_dir, 'ios/OldModule.m'), <<~OBJC)
        #import <Lynx/LynxModule.h>
        LynxNativeModuleRegister("OldModule")
        @interface OldModule : NSObject <LynxModule>
        @end
      OBJC

      components = Lynx::Library::Autolink.scan_components(File.join(package_dir, 'ios'))

      assert_empty components
    end
  end

  def test_manifest_requires_podspec
    Dir.mktmpdir do |dir|
      package_dir = File.join(dir, 'node_modules/bad-lib')
      FileUtils.mkdir_p(File.join(package_dir, 'ios'))
      File.write(File.join(package_dir, 'lynx.lib.json'), '{"platforms":{"ios":{}}}')

      error = assert_raises(RuntimeError) { Lynx::Library::Autolink.scan(dir) }
      assert_includes error.message, 'No iOS podspec found'
    end
  end

  def test_manifest_rejects_source_dir_outside_package
    Dir.mktmpdir do |dir|
      package_dir = File.join(dir, 'node_modules/bad-lib')
      FileUtils.mkdir_p(package_dir)
      File.write(File.join(package_dir, 'lynx.lib.json'),
                 '{"platforms":{"ios":{"sourceDir":"../shared-ios"}}}')
      FileUtils.mkdir_p(File.join(dir, 'node_modules/shared-ios'))

      error = assert_raises(RuntimeError) { Lynx::Library::Autolink.scan(dir) }
      assert_includes error.message, "iOS sourceDir '../shared-ios' must stay within package directory"
    end
  end

  def test_manifest_rejects_podspec_path_outside_package
    Dir.mktmpdir do |dir|
      package_dir = File.join(dir, 'node_modules/bad-lib')
      FileUtils.mkdir_p(File.join(package_dir, 'ios'))
      File.write(File.join(package_dir, 'lynx.lib.json'),
                 '{"platforms":{"ios":{"podspecPath":"../shared/Shared.podspec"}}}')
      shared_dir = File.join(dir, 'node_modules/shared')
      FileUtils.mkdir_p(shared_dir)
      File.write(File.join(shared_dir, 'Shared.podspec'), <<~PODSPEC)
        Pod::Spec.new do |s|
          s.name = 'Shared'
        end
      PODSPEC

      error = assert_raises(RuntimeError) { Lynx::Library::Autolink.scan(dir) }
      assert_includes error.message,
                      "iOS podspecPath '../shared/Shared.podspec' must stay within package directory"
    end
  end

  def test_manifest_rejects_source_dir_symlink_outside_package
    Dir.mktmpdir do |dir|
      package_dir = File.join(dir, 'node_modules/bad-lib')
      outside_dir = File.join(dir, 'outside-ios')
      FileUtils.mkdir_p(package_dir)
      FileUtils.mkdir_p(outside_dir)
      File.symlink(outside_dir, File.join(package_dir, 'ios-link'))
      File.write(File.join(package_dir, 'lynx.lib.json'),
                 '{"platforms":{"ios":{"sourceDir":"ios-link"}}}')

      error = assert_raises(RuntimeError) { Lynx::Library::Autolink.scan(dir) }
      assert_includes error.message, "iOS sourceDir 'ios-link' must stay within package directory"
    end
  end

  def test_manifest_rejects_podspec_path_symlink_outside_package
    Dir.mktmpdir do |dir|
      package_dir = File.join(dir, 'node_modules/bad-lib')
      outside_dir = File.join(dir, 'outside-ios')
      FileUtils.mkdir_p(File.join(package_dir, 'ios'))
      FileUtils.mkdir_p(outside_dir)
      File.write(File.join(outside_dir, 'Outside.podspec'), <<~PODSPEC)
        Pod::Spec.new do |s|
          s.name = 'Outside'
        end
      PODSPEC
      File.symlink(File.join(outside_dir, 'Outside.podspec'),
                   File.join(package_dir, 'ios/Outside.podspec'))
      File.write(File.join(package_dir, 'lynx.lib.json'),
                 '{"platforms":{"ios":{"podspecPath":"ios/Outside.podspec"}}}')

      error = assert_raises(RuntimeError) { Lynx::Library::Autolink.scan(dir) }
      assert_includes error.message,
                      "iOS podspecPath 'ios/Outside.podspec' must stay within package directory"
    end
  end

  def test_manifest_rejects_default_podspec_symlink_outside_package
    Dir.mktmpdir do |dir|
      package_dir = File.join(dir, 'node_modules/bad-lib')
      outside_dir = File.join(dir, 'outside-ios')
      FileUtils.mkdir_p(File.join(package_dir, 'ios'))
      FileUtils.mkdir_p(outside_dir)
      File.write(File.join(outside_dir, 'Outside.podspec'), <<~PODSPEC)
        Pod::Spec.new do |s|
          s.name = 'Outside'
        end
      PODSPEC
      File.symlink(File.join(outside_dir, 'Outside.podspec'),
                   File.join(package_dir, 'ios/Outside.podspec'))
      File.write(File.join(package_dir, 'lynx.lib.json'), '{"platforms":{"ios":{}}}')

      error = assert_raises(RuntimeError) { Lynx::Library::Autolink.scan(dir) }
      assert_includes error.message, 'must stay within package directory'
    end
  end

  def test_manifest_ignores_symlinked_directories_during_default_podspec_discovery
    Dir.mktmpdir do |dir|
      package_dir = File.join(dir, 'node_modules/demo-lib')
      outside_dir = File.join(dir, 'outside-ios')
      FileUtils.mkdir_p(File.join(package_dir, 'ios'))
      FileUtils.mkdir_p(outside_dir)
      File.write(File.join(outside_dir, 'AOutside.podspec'), <<~PODSPEC)
        Pod::Spec.new do |s|
          s.name = 'Outside'
        end
      PODSPEC
      File.write(File.join(package_dir, 'ios/DemoLib.podspec'), <<~PODSPEC)
        Pod::Spec.new do |s|
          s.name = 'DemoLib'
        end
      PODSPEC
      File.symlink(outside_dir, File.join(package_dir, 'ios/linked'))
      File.write(File.join(package_dir, 'lynx.lib.json'), '{"platforms":{"ios":{}}}')

      libraries = Lynx::Library::Autolink.scan(dir)

      assert_equal File.realpath(File.join(package_dir, 'ios/DemoLib.podspec')),
                   libraries.first.podspec_path
    end
  end

  private

  def write_library(root, npm_name, pod_name)
    package_dir = File.join(root, 'node_modules', npm_name)
    FileUtils.mkdir_p(File.join(package_dir, 'ios'))
    File.write(File.join(package_dir, 'lynx.lib.json'), '{"platforms":{"ios":{}}}')
    File.write(File.join(package_dir, "ios/#{pod_name}.podspec"), <<~PODSPEC)
      Pod::Spec.new do |s|
        s.name = '#{pod_name}'
      end
    PODSPEC
    package_dir
  end
end
