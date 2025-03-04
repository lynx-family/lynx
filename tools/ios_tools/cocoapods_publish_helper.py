#!/usr/bin/env python3
# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import argparse
import os
import shutil
import subprocess
import re
from skip_pod_lint import skip_pod_lint

def run_command(command, check=True):
    # When the "command" is a multi-line command, only the status of the last line of the command is checked.
    # Therefore, it is necessary to add "set -e" to ensure that any error in any line of the command will cause the script to exit immediately.
    command = 'set -e\n' + command

    print(f'run command: {command}')
    res = subprocess.run(['bash', '-c', command], stderr=subprocess.STDOUT, check=check, text=True)


def replace_lynx_version(version):
    lines = []
    with open('lynx/build_overrides/darwin.gni', 'r') as f:
        lines = f.readlines()
    with open('lynx/build_overrides/darwin.gni', 'w') as f:
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
                run_command(f'export PACKAGE_ENV=prod && geniospkg --output_type zip --repo {podspec_name} --tag {tag} --cache_path lynx --no_json')


def prepare_cocoapods_publish_source(tag, component):
    repo_path = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    root_path = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))

    # change to root path
    os.chdir(root_path)

    print('Start prepare cocoapods publish source')
    print('1. Replace lynx version')
    replace_lynx_version(tag)

    print('2. Generate podspec files')
    run_command(f'python3 lynx/tools/ios_tools/generate_podspec_scripts_by_gn.py --root {root_path}')
    copy_podspec(repo_path, root_path)

    print('3. Generate lynx_core.js')
    os.chdir(repo_path)
    run_command(f'tools/js_tools/build.sh ios platform/darwin/ios/JSAssets/release/lynx_core.js platform/darwin/ios/lynx_devtool/assets/lynx_core_dev.js {tag}')
    os.chdir(root_path)

    print('4. Generate zip files')
    run_command('cp lynx/Gemfile ./')
    run_command('cp lynx/Gemfile.lock ./')
    generate_zip_file(root_path, tag, component)

def use_local_pod_source(component):
    pattern_source = "(\S+.source\s*=\s*{)\s*\S+\s*=>[\s\S]*?(})"
    with open(f"{component}.podspec",'r',encoding='utf8') as f:
        content=f.read()
    content = re.sub(pattern_source, f"\\1 :http => \"file:\"+__dir__+\"/{component}.zip\"\\2", content)
    with open(f"{component}.podspec","w",encoding='utf8') as f:
        f.write(content)

def create_local_pod_source(local_pod_source_name):
    run_command(f'mkdir ./{local_pod_source_name}')
    run_command(f'cd {local_pod_source_name} && git init && git commit --allow-empty --message "Initial commit."')
    run_command(f'pod repo add {local_pod_source_name} file://{os.getcwd()}/{local_pod_source_name}')

def run_pod_lint(component):
    print(f'Start pod lint to {component} podspec')
    run_command("pod repo add-cdn trunk https://cdn.cocoapods.org/")
    local_pod_source_name = 'local_specs'
    create_local_pod_source(local_pod_source_name)
    skip_pod_lint('public')
    
    # firstly publish the all pods to local source to support components dependencies 
    use_local_pod_source('Lynx')
    run_command(f'bundle exec pod ipc spec Lynx.podspec > Lynx.podspec.json')
    run_command(f'bundle exec pod repo push {local_pod_source_name} Lynx.podspec.json --local-only')
    use_local_pod_source('BaseDevtool')
    run_command(f'bundle exec pod ipc spec BaseDevtool.podspec > BaseDevtool.podspec.json')
    run_command(f'bundle exec pod repo push {local_pod_source_name} BaseDevtool.podspec.json --local-only --skip-import-validation --allow-warnings --skip-tests')
    use_local_pod_source('LynxDevtool')
    run_command(f'bundle exec pod ipc spec LynxDevtool.podspec > LynxDevtool.podspec.json')
    run_command(f'bundle exec pod repo push {local_pod_source_name} LynxDevtool.podspec.json --local-only --skip-import-validation --allow-warnings --skip-tests')
    use_local_pod_source('LynxService')
    run_command(f'bundle exec pod ipc spec LynxService.podspec > LynxService.podspec.json')
    run_command(f'bundle exec pod repo push {local_pod_source_name} LynxService.podspec.json --local-only --skip-import-validation --allow-warnings --skip-tests')
    
    if component == 'all':
        # skip lint and push pod to local pod source
        pod_lint_component('Lynx',local_pod_source_name)
        pod_lint_component('BaseDevtool',local_pod_source_name)
        pod_lint_component('LynxDevtool',local_pod_source_name)
        pod_lint_component('LynxService',local_pod_source_name)
    else:
        pod_lint_component(component,'local_pod_source_name')
        
def pod_lint_component(component, local_pod_source_name):
    # podspec.json will write the current directory path into itself
    run_command(f'bundle exec pod spec lint {component}.podspec.json --sources=trunk,{local_pod_source_name} --verbose --skip-import-validation --allow-warnings --skip-tests')

def publish_component(component, sources, tag):
    if sources != None:
        run_command(f'export PACKAGE_ENV=prod && geniospkg --repo {component} --tag {tag} --public_repo --cache_path lynx')
        run_command(f'COCOAPODS_TRUNK_TOKEN=$COCOAPODS_TRUNK_TOKEN bundle exec pod trunk push {component}.podspec.json --verbose --skip-import-validation --allow-warnings --skip-tests --sources={sources}')
    else:
        run_command(f'export PACKAGE_ENV=prod && geniospkg --repo {component} --tag {tag} --public_repo --cache_path lynx')
        run_command(f'COCOAPODS_TRUNK_TOKEN=$COCOAPODS_TRUNK_TOKEN bundle exec pod trunk push {component}.podspec.json --verbose --skip-import-validation --allow-warnings --skip-tests')


def publish_to_cocoapods(component, sources, tag):
    root_path = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
    # change to root path
    os.chdir(root_path)
    print(f'Start publish {component} to cocoapods')
    if component == 'all':
        # publish in order: Lynx -> BaseDevtool -> LynxDevtool -> LynxService
        publish_component('Lynx', sources, tag)
        publish_component('BaseDevtool', sources, tag)
        publish_component('LynxDevtool', sources, tag)
        publish_component('LynxService', sources, tag)
    else:
        publish_component(component, sources, tag)


def main():
    """
    usage: 1. 'python3 cocoapods_publish_helper.py --prepare-source --tag <tag> --component <component>'
           2. 'python3 cocoapods_publish_helper.py --publish --component <component> --sources <sources>'
    like : 1. python3 publish_pod_to_cocoapods.py --prepare-source --tag 0.0.1 --component Lynx
           2. python3 publish_pod_to_cocoapods.py --publish --component Lynx --sources 'https://cdn.cocoapods.org'
    """
    parser = argparse.ArgumentParser()
    parser.add_argument('--component', type=str, help='the component to publish', required=True)
    parser.add_argument(
        "--prepare-source", action="store_true", help="Prepare the source for publishing"
    )
    parser.add_argument('--tag', type=str, help='the release version of lynx', required=False)
    parser.add_argument(
        "--publish", action="store_true", help="Publish to cocoapods"
    )
    parser.add_argument('--sources', type=str, help='the cocoapods sources', required=False)
    parser.add_argument('--pod_lint', action="store_true", help='Run pod lint')

    args = parser.parse_args()
    if args.prepare_source:
        prepare_cocoapods_publish_source(args.tag, args.component)
    elif args.publish:
        publish_to_cocoapods(args.component, args.sources, args.tag)
    elif args.pod_lint:
        run_pod_lint(args.component)
    else:
        print('Please specify --prepare-source , --publish or --pod_lint')
        exit(1)


if __name__ == '__main__':
    main()
