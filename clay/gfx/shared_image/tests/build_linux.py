#!/usr/bin/env python3
# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
"""Build/run standalone Linux EGL fence tests, without a configured GN output."""
import argparse
import hashlib
import json
import shlex
import subprocess
from pathlib import Path

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--out', type=Path, required=True)
parser.add_argument('--cxx', default='c++')
parser.add_argument('--egl-include', type=Path, help='Optional EGL/GLES include root')
parser.add_argument('--unit-only', action='store_true', help='No EGL runtime or link libraries needed')
args = parser.parse_args()
here = Path(__file__).resolve().parent
source_root = here.parents[3]
out = args.out.resolve()
out.mkdir(parents=True, exist_ok=True)
commands = []


def run(command):
    commands.append(command)
    subprocess.run(command, check=True, timeout=60)


common = shlex.split(args.cxx) + ['-std=c++17', '-pthread', '-I' + str(source_root)]
# build/build_config.h lives at the checkout root in either source layout.
checkout_root = source_root.parent if source_root.name == 'lynx' and (
    source_root.parent / 'build/build_config.h').is_file() else source_root
if checkout_root != source_root:
    common += ['-I' + str(checkout_root)]
if args.egl_include:
    common += ['-I' + str(args.egl_include.resolve())]
names = ['linux_egl_fence_tests']
if not args.unit_only:
    names.append('linux_egl_device_tests')
for name in names:
    binary = out / name
    command = common + [str(here / (name + '.cc')), '-o', str(binary)]
    if name == 'linux_egl_device_tests':
        command += ['-lEGL', '-lGLESv2']
    run(command)
    run([str(binary)])
sources = [here / (name + '.cc') for name in names]
sources += [here.parent / 'linux_egl_fence_sync.h', here.parent / 'fence_sync.h']
(out / 'build-receipt.json').write_text(json.dumps({
    'commands': commands,
    'sources': {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in sources},
}, indent=2) + '\n')
