# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import os, sys, shutil

from blinkpy.common import path_finder
path_finder.add_bindings_scripts_dir_to_sys_path()
path_finder.add_build_scripts_dir_to_sys_path()

from known_modules import MODULES

known_modules = {
  'test': 'core/runtime/bindings/napi/worklet/test',
  'gen_test': 'third_party/binding/gen_test/jsbridge/bindings/gen_test',
  'worklet': 'core/runtime/bindings/napi/worklet',
}

hardcoded_includes = {
    'worklet': {
        'common_headers': [
            'base/include/log/logging.h',
        ],
        'LepusElement': [
            'core/renderer/worklet/lepus_element.h',
        ],
        'LepusLynx': [
            'core/runtime/bindings/napi/worklet/napi_frame_callback.h',
            'core/runtime/bindings/napi/worklet/napi_func_callback.h',
            'core/runtime/bindings/napi/worklet/napi_lepus_element.h',
            'core/renderer/worklet/lepus_element.h',
            'core/renderer/worklet/lepus_lynx.h'
        ],
        'LepusComponent': [
            'core/runtime/bindings/napi/worklet/napi_frame_callback.h',
            'core/runtime/bindings/napi/worklet/napi_func_callback.h',
            'core/runtime/bindings/napi/worklet/napi_lepus_element.h',
            'core/renderer/worklet/lepus_element.h',
            'core/renderer/worklet/lepus_component.h'
        ],
         'LepusGesture': [
            'core/renderer/worklet/lepus_gesture.h',
        ],
    },
    'test': {
        'common_headers': [
            'base/include/log/logging.h',
        ],
        'TestContext': [
            'core/runtime/bindings/napi/worklet/test/test_context.h',
        ],
        'TestElement': [
            'core/runtime/bindings/napi/worklet/test/test_element.h',
            'core/runtime/bindings/napi/worklet/test/napi_test_context.h',
        ],
    },
    'gen_test': {
        'common_headers': [
            'base/include/log/logging.h',
        ],
        'buffering_disabled_methods': [
            'finish',
        ],
        'GenTestCommandBuffer': [
            'jsbridge/bindings/gen_test/napi_async_object.h',
            'jsbridge/bindings/gen_test/napi_test_context.h',
            'third_party/binding/gen_test/test_async_object.h',
            'third_party/binding/gen_test/test_context.h',
        ],
        'TestContext': [
            'third_party/binding/gen_test/test_context.h',
        ],
        'TestElement': [
            'jsbridge/bindings/gen_test/napi_test_context.h',
            'third_party/binding/gen_test/test_element.h',
        ],
    },
}


for key in known_modules:
    MODULES[key] = known_modules[key].split('/')

from blinkpy.bindings.bindings_tests import bindings_tests

def main(argv):
    """Runs Blink bindings IDL compiler on test IDL files and compares the
    results with reference files.

    Please execute the script whenever changes are made to the compiler
    (this is automatically done as a presubmit script),
    and submit changes to the test results in the same patch.
    This makes it easier to track and review changes in generated code.
    """

    res = bindings_tests(os.path.dirname(os.path.dirname(os.path.dirname(os.path.realpath(__file__)))), False, True, hardcoded_includes)
    script_path = os.path.join(os.getcwd(), os.path.dirname(sys.argv[0]))
    # TODO(zhengsenyao): format changed files
    # os.system("pushd %s/../../../../../ > /dev/null && source tools/envsetup.sh && git lynx format --changed && popd > /dev/null" % script_path)
    return res

if __name__ == '__main__':
    sys.exit(main(sys.argv))
