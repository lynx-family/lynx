#!/usr/bin/env python3
# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import argparse
import os
import shutil
import subprocess
import json
from skip_pod_lint import skip_pod_lint

def run_command(command, check=True):
    # When the "command" is a multi-line command, only the status of the last line of the command is checked.
    # Therefore, it is necessary to add "set -e" to ensure that any error in any line of the command will cause the script to exit immediately.
    command = 'set -e\n' + command

    print(f'run command: {command}')
    res = subprocess.run(['bash', '-c', command], stderr=subprocess.STDOUT, check=check, text=True)


def replace_lynx_version(version):
    lines = []
    with open('build_overrides/darwin.gni', 'r') as f:
        lines = f.readlines()
    with open('build_overrides/darwin.gni', 'w') as f:
        for line in lines:
            if 'lynx_version =' in line:
                print(f'new version: {version}')
                f.write(f'lynx_version = "{version}"\n')
            else:
                f.write(f'{line}')


def copy_podspec(src_dir, dest_dir):
    for filename in os.listdir(src_dir):
        if filename.endswith('.podspec'):
            src_file = os.path.join(src_dir, filename)
            dest_file = os.path.join(dest_dir, filename)
            shutil.copy(src_file, dest_file)
            print(f'Copied: {src_file} to {dest_file}')


def generate_zip_file(src_dir, tag, component):
    for filename in os.listdir(src_dir):
        if filename.endswith('.podspec'):
            podspec_name = filename.split('.')[0]
            if component == podspec_name or component == 'all':
                print(f'Generating zip file for {podspec_name}')
                run_command(f'export PACKAGE_ENV=prod && geniospkg --output_type both --repo {podspec_name} --tag {tag} --cache_path .')

def get_enable_trace_param(version: str) -> str:
    """
    Returns '--enable-trace' if the version ends with '-dev', otherwise returns an empty string.
    Args:
        version (str): The version string to check.
    Returns:
        str: '--enable-trace' if version ends with '-dev', else ''.
    """
    if version.endswith('-dev'):
        return '--enable-trace'
    return ''

def prepare_cocoapods_publish_source(version, tag, component):
    root_path = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

    # change to root path
    os.chdir(root_path)

    print('Start prepare cocoapods publish source')
    print('1. Replace lynx version')
    replace_lynx_version(version)

    print('2. Generate podspec files')
    run_command(f'python3 tools/ios_tools/generate_podspec_scripts_by_gn.py --root {root_path} {get_enable_trace_param(version)}')

    print('3. Generate lynx_core.js')
    run_command(f'python3 tools/js_tools/build.py --platform ios --release_output platform/darwin/ios/JSAssets/release/lynx_core.js --dev_output platform/darwin/ios/lynx_devtool/assets/lynx_core_dev.js --version {version}')

    print('4. Generate zip files')
    generate_zip_file(root_path, tag, component)

def use_local_pod_source(component):
    with open(f"{component}.podspec.json",'r',encoding='utf8') as f:
        content = json.load(f)
    version = content["version"]
    source_url = f"file:{os.getcwd()}/{component}-{version}.zip"
    
    content["source"] = {"http": source_url}
    with open(f"{component}.podspec.json","w",encoding='utf8') as f:
        json.dump(content, f, indent=4)

def create_local_pod_source(local_pod_source_name):
    run_command(f'mkdir ./{local_pod_source_name}')
    run_command(f'cd {local_pod_source_name} && git init && git commit --allow-empty --message "Initial commit."')
    run_command(f'bundle exec pod repo add {local_pod_source_name} file://{os.getcwd()}/{local_pod_source_name}')

def publish_component_to_local_source(component,local_pod_source_name):
    file_name = f"{component}.podspec.json"
    if not os.path.exists(file_name):
        run_command(f'bundle exec pod ipc spec {component}.podspec > {file_name}')
    use_local_pod_source(component)
    run_command(f'bundle exec pod repo push {local_pod_source_name} {component}.podspec.json --local-only --skip-import-validation --allow-warnings --skip-tests --verbose')

def run_pod_lint(component):
    print(f'Start pod lint to {component} podspec')
    run_command("bundle exec pod repo add-cdn trunk https://cdn.cocoapods.org/")

    local_pod_source_name = 'local_specs'
    publish_to_local(local_pod_source_name, component)
    skip_pod_lint('private')
    
    if component == 'all':
        # skip lint and push pod to local pod source
        pod_lint_component('LynxBase',local_pod_source_name)
        pod_lint_component('Lynx',local_pod_source_name)
        pod_lint_component('BaseDevtool',local_pod_source_name)
        pod_lint_component('LynxDevtool',local_pod_source_name)
        pod_lint_component('LynxService',local_pod_source_name)
        pod_lint_component('XElement',local_pod_source_name)
    else:
        pod_lint_component(component,'local_pod_source_name')
        
def pod_lint_component(component, local_pod_source_name):
    # podspec.json will write the current directory path into itself
    run_command(f'bundle exec pod spec lint {component}.podspec.json --sources=trunk,{local_pod_source_name} --verbose --skip-import-validation --allow-warnings --skip-tests')

def publish_component(component, sources):
    if sources != None:
        run_command(f'COCOAPODS_TRUNK_TOKEN=$COCOAPODS_TRUNK_TOKEN bundle exec pod trunk push {component}.podspec.json --verbose --skip-import-validation --allow-warnings --skip-tests --sources={sources}')
    else:
        run_command(f'COCOAPODS_TRUNK_TOKEN=$COCOAPODS_TRUNK_TOKEN bundle exec pod trunk push {component}.podspec.json --verbose --skip-import-validation --allow-warnings --skip-tests')


def publish_to_cocoapods(component, sources):
    print(f'Start publish {component} to cocoapods')
    if component == 'all':
        # publish in order: LynxBase -> Lynx -> BaseDevtool -> LynxDevtool -> LynxService
        publish_component('LynxBase', sources)
        publish_component('Lynx', sources)
        publish_component('BaseDevtool', sources)
        publish_component('LynxDevtool', sources)
        publish_component('LynxService', sources)
        publish_component('XElement', sources)
    else:
        publish_component(component, sources)


def publish_to_local(component, local_source_name):
    create_local_pod_source(local_source_name)
    
    skip_pod_lint('private')
    if component == 'all':
        publish_component_to_local_source('LynxBase',local_source_name)
        publish_component_to_local_source('Lynx',local_source_name)
        publish_component_to_local_source('BaseDevtool',local_source_name)
        publish_component_to_local_source('LynxDevtool',local_source_name)
        publish_component_to_local_source('LynxService',local_source_name)
        publish_component_to_local_source('XElement',local_source_name)
    else:
        publish_component_to_local_source(component, local_source_name)

def main():
    """
    usage: 1. 'python3 cocoapods_publish_helper.py --prepare-source --version <version> --component <component>'
           2. 'python3 cocoapods_publish_helper.py --publish --component <component> --sources <sources>'
    like : 1. python3 publish_pod_to_cocoapods.py --prepare-source --version 0.0.1 --component Lynx
           2. python3 publish_pod_to_cocoapods.py --publish --component Lynx --sources 'https://cdn.cocoapods.org'
    """
    parser = argparse.ArgumentParser()
    parser.add_argument('--component', type=str, help='the component to publish', required=True)
    parser.add_argument(
        "--prepare-source", action="store_true", help="Prepare the source for publishing"
    )
    # When publishing a dev version, the tag does not match the version. The version is formatted as version="{tag}-dev"
    parser.add_argument('--tag', type=str, help='the release tag of lynx', required=False)
    parser.add_argument('--version', type=str, help='the pod version of lynx', required=False)
    parser.add_argument(
        "--publish", action="store_true", help="Publish to cocoapods"
    )
    parser.add_argument('--sources', type=str, help='the cocoapods sources', required=False)
    parser.add_argument('--pod_lint', action="store_true", help='Run pod lint')
    parser.add_argument('--publish_local', type=str, help='Publish pod to local source')

    args = parser.parse_args()
    if args.prepare_source:
        prepare_cocoapods_publish_source(args.version, args.tag, args.component)
    elif args.publish:
        publish_to_cocoapods(args.component, args.sources)
    elif args.pod_lint:
        run_pod_lint(args.component)
    elif args.publish_local:
        publish_to_local(args.component, args.publish_local)
    else:
        print('Please specify --prepare-source , --publish, --pod_lint or --publish_local')
        exit(1)


if __name__ == '__main__':
    main()
