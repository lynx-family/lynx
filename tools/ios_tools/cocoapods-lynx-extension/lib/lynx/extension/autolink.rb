# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

require 'fileutils'
require 'json'
require 'pathname'

module Lynx
  module Extension
    ExtensionInfo = Struct.new(:npm_name, :package_dir, :manifest_file, :source_dir, :podspec_path)
    ComponentInfo = Struct.new(:kind, :name, :class_name)

    class Autolink
      REGISTRY_CLASS_NAME = 'LynxGeneratedExtensionRegistry'

      class << self
        def install!(podfile, options = {})
          start_dir = File.expand_path(options[:root] || Dir.pwd)
          output_dir = File.expand_path(options[:output_dir] || 'generated/lynx-extension', start_dir)
          extensions = scan(start_dir)
          extensions.each do |extension|
            pod_name = pod_name_from_podspec(extension.podspec_path)
            podfile.pod pod_name, :path => File.dirname(extension.podspec_path)
          end
          generate_registry(output_dir, extensions)
          podfile.pod 'LynxExtensionRegistry', :path => output_dir
          extensions
        end

        def scan(start_dir)
          node_modules_dirs(start_dir).flat_map do |node_modules|
            manifest_files(node_modules).map { |manifest| parse_manifest(manifest) }.compact
          end.sort_by(&:npm_name)
        end

        def generate_registry(output_dir, extensions)
          FileUtils.mkdir_p(output_dir)
          components = extensions.flat_map { |extension| scan_components(extension.source_dir) }
          File.write(File.join(output_dir, "#{REGISTRY_CLASS_NAME}.h"), header_source)
          File.write(File.join(output_dir, "#{REGISTRY_CLASS_NAME}.m"),
                     implementation_source(components))
          File.write(File.join(output_dir, 'LynxExtensionRegistry.podspec'), podspec_source)
          components
        end

        def scan_components(source_dir)
          source_files(source_dir).flat_map { |file| scan_source_file(file) }
        end

        private

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
          manifest = File.join(package_dir, 'lynx.ext.json')
          manifests << manifest if File.file?(manifest)
        end

        def parse_manifest(manifest_file)
          json = JSON.parse(File.read(manifest_file))
          ios = json.dig('platforms', 'ios')
          return nil if ios.nil?
          raise "Invalid ios platform entry in #{manifest_file}" unless ios.is_a?(Hash)

          package_dir = File.dirname(manifest_file)
          source_dir_name = ios['sourceDir'] || 'ios'
          source_dir = File.expand_path(source_dir_name, package_dir)
          raise "iOS sourceDir '#{source_dir_name}' does not exist for #{manifest_file}" unless
            File.directory?(source_dir)

          podspec_path = if ios['podspecPath']
                           File.expand_path(ios['podspecPath'], package_dir)
                         else
                           Dir[File.join(source_dir, '**/*.podspec')].sort.first
                         end
          raise "No iOS podspec found for #{manifest_file}" unless
            podspec_path && File.file?(podspec_path)

          npm_name = File.basename(package_dir)
          parent_name = File.basename(File.dirname(package_dir))
          npm_name = "#{parent_name}/#{npm_name}" if parent_name.start_with?('@')
          ExtensionInfo.new(npm_name, package_dir, manifest_file, source_dir, podspec_path)
        rescue JSON::ParserError => e
          raise "Failed to parse #{manifest_file}: #{e.message}"
        end

        def pod_name_from_podspec(podspec_path)
          content = File.read(podspec_path)
          match = content.match(/\.name\s*=\s*['"]([^'"]+)['"]/)
          raise "Unable to read pod name from #{podspec_path}" unless match
          match[1]
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
          content.scan(/@?LynxAutolinkUI\(\s*@?"([^"]+)"\s*\)\s*@implementation\s+([A-Za-z_][A-Za-z0-9_]*)/) do
            |name, class_name|
            components << ComponentInfo.new(:ui, name, class_name)
          end
          content.scan(/@?LynxAutolinkNativeModule\(\s*@?"([^"]+)"\s*\)\s*@(implementation|interface)\s+([A-Za-z_][A-Za-z0-9_]*)/) do
            |name, _declaration, class_name|
            components << ComponentInfo.new(:native_module, name, class_name)
          end
          content.scan(/@?LynxAutolinkService\(\s*([A-Za-z_][A-Za-z0-9_]*)\s*,\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)/) do
            |class_name, protocol_name|
            components << ComponentInfo.new(:service, protocol_name, class_name)
          end
          content.scan(/@?LynxServiceRegister\(\s*([A-Za-z_][A-Za-z0-9_]*)\s*,\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)/) do
            |class_name, protocol_name|
            components << ComponentInfo.new(:service, protocol_name, class_name)
          end
          components.uniq { |component| [component.kind, component.name, component.class_name] }
        end

        def header_source
          <<~HEADER
            // Generated by cocoapods-lynx-extension. Do not edit.
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
            // Generated by cocoapods-lynx-extension. Do not edit.
            #import "#{REGISTRY_CLASS_NAME}.h"
            #import <Lynx/LynxConfig.h>

            @implementation #{REGISTRY_CLASS_NAME}
            - (void)setup:(LynxConfig *)config {
              if (config == nil) {
                return;
              }
            #{lines}
            }
            @end
          IMPL
        end

        def podspec_source
          <<~PODSPEC
            Pod::Spec.new do |s|
              s.name = 'LynxExtensionRegistry'
              s.version = '0.1.0'
              s.summary = 'Generated Lynx extension registry.'
              s.homepage = 'https://github.com/lynx-family/lynx'
              s.license = 'Apache-2.0'
              s.author = 'Lynx'
              s.source = { :path => '.' }
              s.source_files = '#{REGISTRY_CLASS_NAME}.{h,m}'
              s.dependency 'Lynx'
              s.requires_arc = true
            end
          PODSPEC
        end
      end
    end
  end
end
