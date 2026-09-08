# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

require 'fileutils'
require 'json'

module Lynx
  module Library
    LibraryInfo = Struct.new(
      :npm_name, :package_dir, :manifest_file, :source_dir, :podspec_path, :node_api_addons)
    ComponentInfo = Struct.new(:kind, :name, :class_name)
    NodeApiAddonInfo = Struct.new(
      :name, :pod_name, :podspec_path, :addon_use_header)

    class Autolink
      REGISTRY_CLASS_NAME = 'LynxGeneratedLibraryRegistry'
      ADDON_USE_SOURCE_NAME = 'LynxGeneratedNodeAPIAddonUse.mm'
      ADDON_NAME_PATTERN = /\A[A-Za-z_][A-Za-z0-9_]*\z/
      PLUGIN_NAME = 'cocoapods-lynx-library'
      DEFAULT_SOURCES = [:node_modules, :pods].freeze
      KNOWN_SOURCES = [:node_modules, :pods].freeze

      class << self
        def install!(podfile, options = {})
          start_dir = File.expand_path(options[:root] || Dir.pwd)
          registry_dir = options[:output_dir] || 'generated/lynx-library'
          output_dir = File.expand_path(registry_dir, start_dir)
          sources = normalize_sources(options[:sources])

          node_modules_libraries = sources.include?(:node_modules) ? scan(start_dir) : []
          add_node_modules_pods(podfile, node_modules_libraries)

          # Generate the initial registry. In pods mode it may be a stub that the
          # post-install hook rewrites once the installed pods are available.
          generate_registry(output_dir, node_modules_libraries)
          # Pass the raw (unexpanded) path so Podfile.lock records it as given,
          # keeping the lockfile portable instead of an absolute machine path.
          podfile.pod 'LynxLibraryRegistry', :path => registry_dir

          if sources.include?(:pods)
            register_pods_hook(output_dir, node_modules_libraries, current_target_label(podfile))
          end

          node_modules_libraries
        end

        def scan(start_dir)
          node_modules_dirs(start_dir).flat_map do |node_modules|
            manifest_files(node_modules).map { |manifest| parse_manifest(manifest) }.compact
          end.sort_by(&:npm_name)
        end

        def generate_registry(output_dir, libraries, write_podspec: true)
          FileUtils.mkdir_p(output_dir)
          components = collect_components(libraries)
          node_api_addons = libraries.flat_map(&:node_api_addons)
          File.write(File.join(output_dir, "#{REGISTRY_CLASS_NAME}.h"), header_source)
          File.write(File.join(output_dir, "#{REGISTRY_CLASS_NAME}.m"),
                     implementation_source(components))
          File.write(File.join(output_dir, ADDON_USE_SOURCE_NAME), addon_use_source(node_api_addons))
          if write_podspec
            File.write(File.join(output_dir, 'LynxLibraryRegistry.podspec'),
                       podspec_source(node_api_addons))
          end
          components
        end

        def scan_components(source_dir)
          source_files(source_dir).flat_map { |file| scan_source_file(file) }
        end

        private

        def normalize_sources(sources)
          list = Array(sources || DEFAULT_SOURCES).map(&:to_sym)
          list = DEFAULT_SOURCES.dup if list.empty?
          unknown = list - KNOWN_SOURCES
          raise "Unknown Lynx library autolink sources: #{unknown.join(', ')}" unless unknown.empty?
          list.uniq
        end

        def add_node_modules_pods(podfile, libraries)
          added_pods = []
          libraries.each do |library|
            pod_name = pod_name_from_podspec(library.podspec_path)
            add_pod_once(podfile, added_pods, pod_name, File.dirname(library.podspec_path))
            library.node_api_addons.each do |addon|
              add_pod_once(podfile, added_pods, addon.pod_name, File.dirname(addon.podspec_path))
            end
          end
        end

        # Merge components across sources and drop duplicates that appear in more
        # than one source.
        def collect_components(libraries)
          libraries.flat_map { |library| scan_components(library.source_dir) }
                   .uniq { |component| [component.kind, component.name, component.class_name] }
        end

        # --- pods source -----------------------------------------------------

        # Register a CocoaPods post-install hook that rescans the installed pods
        # of the current target and rewrites the registry once their sources are
        # available. The registry podspec stays fixed (write_podspec: false) so
        # the resolved pod graph is never mutated after installation.
        def register_pods_hook(output_dir, node_modules_libraries, target_label)
          return unless defined?(Pod::HooksManager)

          Pod::HooksManager.register(PLUGIN_NAME, :post_install) do |context|
            pods_libraries = Autolink.scan_installed_pods(context, target_label)
            libraries = node_modules_libraries + pods_libraries
            Autolink.generate_registry(output_dir, libraries, write_podspec: false)
          end
        end

        # @return [String, nil] label of the target that invoked use_lynx_library!
        def current_target_label(podfile)
          return nil unless podfile.respond_to?(:current_target_definition)

          definition = podfile.current_target_definition
          definition&.label
        end

        public

        # Scan the pods installed for +target_label+ and turn the ones carrying a
        # lynx.lib.json into LibraryInfo entries. Source location comes from the
        # pod's resolved source_files, not from the manifest's sourceDir.
        def scan_installed_pods(context, target_label)
          sandbox = context.sandbox
          seen_manifests = {}
          pod_specs_for_target(context, target_label).map do |spec|
            pod_name = spec.root.name
            next if pod_name == 'LynxLibraryRegistry'

            pod_root = pod_source_root(sandbox, pod_name)
            unless pod_root
              log_warning("skipped pod '#{pod_name}': source directory not found in sandbox")
              next
            end

            manifest = File.join(pod_root, 'lynx.lib.json')
            next unless File.file?(manifest)

            canonical = File.realpath(manifest)
            if (owner = seen_manifests[canonical])
              # Different pods reusing the same manifest is unexpected.
              if owner != pod_name
                log_warning("skipped pod '#{pod_name}': lynx.lib.json at #{canonical} " \
                            "already provided by '#{owner}'")
              end
              next
            end
            seen_manifests[canonical] = pod_name

            native_library_from_pod(spec, pod_root, manifest)
          end.compact.sort_by(&:npm_name)
        end

        private

        def log_warning(message)
          full = "[cocoapods-lynx-library] #{message}"
          if defined?(Pod::UI)
            Pod::UI.warn(full)
          else
            warn(full)
          end
        end

        def pod_specs_for_target(context, target_label)
          targets = context.umbrella_targets
          targets = targets.select { |t| t.cocoapods_target_label == target_label } if target_label
          targets.flat_map(&:specs).uniq(&:name)
        end

        def pod_source_root(sandbox, pod_name)
          path = sandbox.pod_dir(pod_name)
          return nil unless path && File.directory?(path)
          File.realpath(path)
        end

        # Build a LibraryInfo for an installed pod. The source dir is derived from
        # the pod's source_files (the manifest sourceDir is ignored in pods mode).
        def native_library_from_pod(spec, pod_root, manifest_file)
          json = JSON.parse(File.read(manifest_file))
          ios = json.dig('platforms', 'ios')
          return nil if ios.nil?
          raise "Invalid ios platform entry in #{manifest_file}" unless ios.is_a?(Hash)

          source_dir = source_dir_from_spec(spec, pod_root)
          node_api_addons = parse_pod_node_api_addons(ios['nodeApiAddons'], spec, manifest_file)
          LibraryInfo.new(spec.root.name, pod_root, manifest_file, source_dir, nil, node_api_addons)
        rescue JSON::ParserError => e
          raise "Failed to parse #{manifest_file}: #{e.message}"
        end

        # Resolve where the pod's Objective-C/Swift sources live from its
        # source_files patterns, falling back to the pod root.
        def source_dir_from_spec(spec, pod_root)
          patterns = Array(spec.attributes_hash['source_files'])
          ios_attrs = spec.attributes_hash['ios']
          patterns += Array(ios_attrs['source_files']) if ios_attrs.is_a?(Hash)
          roots = patterns.map { |pattern| source_pattern_root(pattern, pod_root) }.compact.uniq
          roots.length == 1 ? roots.first : pod_root
        end

        # Reduce a glob like "common/**/*.{h,m}" to its fixed directory prefix,
        # resolved against the pod root.
        def source_pattern_root(pattern, pod_root)
          fixed = pattern.to_s.split(/[*?\[\]{}]/).first.to_s
          fixed = File.dirname(fixed) unless fixed.end_with?('/') || fixed.empty?
          path = File.expand_path(fixed, pod_root)
          File.directory?(path) ? path : nil
        end

        # Parse nodeApiAddons for an installed pod. Unlike node_modules, the pod is
        # already in the graph, so podspecPath is ignored and podName defaults to
        # the resolved pod name.
        def parse_pod_node_api_addons(addons, spec, manifest_file)
          return [] if addons.nil?
          raise "platforms.ios.nodeApiAddons in #{manifest_file} must be an array" unless
            addons.is_a?(Array)

          addons.each_with_index.map do |addon, index|
            raise "platforms.ios.nodeApiAddons[#{index}] in #{manifest_file} must be an object" unless
              addon.is_a?(Hash)

            name = addon['name']
            validate_addon_name(name, "platforms.ios.nodeApiAddons[#{index}].name", manifest_file)
            pod_name = addon['podName'] || spec.root.name
            addon_use_header = addon['addonUseHeader'] || 'addon_use.h'
            validate_addon_use_header(addon_use_header,
                                      "platforms.ios.nodeApiAddons[#{index}].addonUseHeader",
                                      manifest_file)
            required = addon.key?('required') ? !!addon['required'] : true
            NodeApiAddonInfo.new(name.strip, pod_name, nil, addon_use_header, required)
          end
        end


        def node_modules_dirs(start_dir)
          dirs = []
          current = File.expand_path(start_dir)
          6.times do
            candidate = File.join(current, 'node_modules')
            dirs << candidate if File.directory?(candidate)
            parent = File.dirname(current)
            break if parent == current
            current = parent
          end
          dirs.uniq
        end

        def manifest_files(node_modules)
          manifests = []
          Dir.children(node_modules).sort.each do |name|
            next if name.start_with?('.')
            path = File.join(node_modules, name)
            next unless File.directory?(path)
            if name.start_with?('@')
              Dir.children(path).sort.each do |scoped_name|
                add_manifest(manifests, File.join(path, scoped_name))
              end
            else
              add_manifest(manifests, path)
            end
          end
          manifests
        end

        def add_manifest(manifests, package_dir)
          manifest = File.join(package_dir, 'lynx.lib.json')
          manifests << manifest if File.file?(manifest)
        end

        def parse_manifest(manifest_file)
          json = JSON.parse(File.read(manifest_file))
          ios = json.dig('platforms', 'ios')
          return nil if ios.nil?
          raise "Invalid ios platform entry in #{manifest_file}" unless ios.is_a?(Hash)

          package_dir = File.dirname(manifest_file)
          package_realpath = File.realpath(package_dir)
          source_dir_name = ios['sourceDir'] || 'ios'
          source_dir = resolve_package_path(package_realpath, source_dir_name, manifest_file, 'sourceDir')
          raise "iOS sourceDir '#{source_dir_name}' does not exist for #{manifest_file}" unless
            File.directory?(source_dir)

          podspec_path = if ios['podspecPath']
                           resolve_package_path(package_realpath, ios['podspecPath'], manifest_file, 'podspecPath')
                         else
                           path = podspec_files(source_dir, package_realpath).first
                           validate_package_path(package_realpath, path, manifest_file, 'podspecPath') if path
                         end
          raise "No iOS podspec found for #{manifest_file}" unless
            podspec_path && File.file?(podspec_path)

          node_api_addons = parse_node_api_addons(ios['nodeApiAddons'], package_realpath,
                                                  podspec_path, manifest_file)

          npm_name = File.basename(package_dir)
          parent_name = File.basename(File.dirname(package_dir))
          npm_name = "#{parent_name}/#{npm_name}" if parent_name.start_with?('@')
          LibraryInfo.new(npm_name, package_dir, manifest_file, source_dir, podspec_path,
                          node_api_addons)
        rescue JSON::ParserError => e
          raise "Failed to parse #{manifest_file}: #{e.message}"
        end

        def add_pod_once(podfile, added_pods, pod_name, pod_path)
          key = [pod_name, pod_path]
          return if added_pods.include?(key)

          podfile.pod pod_name, :path => pod_path
          added_pods << key
        end

        def parse_node_api_addons(addons, package_realpath, default_podspec_path, manifest_file)
          return [] if addons.nil?
          raise "platforms.ios.nodeApiAddons in #{manifest_file} must be an array" unless
            addons.is_a?(Array)

          addons.each_with_index.map do |addon, index|
            raise "platforms.ios.nodeApiAddons[#{index}] in #{manifest_file} must be an object" unless
              addon.is_a?(Hash)

            name = addon['name']
            validate_addon_name(name, "platforms.ios.nodeApiAddons[#{index}].name", manifest_file)
            addon_podspec_path = if addon['podspecPath']
                                   resolve_package_path(package_realpath, addon['podspecPath'],
                                                        manifest_file,
                                                        "nodeApiAddons[#{index}].podspecPath")
                                 else
                                   default_podspec_path
                                 end
            raise "No iOS Node-API addon podspec found for #{manifest_file}" unless
              addon_podspec_path && File.file?(addon_podspec_path)

            pod_name = addon['podName'] || pod_name_from_podspec(addon_podspec_path)
            addon_use_header = addon['addonUseHeader'] || 'addon_use.h'
            validate_addon_use_header(addon_use_header,
                                      "platforms.ios.nodeApiAddons[#{index}].addonUseHeader",
                                      manifest_file)
            NodeApiAddonInfo.new(name.strip, pod_name, addon_podspec_path, addon_use_header)
          end
        end

        def validate_addon_name(name, field_name, manifest_file)
          raise "Missing #{field_name} in #{manifest_file}" if name.nil? || name.strip.empty?

          normalized = name.strip
          return if normalized.length <= 128 && !normalized.include?('..') &&
                    normalized.match?(ADDON_NAME_PATTERN)

          raise "Invalid #{field_name} '#{name}' in #{manifest_file}; " \
                'expected a C identifier matching [A-Za-z_][A-Za-z0-9_]*'
        end

        def validate_addon_use_header(header, field_name, manifest_file)
          raise "Missing #{field_name} in #{manifest_file}" if header.nil? || header.strip.empty?

          normalized = header.strip
          return if !normalized.start_with?('/') && !normalized.include?('..') &&
                    normalized.match?(/\A[A-Za-z0-9_\.\/-]+\z/)

          raise "Invalid #{field_name} '#{header}' in #{manifest_file}"
        end

        def pod_name_from_podspec(podspec_path)
          content = File.read(podspec_path)
          match = content.match(/\.name\s*=\s*['"]([^'"]+)['"]/)
          raise "Unable to read pod name from #{podspec_path}" unless match
          match[1]
        end

        def resolve_package_path(package_realpath, configured_path, manifest_file, field_name)
          path = File.expand_path(configured_path, package_realpath)
          validate_package_path(package_realpath, path, manifest_file, field_name, configured_path)
        end

        def validate_package_path(package_realpath, path, manifest_file, field_name, configured_path = path)
          path = File.realpath(path) if File.exist?(path)
          return path if package_path?(package_realpath, path)

          raise "iOS #{field_name} '#{configured_path}' must stay within package directory for #{manifest_file}"
        end

        def package_path?(package_realpath, path)
          path == package_realpath || path.start_with?("#{package_realpath}#{File::SEPARATOR}")
        end

        def podspec_files(source_dir, package_realpath)
          files = []
          dirs = [source_dir]
          until dirs.empty?
            dir = dirs.pop
            Dir.children(dir).each do |name|
              path = File.join(dir, name)
              if File.symlink?(path)
                files << path if name.end_with?('.podspec')
              elsif File.directory?(path)
                dirs << path if package_path?(package_realpath, File.realpath(path))
              elsif File.file?(path) && name.end_with?('.podspec')
                files << path
              end
            end
          end
          files.sort
        end

        def source_files(source_dir)
          Dir[File.join(source_dir, '**/*.{h,m,mm,swift}')].sort
        end

        def scan_source_file(file)
          content = File.read(file)
          components = []
          content.scan(/@implementation\s+([A-Za-z_][A-Za-z0-9_]*)(.*?)(?=@implementation|\z)/m) do
            |class_name, body|
            body.scan(/LYNX_LAZY_REGISTER_UI\(\s*@?"([^"]+)"\s*\)/) do |name|
              components << ComponentInfo.new(:ui, name.first, class_name)
            end
            body.scan(/LYNX_LAZY_REGISTER_SHADOW_NODE\(\s*@?"([^"]+)"\s*\)/) do |name|
              components << ComponentInfo.new(:shadow_node, name.first, class_name)
            end
            body.scan(/LYNX_LAZY_REGISTER_RENDERER_HOST\(\s*@?"([^"]+)"\s*\)/) do |name|
              components << ComponentInfo.new(:renderer_host, name.first, class_name)
            end
          end
          content.scan(/@LynxElement\(\s*@?"([^"]+)"\s*\)\s*@implementation\s+([A-Za-z_][A-Za-z0-9_]*)/) do
            |name, class_name|
            components << ComponentInfo.new(:ui, name, class_name)
          end
          content.scan(/@LynxNativeModule\(\s*@?"([^"]+)"\s*\)\s*@(implementation|interface)\s+([A-Za-z_][A-Za-z0-9_]*)/) do
            |name, _declaration, class_name|
            components << ComponentInfo.new(:native_module, name, class_name)
          end
          content.scan(/@(?:LynxService|LynxServiceRegister)\(\s*([A-Za-z_][A-Za-z0-9_]*)\s*,\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)/) do
            |class_name, protocol_name|
            components << ComponentInfo.new(:service, protocol_name, class_name)
          end
          components.uniq { |component| [component.kind, component.name, component.class_name] }
        end

        def header_source
          <<~HEADER
            // Generated by cocoapods-lynx-library. Do not edit.
            #import <Foundation/Foundation.h>

            @class LynxConfig;

            @interface #{REGISTRY_CLASS_NAME} : NSObject
            - (void)setup:(LynxConfig *)config;
            @end
          HEADER
        end

        def implementation_source(components)
          lines = components.map do |component|
            class_expr = "NSClassFromString(@\"#{component.class_name}\")"
            case component.kind
            when :ui
              "  if (#{class_expr}) { [config registerUI:#{class_expr} withName:@\"#{component.name}\"]; }"
            when :shadow_node
              "  if (#{class_expr}) { [config registerShadowNode:#{class_expr} withName:@\"#{component.name}\"]; }"
            when :renderer_host
              "  if (#{class_expr}) { [config.componentRegistry registerRendererHost:#{class_expr} withName:@\"#{component.name}\"]; }"
            when :native_module
              "  if (#{class_expr}) { [config registerModule:#{class_expr} withName:@\"#{component.name}\"]; }"
            end
          end.compact.join("\n")

          <<~IMPL
            // Generated by cocoapods-lynx-library. Do not edit.
            #import "#{REGISTRY_CLASS_NAME}.h"
            #import <Lynx/LynxConfig.h>

            extern void LynxGeneratedNodeAPIAddonUse(void);

            @implementation #{REGISTRY_CLASS_NAME}
            - (void)setup:(LynxConfig *)config {
              LynxGeneratedNodeAPIAddonUse();
              if (config == nil) {
                return;
              }
            #{lines}
            }
            @end
          IMPL
        end

        def addon_use_source(node_api_addons)
          includes = node_api_addons.uniq { |addon| [addon.pod_name, addon.addon_use_header] }
                                    .map do |addon|
            header = "#{addon.pod_name}/#{addon.addon_use_header}"
            <<~INCLUDE.chomp
              #if __has_include(<#{header}>)
              #include <#{header}>
              #endif
            INCLUDE
          end.join("\n\n")
          addon_register_declarations = node_api_addons.map(&:name).uniq.map do |name|
            "extern void _napi_register_xx_#{name}(void);"
          end.join("\n")
          addon_register_calls = node_api_addons.map(&:name).uniq.map do |name|
            "    _napi_register_xx_#{name}();"
          end.join("\n")
          bridge_setup = if node_api_addons.empty?
                           ''
                         else
                           <<~SETUP.chomp
                             #include <mutex>
                             #include <LynxWeakNodeAPI/primjs_weak_node_api_installer.h>
                             #include <PrimJS/primjs_weak_node_api_provider.h>

                             namespace {
                             extern "C" {
                             #{addon_register_declarations}
                             }

                             void LynxSetupWeakNodeAPI() {
                               static std::once_flag once;
                               std::call_once(once, []() {
                                 PrimJSInstallWeakNodeApiRawPtrHostProvider(
                                     PrimJSGetWeakNodeApiRawPtrHost);
                                 SetupWeakNodeApiEnv();
                             #{addon_register_calls}
                               });
                             }
                             }  // namespace
                           SETUP
                         end
          setup_call = node_api_addons.empty? ? '' : '  LynxSetupWeakNodeAPI();'

          <<~SOURCE
            // Generated by cocoapods-lynx-library. Do not edit.
            #{bridge_setup}

            #{includes}

            #ifdef __cplusplus
            extern "C" {
            #endif
            void LynxGeneratedNodeAPIAddonUse(void) {
            #{setup_call}
            }
            #ifdef __cplusplus
            }
            #endif
          SOURCE
        end

        def podspec_source(node_api_addons = [])
          addon_pod_names = node_api_addons.map(&:pod_name).uniq
          addon_dependencies = addon_pod_names.map { |pod_name| "  s.dependency '#{pod_name}'" }
                                            .join("\n")
          node_api_dependencies = if node_api_addons.empty?
                                    []
                                  else
                                    [
                                      'LynxWeakNodeAPI/primjs_bridge',
                                      'PrimJS/napi/adapter'
                                    ]
                                  end
          node_api_dependency_lines = node_api_dependencies.map do |pod_name|
            "  s.dependency '#{pod_name}'"
          end.join("\n")
          node_api_xcconfig = if node_api_addons.empty?
                                ''
                              else
                                [
                                  '  s.pod_target_xcconfig = {',
                                  '    \'HEADER_SEARCH_PATHS\' => \'$(inherited) "${PODS_ROOT}/LynxWeakNodeAPI/packages/weak-node-api/headers"\'',
                                  '  }'
                                ].join("\n")
                              end
          <<~PODSPEC
            Pod::Spec.new do |s|
              s.name = 'LynxLibraryRegistry'
              s.version = '0.1.0'
              s.summary = 'Generated Lynx library registry.'
              s.homepage = 'https://github.com/lynx-family/lynx'
              s.license = 'Apache-2.0'
              s.author = 'Lynx'
              s.source = { :path => '.' }
              s.source_files = '#{REGISTRY_CLASS_NAME}.{h,m}', '#{ADDON_USE_SOURCE_NAME}'
              s.dependency 'Lynx'
            #{node_api_dependency_lines}
            #{addon_dependencies}
            #{node_api_xcconfig}
              s.requires_arc = true
            end
          PODSPEC
        end
      end
    end
  end
end
