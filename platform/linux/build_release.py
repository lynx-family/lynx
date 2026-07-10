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
    'use_flutter_cxx = true',
    'is_debug = false',
]


def run_command(command):
  print('run command:', ' '.join(command))
  subprocess.run(command, check=True)


def sha256_file(path):
  digest = hashlib.sha256()
  with path.open('rb') as file:
    for chunk in iter(lambda: file.read(1024 * 1024), b''):
      digest.update(chunk)
  return digest.hexdigest()


def write_checksum(path):
  checksum_path = path.with_name(f'{path.name}.sha256')
  checksum_path.write_text(f'{sha256_file(path)}  {path.name}\n',
                           encoding='utf-8')
  return checksum_path


def main():
  parser = argparse.ArgumentParser(
      description='Build and package the Lynx SDK for Linux.')
  parser.add_argument('--target-cpu', required=True)
  parser.add_argument('--build-dir', default='out/Default')
  args = parser.parse_args()

  build_dir = Path(args.build_dir)
  target = 'platform/linux:package_sdk'
  artifact = build_dir / f'lynx_sdk_linux_{args.target_cpu}.zip'
  gn_args = [
      f'target_cpu = "{args.target_cpu}"',
      *COMMON_GN_ARGS,
  ]

  run_command([
      'buildtools/gn/gn',
      'gen',
      str(build_dir),
      f'--root-target=//{target}',
      '--args=' + ' '.join(gn_args),
  ])
  run_command([
      'buildtools/ninja/ninja', '-C', str(build_dir), target
  ])

  if not artifact.is_file():
    raise FileNotFoundError(f'Expected artifact does not exist: {artifact}')
  checksum = write_checksum(artifact)
  print(f'Built Linux SDK: {artifact}')
  print(f'Wrote checksum: {checksum}')


if __name__ == '__main__':
  main()
