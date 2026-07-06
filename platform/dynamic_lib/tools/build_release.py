#!/usr/bin/env python3
# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import argparse
import hashlib
import subprocess
from pathlib import Path


COMMON_GN_ARGS = [
    'enable_clay_standalone = true',
    'disable_visibility_hidden = true',
    'use_ndk_static_cxx = false',
    'enable_linker_map = false',
    'enable_clay = true',
    'is_headless = true',
    'skia_enable_flutter_defines = true',
    'skia_use_dng_sdk = false',
    'skia_use_sfntly = false',
    'skia_enable_pdf = false',
    'skia_enable_svg = true',
    'enable_svg = true',
    'skia_enable_skottie = true',
    'skia_use_x11 = false',
    'skia_use_wuffs = true',
    'skia_use_expat = true',
    'skia_use_fontconfig = false',
    'clay_enable_skshaper = true',
    'skia_use_icu = true',
    'skia_gl_standard = ""',
    'allow_deprecated_api_calls = true',
    'stripped_symbols = true',
    'is_official_build = true',
    'use_clang_static_analyzer = false',
    'enable_lto = false',
    'enable_lepusng_worklet = true',
    'enable_napi_binding = true',
    'enable_inspector = true',
    'jsengine_type = "quickjs"',
    'is_debug = false',
]


def run_command(command):
  print('run command:', ' '.join(command))
  subprocess.run(command, check=True)


def sha256_file(path):
  digest = hashlib.sha256()
  with path.open('rb') as f:
    for chunk in iter(lambda: f.read(1024 * 1024), b''):
      digest.update(chunk)
  return digest.hexdigest()


def write_checksum(path):
  checksum_path = path.with_name(f'{path.name}.sha256')
  checksum_path.write_text(f'{sha256_file(path)}  {path.name}\n',
                           encoding='utf-8')
  return checksum_path


def build_config(platform, target_cpu, build_dir):
  platform_args = []
  if platform == 'macos':
    platform_args.extend([
        'desktop_enable_embedder_layer = true',
        'skia_use_metal = true',
        'shell_enable_metal = true',
        'use_flutter_cxx = false',
    ])
    return {
        'target': 'platform/dynamic_lib/macos:package_sdk',
        'shared_target':
            'platform/dynamic_lib/macos:lynx_clay_shared_library',
        'args': platform_args,
        'artifact': build_dir / f'lynx_clay_sdk_macos_{target_cpu}.zip',
        'dylib': build_dir / 'libLynx_clay.dylib',
    }
  if platform == 'linux':
    platform_args.extend([
        'use_flutter_cxx = true',
    ])
    return {
        'target': 'platform/dynamic_lib/linux:package_sdk',
        'shared_target':
            'platform/dynamic_lib/linux:lynx_clay_shared_library',
        'args': platform_args,
        'artifact': build_dir / f'lynx_clay_sdk_linux_{target_cpu}.zip',
        'dylib': None,
    }
  raise ValueError(f'Unsupported platform: {platform}')


def main():
  parser = argparse.ArgumentParser(
      description='Build and package the libLynx_clay dynamic library SDK.')
  parser.add_argument('--platform', choices=['macos', 'linux'], required=True)
  parser.add_argument('--target-cpu', required=True)
  parser.add_argument('--build-dir', default='out/Default')
  parser.add_argument(
      '--skip-codesign',
      action='store_true',
      help='Skip macOS ad-hoc signing before packaging.')
  args = parser.parse_args()

  build_dir = Path(args.build_dir)
  config = build_config(args.platform, args.target_cpu, build_dir)
  gn_args = [
      f'target_cpu = "{args.target_cpu}"',
      *COMMON_GN_ARGS,
      *config['args'],
  ]

  run_command([
      'buildtools/gn/gn',
      'gen',
      str(build_dir),
      f'--root-target=//{config["target"]}',
      '--args=' + ' '.join(gn_args),
  ])
  run_command(['buildtools/ninja/ninja', '-C', str(build_dir),
               config['shared_target']])

  dylib = config['dylib']
  if dylib is not None and not args.skip_codesign:
    run_command(['codesign', '--force', '--sign', '-', str(dylib)])
    run_command(['codesign', '--verify', '-vvv', str(dylib)])

  run_command(['buildtools/ninja/ninja', '-C', str(build_dir),
               config['target']])

  artifact = config['artifact']
  if not artifact.is_file():
    raise FileNotFoundError(f'Expected artifact does not exist: {artifact}')
  checksum = write_checksum(artifact)
  print(f'Built dynamic library SDK: {artifact}')
  print(f'Wrote checksum: {checksum}')


if __name__ == '__main__':
  main()
