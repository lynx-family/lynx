#!/usr/bin/env python3
# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import argparse
import os
import shutil
import subprocess
import json
import shlex
import time
import re
from skip_pod_lint import skip_pod_lint

SOURCE_TYPE_ZIP = 'zip'
SOURCE_TYPE_GIT = 'git'
GIT_SOURCE_REF_TYPES = ('commit', 'tag', 'branch')

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

def validate_git_source_options(git_source_ref_type, git_source_ref):
    if not git_source_ref_type:
        raise ValueError('--git-source-ref-type is required when --source-type=git')
    if not git_source_ref:
        raise ValueError('--git-source-ref is required when --source-type=git')
    if git_source_ref_type not in GIT_SOURCE_REF_TYPES:
        raise ValueError(f'--git-source-ref-type must be one of: {", ".join(GIT_SOURCE_REF_TYPES)}')

def validate_source_type_options(source_type, git_source_url, git_source_ref_type, git_source_ref):
    if source_type == SOURCE_TYPE_GIT:
        if not git_source_url:
            raise ValueError('--git-source-url is required when --source-type=git')
        validate_git_source_options(git_source_ref_type, git_source_ref)
    elif source_type != SOURCE_TYPE_ZIP:
        raise ValueError(f'Unsupported source type: {source_type}')

def use_git_pod_source(component, git_source_url, git_source_ref_type, git_source_ref):
    with open(f"{component}.podspec.json", 'r', encoding='utf8') as f:
        content = json.load(f)

    content["source"] = {
        "git": git_source_url,
        git_source_ref_type: git_source_ref,
    }

    with open(f"{component}.podspec.json", "w", encoding='utf8') as f:
        json.dump(content, f, indent=4)

def generate_git_source_podspec_files(src_dir, component, git_source_url, git_source_ref_type, git_source_ref):
    run_command('SDKROOT=/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk bundle install --path .')

    for filename in os.listdir(src_dir):
        if filename.endswith('.podspec'):
            podspec_name = filename.split('.')[0]
            if component == podspec_name or component == 'all':
                print(f'Generating git source podspec for {podspec_name}')
                podspec_file = shlex.quote(f'{podspec_name}.podspec')
                podspec_json_file = shlex.quote(f'{podspec_name}.podspec.json')
                run_command(f'bundle exec pod ipc spec {podspec_file} > {podspec_json_file}')
                use_git_pod_source(podspec_name, git_source_url, git_source_ref_type, git_source_ref)

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

def prepare_cocoapods_publish_source(
        version,
        tag,
        component,
        source_type=SOURCE_TYPE_ZIP,
        git_source_url=None,
        git_source_ref_type=None,
        git_source_ref=None):
    validate_source_type_options(source_type, git_source_url, git_source_ref_type, git_source_ref)

    root_path = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

    # change to root path
    os.chdir(root_path)

    print('Start prepare cocoapods publish source')
    print('1. Replace lynx version')
    replace_lynx_version(version)

    print('2. Generate podspec files')
    run_command(f'python3 tools/ios_tools/generate_podspec_scripts_by_gn.py --root {root_path} {get_enable_trace_param(version)}')

    if source_type == SOURCE_TYPE_GIT:
        print('3. Generate git source podspec files')
        generate_git_source_podspec_files(root_path, component, git_source_url, git_source_ref_type, git_source_ref)
        return

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
        pod_lint_component('LynxServiceAPI',local_pod_source_name)
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

def check_version_published(component):
    try:
        with open(f"{component}.podspec.json", 'r', encoding='utf8') as f:
            content = json.load(f)
        version = content.get("version")
        print("11111")
        print(version)
        if not version:
            return False
        
        print(f"Checking if {component} version {version} is already published...")
        res = subprocess.run(
            ['bash', '-c', f'COCOAPODS_TRUNK_TOKEN=$COCOAPODS_TRUNK_TOKEN bundle exec pod trunk info {component}'],
            stderr=subprocess.STDOUT, stdout=subprocess.PIPE, text=True
        )
        print("33333")
        print(res.returncode)
        print(res.stdout)
        if res.returncode == 0:
            print(22222)
            print(r'^\s*-\s*' + re.escape(version) + r'\s*\(')
            print(res.stdout)
            print(re.search(r'^\s*-\s*' + re.escape(version) + r'\s*\(', res.stdout, re.MULTILINE))
            if re.search(r'^\s*-\s*' + re.escape(version) + r'\s*\(', res.stdout, re.MULTILINE):
                print(f"Version {version} of {component} is already published. Skipping.")
                return True

        return False
    except Exception as e:
        print(f"Failed to check if {component} is published: {e}")
        return False

def publish_component(component, sources):
    command = f'COCOAPODS_TRUNK_TOKEN=$COCOAPODS_TRUNK_TOKEN bundle exec pod trunk push {component}.podspec.json --verbose --skip-import-validation --allow-warnings --skip-tests'
    if sources != None:
        command += f' --sources={sources}'

    max_retries = 10
    for attempt in range(max_retries):
        if check_version_published(component):
            return
            
        try:
            print(f"Attempt {attempt + 1} to publish {component}")
            run_command(command)
            break
        except subprocess.CalledProcessError as e:
            if attempt < max_retries - 1:
                print(f"Publish failed, retrying in 5 seconds... (Error: {e})")
                time.sleep(10)
            else:
                raise


def publish_to_cocoapods(component, sources):
    print(f'Start publish {component} to cocoapods')
    if component == 'all':
        # publish in order: LynxServiceAPI -> LynxBase -> Lynx -> BaseDevtool -> LynxDevtool -> LynxService
        publish_component('LynxServiceAPI', sources)
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
        publish_component_to_local_source('LynxServiceAPI', local_source_name)
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
    parser.add_argument('--source-type', default=SOURCE_TYPE_ZIP, help='The podspec source type used by --prepare-source')
    parser.add_argument('--git-source-url', type=str, help='The git repository URL used when --source-type=git', required=False)
    parser.add_argument('--git-source-ref-type', help='The git ref field used when --source-type=git')
    parser.add_argument('--git-source-ref', type=str, help='The git ref value used when --source-type=git', required=False)

    args = parser.parse_args()
    if args.prepare_source:
        try:
            prepare_cocoapods_publish_source(
                args.version,
                args.tag,
                args.component,
                args.source_type,
                args.git_source_url,
                args.git_source_ref_type,
                args.git_source_ref,
            )
        except ValueError as error:
            parser.error(str(error))
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
