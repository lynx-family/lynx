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
        @LynxNativeModule("NativeLocalStorage")
        @interface DemoModule : NSObject <LynxModule>
        @end
        @implementation DemoModule
        + (NSDictionary *)methodLookup { return @{}; }
        @end
      OBJC
      File.write(File.join(package_dir, 'ios/DemoUI.m'), <<~OBJC)
        #import <Lynx/LynxUI.h>
        @LynxElement("marked-ui")
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
        @LynxService(DemoService, LynxDemoServiceProtocol)
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
      assert_includes podspec,
                      "s.source_files = 'LynxGeneratedLibraryRegistry.{h,m}', 'LynxGeneratedNodeAPIAddonUse.mm'"
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

  def test_scan_ios_node_api_addons
    Dir.mktmpdir do |dir|
      package_dir = write_library(dir, 'demo-addon', 'DemoAddon')
      File.write(File.join(package_dir, 'lynx.lib.json'), <<~JSON)
        {
          "platforms": {
            "ios": {
              "nodeApiAddons": [{
                "name": "demo_addon",
                "podName": "DemoAddon",
                "addonUseHeader": "addon_use.h",
                "required": false
              }]
            }
          }
        }
      JSON

      library = Lynx::Library::Autolink.scan(dir).first

      assert_equal 1, library.node_api_addons.size
      addon = library.node_api_addons.first
      assert_equal 'demo_addon', addon.name
      assert_equal 'DemoAddon', addon.pod_name
      assert_equal File.realpath(File.join(package_dir, 'ios/DemoAddon.podspec')),
                   addon.podspec_path
      assert_equal 'addon_use.h', addon.addon_use_header
      refute_respond_to addon, :required
    end
  end

  def test_generate_registry_includes_node_api_addon_use_source
    Dir.mktmpdir do |dir|
      package_dir = write_library(dir, 'demo-addon', 'DemoAddon')
      File.write(File.join(package_dir, 'lynx.lib.json'), <<~JSON)
        {
          "platforms": {
            "ios": {
              "nodeApiAddons": [{
                "name": "demo_addon",
                "podName": "DemoAddon",
                "addonUseHeader": "addon_use.h"
              }]
            }
          }
        }
      JSON

      libraries = Lynx::Library::Autolink.scan(dir)
      Lynx::Library::Autolink.generate_registry(File.join(dir, 'generated/lynx-library'), libraries)
      registry_dir = File.join(dir, 'generated/lynx-library')
      addon_use = File.read(File.join(registry_dir, 'LynxGeneratedNodeAPIAddonUse.mm'))
      implementation = File.read(File.join(registry_dir, 'LynxGeneratedLibraryRegistry.m'))
      podspec = File.read(File.join(registry_dir, 'LynxLibraryRegistry.podspec'))

      assert_includes addon_use, '#if __has_include(<DemoAddon/addon_use.h>)'
      assert_includes addon_use, '#include <DemoAddon/addon_use.h>'
      assert_includes addon_use, '#include <LynxWeakNodeAPI/primjs_weak_node_api_installer.h>'
      assert_includes addon_use, '#include <PrimJS/primjs_weak_node_api_provider.h>'
      assert_includes addon_use,
                      'PrimJSInstallWeakNodeApiRawPtrHostProvider('
      assert_includes addon_use, 'PrimJSGetWeakNodeApiRawPtrHost);'
      assert_includes addon_use, 'SetupWeakNodeApiEnv();'
      assert_includes addon_use, 'extern void _napi_register_xx_demo_addon(void);'
      assert_includes addon_use, '_napi_register_xx_demo_addon();'
      assert_includes addon_use, 'void LynxGeneratedNodeAPIAddonUse(void) {'
      assert_includes addon_use, 'LynxSetupWeakNodeAPI();'
      assert_includes implementation, 'extern void LynxGeneratedNodeAPIAddonUse(void);'
      assert_includes implementation, 'LynxGeneratedNodeAPIAddonUse();'
      assert_includes podspec,
                      "s.source_files = 'LynxGeneratedLibraryRegistry.{h,m}', 'LynxGeneratedNodeAPIAddonUse.mm'"
      assert_includes podspec, "s.dependency 'LynxWeakNodeAPI/primjs_bridge'"
      assert_includes podspec, "s.dependency 'PrimJS/napi/adapter'"
      assert_includes podspec, "s.dependency 'DemoAddon'"
      refute_includes podspec, 'PrimJS/src/napi/js_native_api'
    end
  end

  def test_install_adds_node_api_addons_by_pod_path
    Dir.mktmpdir do |dir|
      package_dir = write_library(dir, 'demo-addon', 'DemoAddon')
      File.write(File.join(package_dir, 'lynx.lib.json'), <<~JSON)
        {
          "platforms": {
            "ios": {
              "nodeApiAddons": [{
                "name": "demo_addon",
                "podName": "DemoAddon",
                "podspecPath": "ios/DemoAddon.podspec",
                "addonUseHeader": "addon_use.h"
              }]
            }
          }
        }
      JSON
      podfile = FakePodfile.new

      Lynx::Library::Autolink.install!(podfile, root: dir,
                                                output_dir: File.join(dir, 'generated/lynx-library'))

      assert_includes podfile.pods,
                      ['DemoAddon', { path: File.realpath(File.join(package_dir, 'ios')) }]
      assert_equal 1, podfile.pods.count { |name, _options| name == 'DemoAddon' }
      assert_includes podfile.pods,
                      ['LynxLibraryRegistry', { path: File.join(dir, 'generated/lynx-library') }]
    end
  end

  def test_manifest_rejects_invalid_node_api_addon_name
    ['@scope/bad', 'demo-addon', 'foo.bar', '1addon'].each do |name|
      Dir.mktmpdir do |dir|
        package_dir = write_library(dir, 'bad-addon', 'BadAddon')
        File.write(File.join(package_dir, 'lynx.lib.json'), <<~JSON)
          {"platforms":{"ios":{"nodeApiAddons":[{"name":"#{name}"}]}}}
        JSON

        error = assert_raises(RuntimeError) { Lynx::Library::Autolink.scan(dir) }
        assert_includes error.message, 'nodeApiAddons[0].name'
        assert_includes error.message,
                        'expected a C identifier matching [A-Za-z_][A-Za-z0-9_]*'
      end
    end
  end

  def test_manifest_rejects_node_api_addon_podspec_path_outside_package
    Dir.mktmpdir do |dir|
      package_dir = write_library(dir, 'bad-addon', 'BadAddon')
      shared_dir = File.join(dir, 'node_modules/shared')
      FileUtils.mkdir_p(shared_dir)
      File.write(File.join(shared_dir, 'SharedAddon.podspec'), <<~PODSPEC)
        Pod::Spec.new do |s|
          s.name = 'SharedAddon'
        end
      PODSPEC
      File.write(File.join(package_dir, 'lynx.lib.json'), <<~JSON)
        {
          "platforms": {
            "ios": {
              "nodeApiAddons": [{
                "name": "demo",
                "podspecPath": "../shared/SharedAddon.podspec"
              }]
            }
          }
        }
      JSON

      error = assert_raises(RuntimeError) { Lynx::Library::Autolink.scan(dir) }
      assert_includes error.message,
                      "iOS nodeApiAddons[0].podspecPath '../shared/SharedAddon.podspec' must stay within package directory"
    end
  end

  def test_rfc_native_module_marker_is_ignored
    Dir.mktmpdir do |dir|
      package_dir = write_library(dir, 'demo-lib', 'DemoLib')
      File.write(File.join(package_dir, 'ios/OldModule.m'), <<~OBJC)
        #import <Lynx/LynxModule.h>
        LynxNativeModule("OldModule")
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

  def test_install_rejects_unknown_source
    Dir.mktmpdir do |dir|
      podfile = FakePodfile.new
      error = assert_raises(RuntimeError) do
        Lynx::Library::Autolink.install!(podfile, root: dir,
                                                  output_dir: File.join(dir, 'generated'),
                                                  sources: [:pods, :bogus])
      end
      assert_includes error.message, 'Unknown Lynx library autolink sources: bogus'
    end
  end

  # pods-only mode does no node_modules discovery: install! must not add any
  # library pod, only inject the generated LynxLibraryRegistry pod. The registry
  # written at Podfile-eval time is a stub (no registration lines) that the
  # post-install hook rewrites later; the podspec must exist so CocoaPods can
  # resolve the pod.
  def test_install_pods_only_generates_stub_registry_and_registry_pod
    Dir.mktmpdir do |dir|
      podfile = FakePodfile.new
      output_dir = File.join(dir, 'generated/lynx-library')

      Lynx::Library::Autolink.install!(podfile, root: dir, output_dir: output_dir,
                                                sources: [:pods])

      # No node_modules pod added; only the registry pod is injected.
      assert_equal [['LynxLibraryRegistry', { path: output_dir }]], podfile.pods
      # Initial registry is a stub with no registration lines.
      impl = File.read(File.join(output_dir, "#{Lynx::Library::Autolink::REGISTRY_CLASS_NAME}.m"))
      refute_includes impl, 'registerUI'
      assert File.exist?(File.join(output_dir, 'LynxLibraryRegistry.podspec'))
    end
  end

  # In pods mode the source location comes from the podspec's source_files, not
  # from the manifest's sourceDir. This sets up a deliberate conflict: the
  # manifest says sourceDir "ios" (which does not exist) while the real sources
  # live under "src" as referenced by source_files. The resolved source_dir must
  # be "src", proving sourceDir is ignored. Regression guard for the earlier
  # `iOS sourceDir 'ios' does not exist` failure.
  def test_scan_installed_pods_uses_source_files_and_ignores_source_dir
    Dir.mktmpdir do |dir|
      # Manifest declares sourceDir "ios" which does NOT exist; real sources
      # live under "src" as referenced by source_files.
      pod_root, = write_installed_pod(dir, 'DemoScreens', 'demo-screen', 'DemoScreenUI',
                                      source_subdir: 'src',
                                      manifest_ios: { 'sourceDir' => 'ios' })
      spec = FakeSpec.new('DemoScreens', 'source_files' => 'src/**/*.{h,m,mm,swift}')
      context = build_hook_context({ 'DemoScreens' => pod_root }, [spec])

      libraries = Lynx::Library::Autolink.scan_installed_pods(context, 'Pods-App')

      assert_equal 1, libraries.size
      assert_equal File.realpath(File.join(pod_root, 'src')), libraries.first.source_dir
    end
  end

  # When source_files spans multiple sibling dirs, no single dir covers them,
  # so source_dir falls back to the pod root. The subsequent recursive scan must
  # still reach components in every sub-dir, so the registry contains markers from both.
  def test_scan_installed_pods_falls_back_to_pod_root_for_multiple_source_dirs
    Dir.mktmpdir do |dir|
      # lynx-screens layout: podspec + manifest at pod root, sources split across
      # sibling dirs. Root resolves to pod root.
      pod_root = File.join(dir, 'Pods', 'LynxScreens')
      FileUtils.mkdir_p(File.join(pod_root, 'common'))
      FileUtils.mkdir_p(File.join(pod_root, 'host'))
      File.write(File.join(pod_root, 'lynx.lib.json'),
                 JSON.generate('platforms' => { 'ios' => { 'sourceDir' => 'ios' } }))
      File.write(File.join(pod_root, 'common/BaseUI.m'), <<~OBJC)
        @implementation BaseUI
        LYNX_LAZY_REGISTER_UI("base-ui")
        @end
      OBJC
      File.write(File.join(pod_root, 'host/HostUI.m'), <<~OBJC)
        @implementation HostUI
        LYNX_LAZY_REGISTER_UI("host-ui")
        @end
      OBJC
      spec = FakeSpec.new('LynxScreens',
                          'source_files' => ['common/**/*.{h,m}', 'host/**/*.{h,m}'])
      context = build_hook_context({ 'LynxScreens' => pod_root }, [spec])

      libraries = Lynx::Library::Autolink.scan_installed_pods(context, 'Pods-App')
      assert_equal File.realpath(pod_root), libraries.first.source_dir

      output_dir = File.join(dir, 'generated')
      Lynx::Library::Autolink.generate_registry(output_dir, libraries, write_podspec: false)
      impl = File.read(File.join(output_dir, "#{Lynx::Library::Autolink::REGISTRY_CLASS_NAME}.m"))
      assert_includes impl, 'withName:@"base-ui"'
      assert_includes impl, 'withName:@"host-ui"'
    end
  end

  # The hook only cares about pods that both carry a lynx.lib.json and are not
  # the generated registry itself: a plain pod without a manifest is skipped,
  # and the LynxLibraryRegistry pod is explicitly excluded (it would otherwise
  # scan its own generated sources).
  def test_scan_installed_pods_skips_pods_without_manifest_and_registry
    Dir.mktmpdir do |dir|
      pod_root, = write_installed_pod(dir, 'DemoScreens', 'demo-screen', 'DemoScreenUI',
                                      source_subdir: 'src')
      plain_root = File.join(dir, 'Pods', 'Plain')
      FileUtils.mkdir_p(plain_root)
      registry_root = File.join(dir, 'Pods', 'LynxLibraryRegistry')
      FileUtils.mkdir_p(registry_root)

      specs = [
        FakeSpec.new('DemoScreens', 'source_files' => 'src/**/*.{h,m}'),
        FakeSpec.new('Plain', 'source_files' => '*.m'),
        FakeSpec.new('LynxLibraryRegistry', 'source_files' => '*.m')
      ]
      context = build_hook_context({
        'DemoScreens' => pod_root,
        'Plain' => plain_root,
        'LynxLibraryRegistry' => registry_root
      }, specs)

      libraries = Lynx::Library::Autolink.scan_installed_pods(context, 'Pods-App')

      assert_equal ['DemoScreens'], libraries.map(&:npm_name)
    end
  end

  # Only pods belonging to the target that enabled autolink are scanned. A pod
  # attached to a different umbrella target (Pods-Other) must be ignored when the
  # hook runs for Pods-App.
  def test_scan_installed_pods_filters_by_target_label
    Dir.mktmpdir do |dir|
      pod_root, = write_installed_pod(dir, 'DemoScreens', 'demo-screen', 'DemoScreenUI',
                                      source_subdir: 'src')
      spec = FakeSpec.new('DemoScreens', 'source_files' => 'src/**/*.{h,m}')
      sandbox = FakeSandbox.new('DemoScreens' => pod_root)
      context = FakeHookContext.new(sandbox, [FakeUmbrellaTarget.new('Pods-Other', [spec])])

      libraries = Lynx::Library::Autolink.scan_installed_pods(context, 'Pods-App')

      assert_empty libraries
    end
  end

  # A component exposed by both a node_modules library and a pods library must be
  # registered only once: generate_registry deduplicates across sources by
  # (kind, name, class_name). Also asserts write_podspec: false leaves the
  # podspec untouched during the hook rewrite.
  def test_generate_registry_merges_and_deduplicates_across_sources
    Dir.mktmpdir do |dir|
      # node_modules library and pods library expose the same component.
      nm_dir = write_library(dir, 'demo-lib', 'DemoLib')
      File.write(File.join(nm_dir, 'ios/DupUI.m'), <<~OBJC)
        @implementation DupUI
        LYNX_LAZY_REGISTER_UI("dup-ui")
        @end
      OBJC
      nm_libraries = Lynx::Library::Autolink.scan(dir)

      pod_root, = write_installed_pod(dir, 'DemoScreens', 'dup-ui', 'DupUI',
                                      source_subdir: 'src')
      spec = FakeSpec.new('DemoScreens', 'source_files' => 'src/**/*.{h,m}')
      context = build_hook_context({ 'DemoScreens' => pod_root }, [spec])
      pods_libraries = Lynx::Library::Autolink.scan_installed_pods(context, 'Pods-App')

      output_dir = File.join(dir, 'generated')
      Lynx::Library::Autolink.generate_registry(output_dir, nm_libraries + pods_libraries,
                                                write_podspec: false)

      impl = File.read(File.join(output_dir, "#{Lynx::Library::Autolink::REGISTRY_CLASS_NAME}.m"))
      assert_equal 1, impl.scan('withName:@"dup-ui"').size
      refute File.exist?(File.join(output_dir, 'LynxLibraryRegistry.podspec'))
    end
  end

  # pods mode still reads nodeApiAddons from the manifest. podName defaults
  # to the resolved pod name (spec.root.name) since podspecPath is ignored in
  # pods mode. The generated addon-use source must include the addon header
  # under that pod name and call its register symbol.
  def test_scan_installed_pods_generates_node_api_addon_use
    Dir.mktmpdir do |dir|
      manifest_ios = {
        'sourceDir' => 'ios',
        'nodeApiAddons' => [{ 'name' => 'demo_addon' }]
      }
      pod_root, = write_installed_pod(dir, 'DemoScreens', 'demo-screen', 'DemoScreenUI',
                                      source_subdir: 'src', manifest_ios: manifest_ios)
      spec = FakeSpec.new('DemoScreens', 'source_files' => 'src/**/*.{h,m}')
      context = build_hook_context({ 'DemoScreens' => pod_root }, [spec])
      libraries = Lynx::Library::Autolink.scan_installed_pods(context, 'Pods-App')

      addon = libraries.first.node_api_addons.first
      assert_equal 'demo_addon', addon.name
      assert_equal 'DemoScreens', addon.pod_name

      output_dir = File.join(dir, 'generated')
      Lynx::Library::Autolink.generate_registry(output_dir, libraries, write_podspec: false)
      addon_use = File.read(File.join(output_dir,
                                      Lynx::Library::Autolink::ADDON_USE_SOURCE_NAME))
      assert_includes addon_use, '#include <DemoScreens/addon_use.h>'
      assert_includes addon_use, '_napi_register_xx_demo_addon();'
    end
  end

  private

  class FakePodfile
    attr_reader :pods

    def initialize
      @pods = []
    end

    def pod(name, options = {})
      @pods << [name, options]
    end
  end

  # --- pods source fakes ---------------------------------------------------

  FakeSpec = Struct.new(:name, :attributes_hash) do
    def root
      self
    end
  end

  FakeUmbrellaTarget = Struct.new(:cocoapods_target_label, :specs)

  class FakeSandbox
    def initialize(pod_dirs)
      @pod_dirs = pod_dirs
    end

    def pod_dir(name)
      @pod_dirs[name]
    end
  end

  FakeHookContext = Struct.new(:sandbox, :umbrella_targets)

  # Write an installed pod layout: pod root holds lynx.lib.json + a source dir
  # (referenced from source_files) that contains a UI marker.
  def write_installed_pod(root, pod_name, tag_name, class_name, source_subdir: 'src',
                          manifest_ios: { 'sourceDir' => 'ios' })
    pod_root = File.join(root, 'Pods', pod_name)
    source_dir = File.join(pod_root, source_subdir)
    FileUtils.mkdir_p(source_dir)
    File.write(File.join(pod_root, 'lynx.lib.json'),
               JSON.generate('platforms' => { 'ios' => manifest_ios }))
    File.write(File.join(source_dir, "#{class_name}.m"), <<~OBJC)
      @implementation #{class_name}
      LYNX_LAZY_REGISTER_UI("#{tag_name}")
      @end
    OBJC
    [pod_root, source_dir]
  end

  def build_hook_context(pod_roots, specs, label: 'Pods-App')
    sandbox = FakeSandbox.new(pod_roots)
    FakeHookContext.new(sandbox, [FakeUmbrellaTarget.new(label, specs)])
  end

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
