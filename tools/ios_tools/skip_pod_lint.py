#!/usr/bin/env python3
# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import argparse
import re
import subprocess
import sys

def skip_pod_lint(source):
    command = "bundle info cocoapods-trunk" if source =='public' else  "bundle info cocoapods"
  
    output = run_command(command)
    
    pod_path = get_path(output)
    path = f"{pod_path}/lib/pod/command/trunk/push.rb" if source =='public' else  f"{pod_path}/lib/cocoapods/command/repo/push.rb"
    
    print(f"skip lint, file path {path}")
    pattern = "validate_podspec"
    with open(path,"r",encoding='utf8') as f:
        file_content = f.read()
        file_content = re.sub(pattern, r"#validate_podspec", file_content, 1)
    with open(path,"w",encoding='utf8') as f:
        f.write(file_content)

def run_command(command):
    output = subprocess.check_output(command.split()).decode("utf-8")
    return output

def get_path(content):
    match = re.search(r'Path:\s+(.*)', content)
    if match:
        path = match.group(1).strip()
        return path
    else:
        return ""

def main():
    """
    usage: 'python3 skip_pod_lint.py --source public'
    """
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", choices=['public', 'private'], default='public', help="The source of pod")

    args = parser.parse_args()
    source = args.source
    skip_pod_lint(source)
   
if __name__ == '__main__':
    sys.exit(main())