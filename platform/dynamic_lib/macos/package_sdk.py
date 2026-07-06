#!/usr/bin/env python3
# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import os
import sys
import zipfile


def _zip_dir(path, zip_file, prefix):
  print(f"_zip_dir: {path}")
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


try:
  destination_file = sys.argv[1]
  root_build_dir = sys.argv[2]
  icudtl = sys.argv[3]

  print(f"destination_file: {destination_file}")
  print(f"root_build_dir: {root_build_dir}")
  print(f"icudtl: {icudtl}")

  if os.path.exists(destination_file):
    os.remove(destination_file)
  if not os.path.exists(root_build_dir):
    raise Exception(f"root_build_dir: {root_build_dir} does not exist")
  if not os.path.exists(icudtl):
    raise Exception(f"icudtl: {icudtl} does not exist")

  dylib = os.path.join(root_build_dir, 'libLynx_clay.dylib')
  resource_bundles = os.path.join(root_build_dir, 'resource_bundles')
  include_dir = os.path.join(root_build_dir, 'include')
  if not os.path.exists(dylib):
    raise Exception(f"libLynx_clay.dylib: {dylib} does not exist")
  if not os.path.isdir(resource_bundles):
    raise Exception(f"resource_bundles: {resource_bundles} is not a directory")
  if not os.path.isdir(include_dir):
    raise Exception(f"include: {include_dir} is not a directory")

  zip_file = zipfile.ZipFile(destination_file, 'w', zipfile.ZIP_DEFLATED)
  _zip_dir(include_dir, zip_file, 'include')
  zip_file.write(dylib, 'lib/libLynx_clay.dylib')
  _zip_dir(resource_bundles, zip_file, 'bundles')
  zip_file.write(icudtl, 'data/icudtl.dat')
  zip_file.close()
  print(f"Successfully packaged macOS Lynx clay SDK to {destination_file}")
except Exception as e:
  print(f"Failed to package macOS Lynx clay SDK: {e}")
  sys.exit(1)
