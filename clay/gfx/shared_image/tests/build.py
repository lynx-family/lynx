#!/usr/bin/env python3
# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
"""Build standalone tests from real source units and an existing GN output."""
# cspell:ignore ffunction fdata lvulkan
import argparse
import hashlib
import json
import shlex
import subprocess
from pathlib import Path

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--root', type=Path, required=True, help='Checkout containing clay/ or lynx/clay/')
parser.add_argument('--gn-out', type=Path, required=True)
parser.add_argument('--out', type=Path, required=True)
parser.add_argument('--platform', choices=['android', 'macosx', 'iphoneos', 'iphonesimulator'], required=True)
parser.add_argument('--egl-include', type=Path, help='EGL/GLES headers for macOS SurfaceTexture host tests')
parser.add_argument('--surface-texture-host', action='store_true',
                    help='Also build SurfaceTexture state tests on macOS (requires a Skity GN output)')
args = parser.parse_args()
root, gn, out = (p.resolve() for p in (args.root, args.gn_out, args.out))
here = Path(__file__).resolve().parent
prefix = 'lynx/' if (root / 'lynx/clay').exists() else ''
source_root = root / prefix
out.mkdir(parents=True, exist_ok=True)
is_android = args.platform == 'android'
if args.surface_texture_host and args.platform != 'macosx':
    parser.error('--surface-texture-host requires --platform macosx')
receipt = {'platform': args.platform, 'sources': {}, 'commands': []}
wrapper = root / 'tools/env.sh'
ninja = [str(wrapper), 'ninja'] if wrapper.exists() else ['ninja']
sdk = compiler = None
target = []
if not is_android:
    sdk = subprocess.check_output(['xcrun', '--sdk', args.platform, '--show-sdk-path'], text=True).strip()
    compiler = subprocess.check_output(['xcrun', '--sdk', args.platform, '--find', 'clang++'], text=True).strip()
    if args.platform != 'macosx':
        target = ['-target', 'arm64-apple-ios15.0' + ('-simulator' if args.platform == 'iphonesimulator' else '')]


def run(command):
    receipt['commands'].append(command)
    result = subprocess.run(command, cwd=gn)
    if result.returncode:
        raise SystemExit(result.returncode)


def compile_gn(name, target_path, replacement=None, extra_flags=()):
    global compiler
    path = 'obj/' + prefix + target_path
    lines = subprocess.check_output(ninja + ['-C', str(gn), '-t', 'commands', path], cwd=root, text=True)
    command = shlex.split(next(line for line in lines.splitlines() if ' -c ' in line and (' -o ' + path) in line))
    if Path(command[0]).name in ('sccache', 'ccache'):
        command = command[1:]
    if is_android:
        compiler = command[0]
    else:
        command[0] = compiler
        command[command.index('-isysroot') + 1] = sdk
        command += ['-Wno-error=unknown-warning-option']
        if not target:
            command = ['-mmacosx-version-min=13.0' if item.startswith('-mmacosx-version-min=') else item for item in command]
        if target:
            i = command.index('-arch'); del command[i:i + 2]
            command = [item.replace('-DOS_OSX=1', '-DOS_IOS=1').replace('-DSKITY_PLATFORM_MACOS', '-DSKITY_PLATFORM_IOS')
                       for item in command if not item.startswith('-mmacosx-version-min=')]
            command += target
    if replacement:
        command[command.index('-c') + 1] = str(replacement)
    # Standalone links discard unrelated renderer/platform implementations,
    # including when the configured GN output uses Debug compilation flags.
    command += ['-ffunction-sections', '-fdata-sections', *extra_flags]
    for flag, suffix in [('-o', '.o'), ('-MF', '.d')]:
        command[command.index(flag) + 1] = str(out / (name + suffix))
    run(command)
    return str(out / (name + '.o'))


api = compile_gn('api', 'clay/ui/public/ui.clay.o')
if args.surface_texture_host and '-DENABLE_SKITY' not in receipt['commands'][0]:
    parser.error('--surface-texture-host requires a Skity GN output')
logs = [compile_gn(name, 'base/src/log/base_log.' + name + '.o')
        for name in ['logging', 'log_stream', 'alog_wrapper']]
representation = compile_gn('representation', 'clay/gfx/shared_image/shared_image.shared_image_representation.o')
backing = compile_gn('backing', 'clay/gfx/shared_image/shared_image.shared_image_backing.o')
blocking = compile_gn('blocking', 'clay/gfx/shared_image/shared_image.shared_image_representation.o',
                      here / 'blocking_fence_tests.cc')
if is_android:
    fence_target = 'clay/gfx/shared_image/shared_image.android_egl_image_representation.o'
    fence = compile_gn('fence', fence_target)
    tests = compile_gn('tests', fence_target, here / 'egl_tests.cc')
    backend = 'skity' if '-DENABLE_SKITY' in receipt['commands'][0] else 'skia'
    fixture = compile_gn('backend_fixture', fence_target, here / ('egl_' + backend + '_fixture.cc'))
    binders = [compile_gn(name, 'clay/common/graphics/gl/gl_scoped_binder.' + name + '.o')
               for name in ['gl_api_common', 'scoped_texture_binder', 'scoped_framebuffer_binder']]
    accessor = compile_gn('accessor', 'clay/gfx/shared_image/shared_image.shared_image_sink_accessor.o')
    sink = compile_gn('sink', 'clay/gfx/shared_image/shared_image.shared_image_sink.o')
    run([compiler, '--target=aarch64-linux-android26', '-Wl,--gc-sections', '-static-libstdc++',
         blocking, representation, backing, *logs, '-llog', '-ldl', '-o', str(out / 'blocking-tests')])
    run([compiler, '--target=aarch64-linux-android26', '-Wl,--gc-sections', '-static-libstdc++',
         tests, fixture, fence, api, representation, backing, accessor, sink, *logs, *binders,
         '-lEGL', '-lGLESv2', '-landroid', '-llog', '-ldl', '-o', str(out / 'egl-tests')])
    vk_fence = compile_gn('vk_fence', 'clay/gfx/shared_image/shared_image.vulkan_image_hardwarebuffer_representation.o')
    vk_helper = compile_gn('vk_fence_helper', 'clay/gfx/vulkan/gfx.vulkan_fence_helper.o')
    vk_pool = compile_gn('vk_pool', 'clay/gfx/vulkan/gfx.vulkan_command_pool_helper.o')
    unique_fd = compile_gn('unique_fd', fence_target, source_root / 'base/src/fml/unique_fd.cc')
    vk_tests = compile_gn('vk_tests', fence_target, here / 'vulkan_tests.cc')
    vk_image = compile_gn('vk_image', fence_target, source_root / 'clay/gfx/vulkan/vulkan_image.cc')
    run([compiler, '--target=aarch64-linux-android26', '-Wl,--gc-sections', '-static-libstdc++',
         vk_tests, vk_image, vk_fence, vk_helper, vk_pool, unique_fd, fence, api, representation, backing, *logs,
         '-lEGL', '-lGLESv2', '-landroid', '-llog', '-ldl', '-o', str(out / 'vulkan-tests')])
    vk_driver = compile_gn('vk_driver', 'clay/gfx/vulkan/gfx.vulkan_helper.o')
    native_loader = compile_gn('native_loader', fence_target,
                              source_root / 'clay/fml/platform/posix/native_library_posix.cc')
    vk_device_tests = compile_gn('vk_device_tests', fence_target, here / 'vulkan_device_tests.cc')
    run([compiler, '--target=aarch64-linux-android26', '-Wl,--gc-sections', '-static-libstdc++',
         vk_device_tests, vk_fence, vk_helper, vk_driver, native_loader, unique_fd, fence, api, *logs,
         '-lvulkan', '-lEGL', '-lGLESv2', '-landroid', '-llog', '-ldl',
         '-o', str(out / 'vulkan-device-tests')])
else:
    common = [compiler, '-isysroot', sdk, *target, '-std=c++17', '-fobjc-arc',
              '-ffunction-sections', '-fdata-sections', '-I' + str(source_root), '-I' + str(root)]
    for name, source in [('fence', source_root / 'clay/gfx/shared_image/mtl_fence_sync.mm'),
                         ('tests', here / 'metal_tests.mm'), ('symbols', here / 'optional_exports.cc')]:
        run(common + ['-c', str(source), '-o', str(out / (name + '.o'))])
    link = [compiler, '-isysroot', sdk, *target, '-Wl,-dead_strip', '-framework', 'Foundation', '-framework', 'Metal']
    run(link + [blocking, representation, backing, *logs, '-o', str(out / 'blocking-tests')])
    run(link + [str(out / 'tests.o'), str(out / 'fence.o'), api, '-o', str(out / 'metal-tests')])
    run(link + [str(out / 'symbols.o'), api, '-o', str(out / 'optional-exports')])
    if args.platform == 'macosx':
        path = 'clay/gfx/shared_image/shared_image.mtl_image_representation.o'
        image = compile_gn('image', path)
        tests = compile_gn('representation_tests', path, here / 'metal_representation_tests.mm')
        accessor = compile_gn('accessor', 'clay/gfx/shared_image/shared_image.shared_image_sink_accessor.o')
        sink = compile_gn('sink', 'clay/gfx/shared_image/shared_image.shared_image_sink.o')
        run(link + ['-framework', 'CoreVideo', '-framework', 'IOSurface', tests, image, representation,
                    backing, accessor, sink, api, str(out / 'fence.o'), *logs, '-o', str(out / 'representation-tests')])
    if args.surface_texture_host:
        egl_include = args.egl_include or root / 'third_party/angle/include'
        if not (egl_include / 'EGL/egl.h').is_file():
            parser.error('macOS SurfaceTexture host tests require --egl-include with EGL/GLES headers')
        fixture_header = here / 'surface_texture_host_fixture.h'
        host_flags = ['-I' + str(egl_include.resolve()), '-UENABLE_HEADLESS',
                      '-DGL_GLEXT_PROTOTYPES', '-DEGL_EGLEXT_PROTOTYPES',
                      '-include', str(fixture_header)]
        host_objects = [
            compile_gn('surface_' + name, 'clay/gfx/shared_image/shared_image.shared_image_backing.o',
                       source, host_flags)
            for name, source in [
                ('backing', source_root / 'clay/gfx/shared_image/android_surface_texture_image_backing.cc'),
                ('fence', source_root / 'clay/gfx/shared_image/android_egl_image_representation.cc'),
                ('tests', here / 'surface_texture_tests.cc')]]
        run(link + [*host_objects, representation, backing, *logs,
                    '-o', str(out / 'surface-texture-tests')])
        receipt['sources'][str(fixture_header)] = hashlib.sha256(fixture_header.read_bytes()).hexdigest()
        surface_header = source_root / 'clay/gfx/shared_image/android_surface_texture_image_backing.h'
        receipt['sources'][str(surface_header)] = hashlib.sha256(surface_header.read_bytes()).hexdigest()
    gl_backend = 'cgl' if args.platform == 'macosx' else 'eagl'
    template = 'clay/gfx/shared_image/shared_image.mtl_image_representation.o'
    gl_image = compile_gn('gl_image', template,
                          source_root / ('clay/gfx/shared_image/' + gl_backend + '_image_representation.mm'))
    gl_tests = compile_gn('gl_tests', template, here / (gl_backend + '_fence_tests.mm'))
    gl_contract = compile_gn('gl_contract', template, here / 'apple_gl_nonblocking_tests.mm')
    run(link + [gl_contract, '-framework', 'OpenGL' if args.platform == 'macosx' else 'OpenGLES',
                '-o', str(out / 'gl-contract-tests')])
    gl_binders = [compile_gn('gl_' + name, 'clay/common/graphics/gl/gl_scoped_binder.' + name + '.o')
                  for name in ['gl_api_common', 'scoped_texture_binder', 'scoped_framebuffer_binder']]
    run(link + ['-framework', 'CoreVideo', '-framework', 'IOSurface', '-framework',
                'OpenGL' if args.platform == 'macosx' else 'OpenGLES', gl_tests, gl_image,
                representation, backing, api, *gl_binders, *logs, '-o', str(out / 'gl-tests')])
for command in receipt['commands']:
    if '-c' in command:
        source = (gn / command[command.index('-c') + 1]).resolve()
        receipt['sources'][str(source)] = hashlib.sha256(source.read_bytes()).hexdigest()
for source in [here / 'api_contract.h', source_root / 'clay/public/clay.h', source_root / 'clay/gfx/shared_image/fence_sync.h',
               source_root / 'clay/gfx/shared_image/apple_gl_fence_sync.h',
               source_root / 'clay/gfx/shared_image/android_egl_image_representation.h',
               source_root / 'clay/gfx/shared_image/vulkan_image_hardwarebuffer_representation.h',
               source_root / 'clay/gfx/vulkan/vulkan_fence_helper.h',
               source_root / 'clay/gfx/shared_image/mtl_fence_sync.h', source_root / 'clay/gfx/shared_image/mtl_image_representation.h']:
    receipt['sources'][str(source)] = hashlib.sha256(source.read_bytes()).hexdigest()
(out / 'build-receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
print(out)
