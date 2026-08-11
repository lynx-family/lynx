# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

require 'fileutils'
require 'json'
require 'pathname'
require 'cocoapods'

module SparklingPodspecOverlays
  PODS = {
    'Sparkling' => {
      :package => '@sparklingjs/runtime',
      :ios_dir => 'ios',
      :podspec => 'Sparkling.podspec',
    },
    'SparklingMethod' => {
      :package => 'sparkling-method',
      :ios_dir => 'ios',
      :podspec => 'SparklingMethod.podspec',
    },
    'Sparkling-Router' => {
      :package => 'sparkling-navigation',
      :ios_dir => 'ios',
      :podspec => 'Sparkling-Router.podspec',
    },
  }.freeze

  LOCAL_LYNX_DEPENDENCIES = [
    'Lynx',
    'Lynx/Framework',
    'LynxBase',
    'LynxBase/Framework',
    'LynxServiceAPI',
    'LynxServiceAPI/Core',
  ].freeze

  module_function

  def generate!(lynx_root:, output_dir:, pod_names: PODS.keys)
    missing = missing_requirements(lynx_root: lynx_root, pod_names: pod_names)
    unless missing.empty?
      raise Pod::Informative, "Missing Sparkling iOS packages:\n  - #{missing.join("\n  - ")}"
    end

    FileUtils.mkdir_p(output_dir)

    pod_names.each_with_object({}) do |pod_name, generated|
      config = PODS.fetch(pod_name)
      package_root = package_root(lynx_root, config)
      ios_root = File.join(package_root, config.fetch(:ios_dir))
      source_podspec = File.join(ios_root, config.fetch(:podspec))
      package_version = JSON.parse(File.read(File.join(package_root, 'package.json'))).fetch('version')
      generated_pod_root = File.join(output_dir, pod_name)

      spec = Pod::Specification.from_file(source_podspec)
      spec_hash = spec.to_hash
      spec_hash['version'] = package_version
      spec_hash['source']['tag'] = package_version if spec_hash.dig('source', 'tag')

      normalize_spec_hash!(spec_hash)

      FileUtils.rm_rf(generated_pod_root)
      FileUtils.mkdir_p(generated_pod_root)
      FileUtils.rm_f(File.join(output_dir, "#{pod_name}.podspec.json"))
      sync_source_links!(ios_root, generated_pod_root, config.fetch(:podspec))

      generated_podspec = File.join(generated_pod_root, "#{pod_name}.podspec.json")
      File.write(generated_podspec, "#{JSON.pretty_generate(spec_hash)}\n")
      generated[pod_name] = generated_pod_root
    end
  end

  def missing_requirements(lynx_root:, pod_names: PODS.keys)
    pod_names.flat_map do |pod_name|
      config = PODS.fetch(pod_name)
      package_root = package_root(lynx_root, config)
      ios_root = File.join(package_root, config.fetch(:ios_dir))
      [
        File.join(package_root, 'package.json'),
        File.join(ios_root, config.fetch(:podspec)),
      ].reject { |path| File.exist?(path) }
    end
  end

  def package_root(lynx_root, config)
    File.join(lynx_root, 'node_modules', config.fetch(:package))
  end

  def normalize_spec_hash!(spec_hash)
    strip_local_lynx_versions!(spec_hash)
    spec_hash.fetch('subspecs', []).each do |subspec|
      normalize_spec_hash!(subspec)
    end
  end

  def sync_source_links!(source_root, generated_root, source_podspec_name)
    Dir.children(source_root).sort.each do |entry|
      next if entry == source_podspec_name

      source = File.join(source_root, entry)
      target = File.join(generated_root, entry)
      next if File.exist?(target) && File.identical?(source, target)

      FileUtils.rm_rf(target)
      relative_source = Pathname.new(source).relative_path_from(Pathname.new(generated_root))
      FileUtils.ln_s(relative_source, target)
    end
  end

  def strip_local_lynx_versions!(spec_hash)
    dependencies = spec_hash['dependencies']
    return unless dependencies

    LOCAL_LYNX_DEPENDENCIES.each do |dependency|
      dependencies[dependency] = [] if dependencies.key?(dependency)
    end
  end
end

if $PROGRAM_NAME == __FILE__
  lynx_root = File.expand_path(ARGV.shift || File.join(__dir__, '..', '..', '..', '..', '..'))
  output_dir = File.expand_path(ARGV.shift || File.join(__dir__, '..', 'generated', 'sparkling-podspecs'))
  pod_names = ARGV.empty? ? SparklingPodspecOverlays::PODS.keys : ARGV

  generated = SparklingPodspecOverlays.generate!(
    :lynx_root => lynx_root,
    :output_dir => output_dir,
    :pod_names => pod_names
  )
  generated.each_value { |path| puts path }
end
