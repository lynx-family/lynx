#!/usr/bin/env python3
# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import os
import sys

def directory_really_is_buildtools(directory):
  return os.path.exists(os.path.join(directory, 'gn')) and os.path.exists(os.path.join(directory, 'ninja'))

def get_buildtools_path():
  """Search for buildtools """
  # First look if buildtools is already in PATH
  for i in os.environ['PATH'].split(os.pathsep):
    if i.rstrip(os.sep).endswith('buildtools'):
      if directory_really_is_buildtools(i):
        return i

  # If it's not in PATH, look upward up to root.
  root_dir = os.path.dirname(os.path.abspath(__file__))
  previous_dir = os.path.abspath(__file__)
  while root_dir and root_dir != previous_dir:
    if directory_really_is_buildtools(os.path.join(root_dir, 'buildtools')):
      i = os.path.join(root_dir, 'buildtools')
      return i
    previous_dir = root_dir
    root_dir = os.path.dirname(root_dir)
  print('Failed to find buildtools', file=sys.stderr)
  return None

def main():
  get_buildtools_path()

if __name__ == '__main__':
  main()