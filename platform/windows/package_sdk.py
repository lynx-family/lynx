#!/usr/bin/env python3
# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import os
import sys
import zipfile

def _zip_dir(path, zip_file, prefix):
  print(f"_zip_dir: {path}")
  path = path.rstrip('/\\')
  for root, directories, files in os.walk(path):
    for file in files:
      if os.path.islink(os.path.join(root, file)):
        continue
      zip_file.write(
          os.path.join(root, file),
          os.path.join(root.replace(path, prefix), file)
      )

try:
    # Get arguments passed from GN
    destination_file = sys.argv[1]
    root_build_dir = sys.argv[2]
    icudtl = sys.argv[3]

    print(f"destination_file: {destination_file}")
    print(f"root_build_dir: {root_build_dir}")
    print(f"icudtl: {icudtl}")

    if (os.path.exists(destination_file)):
        os.remove(destination_file)
    if (not os.path.exists(root_build_dir)):
        print(f"root_build_dir: {root_build_dir} does not exist")
        raise Exception(f"root_build_dir: {root_build_dir} does not exist")
    if (not os.path.exists(icudtl)):
        print(f"icudtl: {icudtl} does not exist")
        raise Exception(f"icudtl: {icudtl} does not exist")
    if (not os.path.exists(os.path.join(root_build_dir, 'lynx.dll'))):
        print(f"lynx.dll: {os.path.join(root_build_dir, 'lynx.dll')} does not exist")
        raise Exception(f"lynx.dll: {os.path.join(root_build_dir, 'lynx.dll')} does not exist")
    if (not os.path.isdir(os.path.join(root_build_dir, 'include')) or not os.path.exists(os.path.join(root_build_dir, 'include'))):
        print(f"include: {os.path.join(root_build_dir, 'include')} is not a directory")
        raise Exception(f"include: {os.path.join(root_build_dir, 'include')} is not a directory")
    

    zip_file = zipfile.ZipFile(destination_file, 'w', zipfile.ZIP_DEFLATED)

    # package includes
    _zip_dir(os.path.join(root_build_dir, 'include'), zip_file, 'include')

    # package lynx.dll
    zip_file.write(os.path.join(root_build_dir, 'lynx.dll'), 'lib/lynx.dll')
    zip_file.write(os.path.join(root_build_dir, 'lynx.dll.lib'), 'lib/lynx.dll.lib')

    # package lynx_core
    zip_file.write(os.path.join(root_build_dir, 'lynx_core/lynx_core.js'), 'lynx_core.js')
    zip_file.write(os.path.join(root_build_dir, 'lynx_core/lynx_core_dev.js'), 'lynx_core_dev.js')

    # package icudtl.dat
    zip_file.write(icudtl, 'data/icudtl.dat')

    zip_file.close()
    print(f"Successfully packaged SDK to {destination_file}")
except Exception as e:
    print(f"Failed to package SDK: {e}")
    sys.exit(1)
