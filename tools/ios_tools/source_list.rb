#!/usr/bin/env ruby
# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
#
# Podfile helper for consuming a `Podfile.flatten` source list produced by
# lynx/tools/ios_tools/prepare_cocoapods_sources.py. Each entry in that file
# looks like:
#
#   <source_url>:<PodName>:<Version> pod
#
# Callers set $sources_file to the absolute path of Podfile.flatten before
# invoking `use_pod`.

def reverse_split(str, sep, max_splits = nil)
  str = str.reverse
  sep = sep.reverse
  str.split(sep, max_splits).map(&:reverse).reverse
end

def load_sources(file_path)
  sources = {}
  File.foreach(file_path) do |raw_line|
    line = raw_line.strip
    next if line.empty? || line.start_with?('#')
    source_line, type = line.split(' ')
    next unless type == 'pod'
    source, name, version = reverse_split(source_line, ':', 3)
    sources[name] = { :source => source, :version => version }
  end
  sources
end

$sources = nil

def pod_with_version(name)
  if $sources.nil?
    if $sources_file.nil? || !File.exist?($sources_file)
      raise "source_list.rb: $sources_file must be set to an existing Podfile.flatten path"
    end
    $sources = load_sources($sources_file)
  end
  pod_name = name.split('/')[0]
  unless $sources.key?(pod_name)
    puts "pod #{pod_name} does not exist in #{$sources_file}"
    exit(1)
  end
  version = $sources[pod_name][:version]
  env_key = version.sub('$', '')
  version = ENV[env_key] if version.start_with?('$') && ENV.include?(env_key)
  puts "use pod #{name} => #{version}"
  [name, version]
end

def use_pod(name, *args)
  pod(*pod_with_version(name), *args)
end
