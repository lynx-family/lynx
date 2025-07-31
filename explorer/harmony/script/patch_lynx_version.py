# -*- coding: UTF-8 -*-
# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import os
import json

CUR_DIR = os.path.dirname(os.path.abspath(__file__))
HARMONY_DIR = os.path.join(CUR_DIR, '..')

def patch_build_profile(module_path, version, commit_hash):
    # replace version & commit hash in module_path/build-profile.json5
    build_profile_file = os.path.join(module_path, "build-profile.json5")
    with open(build_profile_file, "r") as f:
        content = f.read()

    content = content.replace('VERSION_PLACE_HOLDER_TO_DISALLOW_PUBLISH_BY_ACCIDENT', version, 1)
    content = content.replace('COMMIT_HASH_PLACE_HOLDER_TO_DISALLOW_PUBLISH_BY_ACCIDENT', commit_hash, 1)
    with open(build_profile_file, "w") as f:
        f.write(content)

    print(f'patch {module_path}/build-profile.json5 done')
    # print replaced file
    with open(build_profile_file, "r") as f:
        print(f.read())


def patch_oh_package(version):
    print(f'start patch version use {version}')
    # replace version in HARMONY_DIR/parameter.json
    with open(os.path.join(HARMONY_DIR, 'parameter.json'), 'r') as f:
        parameter = json.load(f)
    parameter["dependencies"]["lynx_version"] = version
    with open(os.path.join(HARMONY_DIR, 'parameter.json'), 'w') as f:
        json.dump(parameter, f, indent=2)
    print(f'patch {HARMONY_DIR}/parameter.json done')
    # print replaced file
    with open(os.path.join(HARMONY_DIR, 'parameter.json'), 'r') as f:
        print(f.read())

def patch_lynx_version(version, commit_hash, module_path):
    patch_oh_package(version)
    patch_build_profile(module_path, version, commit_hash)