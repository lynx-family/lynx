#!/usr/bin/env python3
# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import os
import sys
import zipfile


def _zip_dir(path, zip_file, prefix):
  print(f'_zip_dir: {path}')
  path = path.rstrip('/\\')
  for root, directories, files in os.walk(path):
    directories[:] = [
        directory for directory in directories
        if not os.path.islink(os.path.join(root, directory))
    ]
    for file in files:
      full_path = os.path.join(root, file)
      if os.path.islink(full_path):
        continue
      zip_file.write(full_path, os.path.join(root.replace(path, prefix), file))


def _zip_file_if_exists(zip_file, path, archive_path):
  if os.path.exists(path):
    zip_file.write(path, archive_path)


try:
  destination_file = sys.argv[1]
  root_build_dir = sys.argv[2]
  icudtl = sys.argv[3]

  print(f'destination_file: {destination_file}')
  print(f'root_build_dir: {root_build_dir}')
  print(f'icudtl: {icudtl}')

  if os.path.exists(destination_file):
    os.remove(destination_file)
  if not os.path.exists(root_build_dir):
    raise Exception(f'root_build_dir: {root_build_dir} does not exist')
  if not os.path.exists(icudtl):
    raise Exception(f'icudtl: {icudtl} does not exist')

  include_dir = os.path.join(root_build_dir, 'include')
  liblynx = os.path.join(root_build_dir, 'liblynx.so')
  liblynx_in_lib_dir = os.path.join(root_build_dir, 'lib', 'liblynx.so')
  if not os.path.isdir(include_dir):
    raise Exception(f'include: {include_dir} is not a directory')
  if not os.path.exists(liblynx) and os.path.exists(liblynx_in_lib_dir):
    liblynx = liblynx_in_lib_dir
  if not os.path.exists(liblynx):
    raise Exception(
        f'liblynx.so: neither {liblynx} nor {liblynx_in_lib_dir} exists')

  with zipfile.ZipFile(destination_file, 'w',
                       zipfile.ZIP_DEFLATED) as zip_file:
    _zip_dir(include_dir, zip_file, 'include')
    zip_file.write(liblynx, 'lib/liblynx.so')
    zip_file.write(icudtl, 'data/icudtl.dat')

    resource_bundles = os.path.join(root_build_dir, 'resource_bundles')
    if os.path.isdir(resource_bundles):
      _zip_dir(resource_bundles, zip_file, 'bundles')

    _zip_file_if_exists(
        zip_file, os.path.join(root_build_dir, 'lynx_core', 'lynx_core.js'),
        'lynx_core.js')
    _zip_file_if_exists(
        zip_file,
        os.path.join(root_build_dir, 'lynx_core', 'lynx_core_dev.js'),
        'lynx_core_dev.js')

  print(f'Successfully packaged Linux Lynx SDK to {destination_file}')
except Exception as error:
  print(f'Failed to package Linux Lynx SDK: {error}')
  sys.exit(1)
