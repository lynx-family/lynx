#!/usr/bin/env python3
# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import argparse
import re


def replace_pod_version(component, version, podfile):
    pattern = re.compile(r"pod\s+['\"](\S+)['\"]\s*,\s*['\"]([~>\d.\-a-zA-Z]+)['\"]")
    replace_pattern = re.compile(
        rf"(pod\s+['\"]{re.escape(component)}['\"]\s*,\s*['\"])([~>\d.\-a-zA-Z]+)(['\"])",
        re.MULTILINE
    )
    
    success_write = 0
    with open(podfile, 'r', encoding='utf8') as f:
        content = f.read()
    results = pattern.findall(content)
    for res in results:
        if res[0] == component:
            content = replace_pattern.sub(rf"\g<1>{version}\3", content)
            success_write = 1
        
    with open(podfile, 'w') as f:
        f.write(content)
        
    if success_write:
        print(f"Successfully wrote the new version {version} of {component} to {podfile} ")
    else:
        print(f"Failed to write the new version {version} of {component} to {podfile} ")

def insert_source_to_podfile(source, podfile):
    with open(podfile, "r") as f:
        content = f.read()

    new_content = f"source \"{source}\"\n" + content
    
    with open(podfile, "w") as f:
        f.write(new_content)
    print(f"Successfully wrote the source {source} to {podfile} ")

def main():
    """
    usage: 'python3 process_podfile.py --component <component> --version <version> --podfile <podfile_path>'
    like : python3 process_podfile.py  --component Lynx --version 3.4.0 --podfile ios/HelloLynxObjc/Podfile
    """
    parser = argparse.ArgumentParser()
    parser.add_argument('--action', type=str, choices=["replace_pod_version", "insert_pod_source"], help='the action for processing podfile', required=True)
    parser.add_argument('--component', type=str, help='the component need to be processed', required=False)
    parser.add_argument('--version', type=str, help='the component version', required=False)
    parser.add_argument('--pod_source', type=str, help='the pod source need to be insert to the top of podfile', required=False)
    parser.add_argument('--podfile', type=str, help='the pod file', required=True)

    args = parser.parse_args()
    if args.action == 'replace_pod_version':
        if not args.component or not args.version or not args.podfile:
            print('Please specify --component, --version, --podfile')
            exit(1)
        else:
            replace_pod_version(args.component, args.version, args.podfile)
    
    elif args.action == 'insert_pod_source':
        if not args.pod_source or not args.podfile:
            print('Please specify --pod_source, --podfile')
            exit(1)
        else:
            insert_source_to_podfile(args.pod_source, args.podfile)
 

if __name__ == '__main__':
    main()
