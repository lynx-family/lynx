project(
  'jsoncpp',
  'cpp',
  version : '1.8.1',
  default_options : [
    'buildtype=release',
    'warning_level=1'],
  license : 'Public Domain',
  meson_version : '>= 0.41.1')

jsoncpp_ver_arr = meson.project_version().split('.')
jsoncpp_major_version = jsoncpp_ver_arr[0]
jsoncpp_minor_version = jsoncpp_ver_arr[1]
jsoncpp_patch_version = jsoncpp_ver_arr[2]

jsoncpp_cdata = configuration_data()
jsoncpp_cdata.set('JSONCPP_VERSION', meson.project_version())
jsoncpp_cdata.set('JSONCPP_VERSION_MAJOR', jsoncpp_major_version)
jsoncpp_cdata.set('JSONCPP_VERSION_MINOR', jsoncpp_minor_version)
jsoncpp_cdata.set('JSONCPP_VERSION_PATCH', jsoncpp_patch_version)

jsoncpp_gen_sources = configure_file(
  input : 'src/lib_json/version.h.in',
  output : 'version.h',
  configuration : jsoncpp_cdata,
  install : true,
  install_dir : join_paths(get_option('prefix'), get_option('includedir'), 'json')
)

jsoncpp_headers = [
  'include/json/allocator.h',
  'include/json/assertions.h',
  'include/json/autolink.h',
  'include/json/config.h',
  'include/json/features.h',
  'include/json/forwards.h',
  'include/json/json.h',
  'include/json/reader.h',
  'include/json/value.h',
  'include/json/writer.h']
jsoncpp_include_directories = include_directories('include')

install_headers(
  jsoncpp_headers,
  subdir : 'json')

jsoncpp_lib = library(
  'jsoncpp',
  [ jsoncpp_gen_sources,
    jsoncpp_headers,
    'src/lib_json/json_tool.h',
    'src/lib_json/json_reader.cpp',
    'src/lib_json/json_value.cpp',
    'src/lib_json/json_writer.cpp'],
  soversion : 18,
  install : true,
  include_directories : jsoncpp_include_directories)

import('pkgconfig').generate(
  libraries : jsoncpp_lib,
  version : meson.project_version(),
  name : 'jsoncpp',
  filebase : 'jsoncpp',
  description : 'A C++ library for interacting with JSON')

# for libraries bundling jsoncpp
declare_dependency(
  include_directories : jsoncpp_include_directories,
  link_with : jsoncpp_lib,
  version : meson.project_version(),
  sources : jsoncpp_gen_sources)

# tests
python = import('python3').find_python()

jsoncpp_test = executable(
  'jsoncpp_test',
  [ 'src/test_lib_json/jsontest.cpp',
    'src/test_lib_json/jsontest.h',
    'src/test_lib_json/main.cpp'],
  include_directories : jsoncpp_include_directories,
  link_with : jsoncpp_lib,
  install : false)
test(
  'unittest_jsoncpp_test',
  jsoncpp_test)

jsontestrunner = executable(
  'jsontestrunner',
  'src/jsontestrunner/main.cpp',
  include_directories : jsoncpp_include_directories,
  link_with : jsoncpp_lib,
  install : false)
test(
  'unittest_jsontestrunner',
  python,
  args : [
    '-B',
    join_paths(meson.current_source_dir(), 'test/runjsontests.py'),
    jsontestrunner,
    join_paths(meson.current_source_dir(), 'test/data')]
  )
