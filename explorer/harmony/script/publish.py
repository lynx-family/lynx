#!/usr/bin/env python3
# -*- coding: UTF-8 -*-
# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import argparse
import os
import sys
from subprocess import check_call

from build import DEFAULT_MODULES, LYNX_DIR, collect_module_config_list

PLATFORM_HARMONY_DIR = os.path.normpath(os.path.join(LYNX_DIR, 'platform', 'harmony'))

def publish(module_full_path, module):
    publish_id = os.environ.get('PUBLISH_ID')
    key_path = os.environ.get('KEY_PATH')
    if not publish_id:
        raise Exception('PUBLISH_ID not set')
    if not key_path:
        raise Exception('KEY_PATH not set')
    target_dir = f"{module_full_path}/build/default/outputs/default"
    cmd = f"ohpm publish {module}.har --publish_id {publish_id} --key_path {key_path}"
    print(f'run command {cmd}')
    check_call(cmd, shell=True, cwd=target_dir)

def main():
    parser = argparse.ArgumentParser()

    parser.add_argument("--modules", nargs="*", help="list of modules name")
    parser.add_argument("--version", type=str, help="sdk release version")
    parser.add_argument("--verbose", action="store_true", default=False, help="print all commands")

    args = parser.parse_args()

    if args.modules:
        if len(args.modules) == 1 and args.modules[0].lower() == "default":
            modules = DEFAULT_MODULES
        else:
            modules = args.modules
    else:
        modules = []

    for module in modules:
        module_config_list = collect_module_config_list(args)
        for module_config in module_config_list:
            if module_config['name'] == module:
                module_path = module_config['srcPath']
                break
        else:
            raise Exception(f'module {module} not found in build-profile.json5')
        print(f'module {module} path: {module_path}')
        module_full_path = os.path.join(PLATFORM_HARMONY_DIR, module_path)
        publish(module_full_path, module)

if __name__ == "__main__":
    sys.exit(main())
