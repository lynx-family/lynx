# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import os
import pathlib

# This is the portal of all LLDB custom scripts provided by Lynx.
#
# 1. Debug LynxExplorer in Xcode.
# LynxExplorer -> Scheme -> LLDB Init File has been automatically configured.
# You don't need to do anything.
#
# 2. Other cases, expand the following command with full path of this file.
#    command script import {full_path_of_this_file}
#
# To globally activate the scripts, add command above to `~/.lldbinit` file,
# and create one if not exists. All subsequent LLDB processes will load these
# custom scripts when they start.
#
# You can also run the command manually in each LLDB session to only load the
# scripts in the current LLDB process.


def __lldb_init_module(debugger, internal_dict):
    this_dir = os.path.dirname(os.path.abspath(__file__))
    lynx_dir = pathlib.Path(this_dir).parent.parent

    # All Lynx custom LLDB scripts managed by this list.
    scripts = [
        'base/include/linked_hash_map_lldb.py',
        'base/include/vector_lldb.py',
        'base/include/value/lynx_value_lldb.py',
        'base/include/value/base_string_lldb.py',
    ]

    for f in scripts:
        parts = f.split('/')
        path = os.path.join(lynx_dir, *parts)
        print(f"Load LLDB debugger script at {path}")
        debugger.HandleCommand(f"command script import {path}")
