#!/usr/bin/env python3
#
# Copyright 2013 The Flutter Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
# Copyright 2023 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

"""
A top level harness to run all unit-tests in a specific engine build.
"""

import argparse
import glob
import errno
import multiprocessing
import os
import re
import subprocess
import sys
import time
import csv
import xvfb

SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))
BUILDROOT_DIR = os.path.abspath(
    os.path.join(os.path.realpath(__file__), '..', '..', '..', '..')
)
OUT_DIR = os.path.join(BUILDROOT_DIR, 'out')
GOLDEN_DIR = os.path.join(BUILDROOT_DIR, 'lynx', 'clay', 'testing', 'resources')
FONTS_DIR = os.path.join(
    BUILDROOT_DIR, 'lynx', 'clay', 'third_party', 'txt', 'third_party', 'fonts'
)
ROBOTO_FONT_PATH = os.path.join(FONTS_DIR, 'Roboto-Regular.ttf')

COVERAGE_SCRIPT = os.path.join(
    BUILDROOT_DIR, 'lynx', 'clay', 'build', 'generate_coverage.py'
)

FML_UNITTESTS_FILTER = '--gtest_filter=-*TimeSensitiveTest*'

g_executables = []


def print_divider(char='='):
  print('\n')
  for _ in range(4):
    print(''.join([char for _ in range(80)]))
  print('\n')

def run_cmd(
    cmd, forbidden_output=None, expect_failure=False, env=None, **kwargs
):
  if forbidden_output is None:
    forbidden_output = []

  command_string = ' '.join(cmd)

  print_divider('>')
  print('Running command "%s"' % command_string)

  start_time = time.time()
  stdout_pipe = sys.stdout if not forbidden_output else subprocess.PIPE
  stderr_pipe = sys.stderr if not forbidden_output else subprocess.PIPE
  process = subprocess.Popen(
      cmd,
      stdout=stdout_pipe,
      stderr=stderr_pipe,
      env=env,
      universal_newlines=True,
      **kwargs
  )
  stdout, stderr = process.communicate()
  end_time = time.time()

  if process.returncode != 0 and not expect_failure:
    print_divider('!')

    print(
        'Failed Command:\n\n%s\n\nExit Code: %d\n' %
        (command_string, process.returncode)
    )

    if stdout:
      print('STDOUT: \n%s' % stdout)

    if stderr:
      print('STDERR: \n%s' % stderr)

    print_divider('!')

    raise Exception(
        'Command "%s" exited with code %d.' %
        (command_string, process.returncode)
    )

  if stdout or stderr:
    print(stdout)
    print(stderr)

  for forbidden_string in forbidden_output:
    if (stdout and forbidden_string in stdout) or (stderr and
                                                   forbidden_string in stderr):
      raise Exception(
          'command "%s" contained forbidden string %s' %
          (command_string, forbidden_string)
      )

  print_divider('<')
  print(
      'Command run successfully in %.2f seconds: %s' %
      (end_time - start_time, command_string)
  )


def is_mac():
  return sys.platform == 'darwin'


def is_linux():
  return sys.platform.startswith('linux')


def is_windows():
  return sys.platform.startswith(('cygwin', 'win'))


def executable_suffix():
  return '.exe' if is_windows() else ''


def find_executable_path(path):
  if os.path.exists(path):
    return path

  if is_windows():
    exe_path = path + '.exe'
    if os.path.exists(exe_path):
      return exe_path

    bat_path = path + '.bat'
    if os.path.exists(bat_path):
      return bat_path

  raise Exception('Executable %s does not exist!' % path)


def build_engine_executable_command(
    build_dir, executable_name, flags=None, coverage=False, gtest=False
):
  if flags is None:
    flags = []

  unstripped_exe = os.path.join(build_dir, 'exe.unstripped', executable_name)
  # We cannot run the unstripped binaries directly when coverage is enabled.
  if is_linux() and os.path.exists(unstripped_exe) and not coverage:
    # Use unstripped executables in order to get better symbolized crash
    # stack traces on Linux.
    executable = unstripped_exe
  else:
    executable = find_executable_path(os.path.join(build_dir, executable_name))

  if coverage:
    global g_executables
    g_executables.append(executable)
    coverage_flags = [
        '-t', executable, '-o',
        os.path.join(build_dir, 'coverage', executable_name), '-f', 'html'
    ]
    updated_flags = ['--args=%s' % ' '.join(flags)]
    test_command = [COVERAGE_SCRIPT] + coverage_flags + updated_flags
  else:
    test_command = [executable] + flags
    if gtest:
      gtest_parallel = os.path.join(
          BUILDROOT_DIR, 'third_party', 'gtest-parallel', 'gtest-parallel'
      )
      test_command = ['python3', gtest_parallel] + test_command

  return test_command


def run_engine_executable( # pylint: disable=too-many-arguments
    build_dir,
    executable_name,
    executable_filter,
    flags=None,
    cwd=BUILDROOT_DIR,
    forbidden_output=None,
    expect_failure=False,
    coverage=False,
    extra_env=None,
    gtest=False
):
  if executable_filter is not None and executable_name not in executable_filter:
    print('Skipping %s due to filter.' % executable_name)
    return

  if flags is None:
    flags = []
  if forbidden_output is None:
    forbidden_output = []
  if extra_env is None:
    extra_env = {}

  unstripped_exe = os.path.join(build_dir, 'exe.unstripped', executable_name)
  env = os.environ.copy()
  if is_linux():
    env['LD_LIBRARY_PATH'] = build_dir
    env['VK_DRIVER_FILES'] = os.path.join(build_dir, 'vk_swiftshader_icd.json')
    if os.path.exists(unstripped_exe):
      try:
        os.symlink(
            os.path.join(build_dir, 'lib.unstripped', 'libvulkan.so.1'),
            os.path.join(build_dir, 'exe.unstripped', 'libvulkan.so.1')
        )
      except OSError as err:
        if err.errno == errno.EEXIST:
          pass
        else:
          raise
  elif is_mac():
    env['DYLD_LIBRARY_PATH'] = build_dir
  else:
    env['PATH'] = build_dir + ':' + env['PATH']

  print('Running %s in %s' % (executable_name, cwd))

  test_command = build_engine_executable_command(
      build_dir,
      executable_name,
      flags=flags,
      coverage=coverage,
      gtest=gtest,
  )

  env['FLUTTER_BUILD_DIRECTORY'] = build_dir
  for key, value in extra_env.items():
    env[key] = value

  try:
    run_cmd(
        test_command,
        cwd=cwd,
        forbidden_output=forbidden_output,
        expect_failure=expect_failure,
        env=env
    )
  except:
    # The LUCI environment may provide a variable containing a directory path
    # for additional output files that will be uploaded to cloud storage.
    # If the command generated a core dump, then run a script to analyze
    # the dump and output a report that will be uploaded.
    luci_test_outputs_path = os.environ.get('FLUTTER_TEST_OUTPUTS_DIR')
    core_path = os.path.join(cwd, 'core')
    if luci_test_outputs_path and os.path.exists(core_path) and os.path.exists(
        unstripped_exe):
      dump_path = os.path.join(
          luci_test_outputs_path, '%s_%s.txt' % (executable_name, sys.platform)
      )
      print('Writing core dump analysis to %s' % dump_path)
      subprocess.call([
          os.path.join(
              BUILDROOT_DIR, 'clay', 'testing', 'analyze_core_dump.sh'
          ),
          BUILDROOT_DIR,
          unstripped_exe,
          core_path,
          dump_path,
      ])
      os.unlink(core_path)
    raise

shuffle_flags = [
    '--gtest_repeat=2',
    '--gtest_shuffle',
]


def run_cc_tests(build_dir, executable_filter, coverage, capture_core_dump):
  print('Running Engine Unit-tests.')

  if capture_core_dump and is_linux():
    import resource  # pylint: disable=import-outside-toplevel
    resource.setrlimit(
        resource.RLIMIT_CORE, (resource.RLIM_INFINITY, resource.RLIM_INFINITY)
    )

  repeat_flags = [
      '--repeat=2',
  ]

  def make_test(name, flags=None, extra_env=None):
    if flags is None:
      flags = repeat_flags
    if extra_env is None:
      extra_env = {}
    return (name, flags, extra_env)

  unittests = [
      make_test('common_cpp_core_unittests'),
      make_test('common_cpp_unittests'),
      make_test('memory_unittests'),
      make_test('testing_unittests'),
      make_test('clay_unittests'),
      make_test('gfx_unittests'),
  ]

  if is_linux():
    flow_flags = [
        '--golden-dir=%s' % GOLDEN_DIR,
        '--font-file=%s' % ROBOTO_FONT_PATH,
    ]
    icu_flags = [
        '--icu-data-file-path=%s' % os.path.join(build_dir, 'icudtl.dat')
    ]
    unittests += [
        make_test('flow_unittests', flags=repeat_flags + ['--'] + flow_flags),
        # https://github.com/flutter/flutter/issues/36296
        make_test('txt_unittests', flags=repeat_flags + ['--'] + icu_flags),
    ]
  else:
    flow_flags = ['--gtest_filter=-PerformanceOverlayLayer*.Gold']
    unittests += [
        make_test('flow_unittests', flags=repeat_flags + flow_flags),
    ]

  for test, flags, extra_env in unittests:
    run_engine_executable(
        build_dir,
        test,
        executable_filter,
        flags,
        coverage=coverage,
        extra_env=extra_env,
        cwd = build_dir,
        gtest=True
    )

def ensure_debug_unopt_sky_packages():
  variant_out_dir = os.path.join(OUT_DIR, 'host_debug_unopt')
  message = []
  message.append('gn --runtime-mode debug --unopt --no-lto')
  message.append('ninja -C %s clay/sky/packages' % variant_out_dir)
  final_message = "%s doesn't exist. Please run the following commands: \n%s" % (
      variant_out_dir, '\n'.join(message)
  )
  assert os.path.exists(variant_out_dir), final_message


def ensure_ios_tests_are_built(ios_out_dir):
  """Builds the engine variant and the test dylib containing the XCTests"""
  tmp_out_dir = os.path.join(OUT_DIR, ios_out_dir)
  ios_test_lib = os.path.join(tmp_out_dir, 'libios_test_flutter.dylib')
  message = []
  message.append(
      'gn --ios --is-debug --no-lto --simulator'
  )
  message.append('autoninja -C %s ios_test_flutter' % ios_out_dir)
  final_message = "%s or %s doesn't exist. Please run the following commands: \n%s" % (
      ios_out_dir, ios_test_lib, '\n'.join(message)
  )
  assert os.path.exists(tmp_out_dir
                       ) and os.path.exists(ios_test_lib), final_message


def assert_expected_xcode_version():
  """Checks that the user has a version of Xcode installed"""
  version_output = subprocess.check_output(['xcodebuild', '-version'])
  match = re.match(r'Xcode (\d+)', version_output)
  message = 'Xcode must be installed to run the iOS embedding unit tests'
  assert match, message

def make_coverage_summary(build_dir):
  if len(g_executables) == 0:
    return

  print('Merge all coverage into summary.json')
  coverage_flags = ['-t']
  coverage_flags.extend(g_executables)
  coverage_flags.extend(['-o', os.path.join(build_dir, 'coverage'), '-f', 'all', '-m'])
  test_command = [COVERAGE_SCRIPT] + coverage_flags
  run_cmd(test_command)

def main():
  parser = argparse.ArgumentParser()
  all_types = [ 'engine', 'benchmarks' ]

  parser.add_argument(
      '--variant',
      dest='variant',
      action='store',
      default='host_debug_unopt',
      help='The engine build variant to run the tests for.'
  )
  parser.add_argument(
      '--type',
      type=str,
      default='all',
      help='A list of test types, default is "all" (equivalent to "%s")' %
      (','.join(all_types))
  )
  parser.add_argument(
      '--engine-filter',
      type=str,
      default='',
      help='A list of engine test executables to run.'
  )
  parser.add_argument(
      '--coverage',
      action='store_true',
      default=None,
      help='Generate coverage reports for each unit test framework run.'
  )
  parser.add_argument(
      '--engine-capture-core-dump',
      dest='engine_capture_core_dump',
      action='store_true',
      default=False,
      help='Capture core dumps from crashes of engine tests.'
  )
  parser.add_argument(
      '--use-sanitizer-suppressions',
      dest='sanitizer_suppressions',
      action='store_true',
      default=False,
      help='Provide the sanitizer suppressions lists to the via environment to the tests.'
  )

  args = parser.parse_args()

  if args.type == 'all':
    types = all_types
  else:
    types = args.type.split(',')

  build_dir = os.path.join(OUT_DIR, args.variant)
  assert os.path.exists(
      build_dir
  ), 'Build variant directory %s does not exist!' % build_dir

  if args.sanitizer_suppressions:
    assert is_linux() or is_mac(
    ), 'The sanitizer suppressions flag is only supported on Linux and Mac.'
    file_dir = os.path.dirname(os.path.abspath(__file__))
    command = [
        'env', '-i', 'bash', '-c',
        'source {}/sanitizer_suppressions.sh >/dev/null && env'
        .format(file_dir)
    ]
    process = subprocess.Popen(command, stdout=subprocess.PIPE)
    for line in process.stdout:
      key, _, value = line.decode('utf8').strip().partition('=')
      os.environ[key] = value
    process.communicate()  # Avoid pipe deadlock while waiting for termination.

  engine_filter = args.engine_filter.split(',') if args.engine_filter else None
  if 'engine' in types:
    run_cc_tests(
        build_dir, engine_filter, args.coverage, args.engine_capture_core_dump
    )

  if args.coverage:
    make_coverage_summary(build_dir)


if __name__ == '__main__':
  sys.exit(main())
