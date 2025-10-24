#!/usr/bin/env python3
# -*- coding: UTF-8 -*-
# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import argparse
import os
import sys
import shutil
from subprocess import check_call

from patch_lynx_version import patch_lynx_version

CUR_DIR = os.path.dirname(os.path.abspath(__file__))
HARMONY_DIR = os.path.normpath(os.path.join(CUR_DIR, '..'))
LYNX_DIR = os.path.normpath(os.path.join(HARMONY_DIR, '..', '..'))
TEMPLATE_DEVTOOL_DIR = os.path.normpath(
    os.path.join(LYNX_DIR, 'devtool', 'lynx_devtool', 'resources', 'devtool-switch'))

ABI_LIST = [
    'arm64-v8a',
    'x86_64',
]

DEFAULT_MODULES = [
    'lynx',
    'lynx_base',
    'lynx_devtool',
    'lynx_devtool_service',
    'lynx_log_service',
    'lynx_http_service',
]


def add_node_home_env():
    if 'COMMANDLINE_TOOL_DIR' not in os.environ:
        print('COMMANDLINE_TOOL_DIR environment variable not found, ignore set NODE_HOME')
        return
    if 'NODE_HOME' in os.environ:
        print('NODE_HOME environment variable already exists, skipping setup')
        return
    cmd_tool_dir = os.environ['COMMANDLINE_TOOL_DIR']
    node_home = os.path.join(cmd_tool_dir, 'tool', 'node')
    if not os.path.isdir(node_home):
        print(f'Warning: Node home directory not found at {node_home}, NODE_HOME not set')
        return
    os.environ['NODE_HOME'] = node_home
    print(f'Successfully set NODE_HOME to: {node_home}')


def get_build_type(args):
    return 'debug' if args.debug else 'release'

def get_cpu_type(abi):
    return 'arm64' if abi == 'arm64-v8a' else 'x64'

def run_build_lynx_core(verbose, version):
    os.environ['PATH'] = os.pathsep.join([os.path.join(LYNX_DIR, '..', 'buildtools', 'node', 'bin'), os.environ['PATH']])
    main_dest_path = os.path.join(LYNX_DIR, 'platform', 'harmony', 'lynx_harmony', 'src', 'main', 'resources', 'rawfile')
    debug_dest_path = os.path.join(LYNX_DIR, 'platform', 'harmony', 'lynx_devtool', 'src', 'main', 'resources', 'rawfile')
    cmd = (f'python3 ./build.py --platform android '
           f'--release_output {os.path.join(main_dest_path, "lynx_core.js")} '
           f'--dev_output {os.path.join(debug_dest_path, "lynx_core_dev.js")} '
           f'--version {version}')
    if verbose:
        print(f'run command {cmd}')
    check_call(cmd, shell=True, cwd=os.path.join(LYNX_DIR, 'tools', 'js_tools'))

def run_copy_devtool_resources(verbose):
    scripts = [
        os.path.join(LYNX_DIR, 'devtool', 'base_devtool', 'resources', 'copy_resources.py'),
        os.path.join(LYNX_DIR, 'devtool', 'lynx_devtool', 'resources', 'copy_resources.py'),
    ]
    for script in scripts:
        if os.path.exists(script):
            command = "python3 " + script
            if verbose:
                print(f'run command {command}')
            check_call(command, shell=True, cwd=LYNX_DIR)
        else:
            print(f"Script {script} does not exist.")

def build_explorer_bundle(verbose):
    build_and_copy_path = os.path.join(LYNX_DIR, 'explorer', 'showcase', 'build_and_copy.py')
    if os.path.exists(build_and_copy_path):
        check_call(['python3', build_and_copy_path])
    else:
        print(f'Warning: {build_and_copy_path} not found')

    homepage_build_path = os.path.join(LYNX_DIR, 'explorer', 'homepage', 'build.py')
    if os.path.exists(homepage_build_path):
        if verbose:
            print(f'run command python3 {homepage_build_path}')
        check_call(['python3', homepage_build_path], cwd=os.path.dirname(homepage_build_path))

        # Copy main.lynx.bundle
        src_file = os.path.join(LYNX_DIR, 'explorer', 'homepage', 'dist', 'main.lynx.bundle')
        dst_dir = os.path.join(LYNX_DIR, 'explorer', 'harmony', 'lynx_explorer', 'src', 'main', 'resources', 'rawfile')
        if os.path.exists(src_file):
            import shutil
            os.makedirs(dst_dir, exist_ok=True)
            shutil.copy(src_file, dst_dir)
        else:
            print(f'Warning: {src_file} not found')
    else:
        print(f'Warning: {homepage_build_path} not found')

def get_out_dir(args, abi):
    dir_name = f'harmony_{get_build_type(args)}_{get_cpu_type(abi)}'
    if args.dev:
        dir_name += '_dev'
    out_dir = os.path.join(HARMONY_DIR, '..', '..', 'out', dir_name)
    return out_dir

def run_gn(args, gn_out_dir, abi):
    gn_args = f'target_os="harmony" jsengine_type="quickjs" is_debug={str(args.debug).lower()} target_cpu="{get_cpu_type(abi)}" harmony_sdk_version="default" use_primjs_napi=true build_lepus_compile=false enable_primjs_prebuilt_lib=true enable_inspector=true enable_harmony_shared=true'
    if args.dev:
        gn_args += ' enable_testbench_replay=true enable_testbench_recorder=true enable_trace="perfetto"'
    cmd = f'gn gen {gn_out_dir} --args=\'{gn_args}\' --export-compile-commands'
    check_call(cmd, shell=True, cwd=HARMONY_DIR)

def run_build_so(output_path, args):
    target = 'default'
    cmd = f'ninja -C {output_path} {target}'
    if args.verbose:
        print(f'run command {cmd}')
    check_call(cmd, shell=True, cwd=HARMONY_DIR)


def run_cp_so(output_path, args, abi):
    shared_object_cp_map = {
        'liblynx.so': os.path.join(LYNX_DIR, 'platform', 'harmony', 'lynx_harmony', 'libs', abi),
        'liblynxbase.so': os.path.join(LYNX_DIR, 'base', 'platform', 'harmony', 'libs', abi),
        'liblynxdevtool.so': os.path.join(LYNX_DIR, 'platform', 'harmony', 'lynx_devtool', 'libs', abi),
    }
    for so, dst in shared_object_cp_map.items():
        src = os.path.join(output_path, so)
        if not os.path.isfile(src):
            print(f'skip cp {so} to {dst} as the {so} file is not built')
            continue
        os.makedirs(dst, exist_ok=True)
        shutil.copy(src, dst)
        if args.verbose:
            print(f'run command cp {src} {dst}')

def run_package_har(module_name, module_path, verbose):
    if verbose:
        print(f'===== start run package {module_name} =====')
    cmd = f'hvigorw assembleHar --mode module -p module={module_name}@default -p product=default -p buildMode=debug --no-daemon'
    if verbose:
        print(f'run command {cmd}')
    check_call(cmd, shell=True, cwd=HARMONY_DIR)
    # as even hvigor build failed, it still returns value 0, so we need to check har file exist or not
    har_path = os.path.join(HARMONY_DIR, module_path, 'build', 'default', 'outputs', 'default', f'{module_name}.har')
    if not os.path.isfile(har_path):
        raise Exception('har file not found, please check your build')


def collect_module_config_list(args):
    import json5
    with open(os.path.join(HARMONY_DIR, 'build-profile.json5'), 'r') as f:
        build_profile = json5.load(f)

    module_config_list = build_profile['modules']
    if args.verbose:
        print('module_config_list is' + str(module_config_list))
    return module_config_list


def run_package_hap(args):
    build_type = get_build_type(args)
    cmd = f"hvigorw assembleApp --mode project -p product=default -p buildMode={build_type} -p skipGn=true --no-daemon"
    if args.verbose:
        print(f'run command {cmd}')
    check_call(cmd, shell=True, cwd=HARMONY_DIR)

def delete_gitignore_file():
    gitignore_paths = [
        os.path.join(LYNX_DIR, 'platform', 'harmony', 'lynx_harmony', 'src', 'main', 'ets', 'tasm', 'gen', '.gitignore'),
        os.path.join(LYNX_DIR, 'platform', 'harmony', 'lynx_devtool', 'src', 'main', 'resources', 'rawfile', '.gitignore')
    ]    
    for gitignore_path in gitignore_paths:
        if os.path.exists(gitignore_path):
            os.remove(gitignore_path)

def main(argv):
    parser = argparse.ArgumentParser()

    parser.add_argument("--debug", action="store_true", default=False, help="debug")
    parser.add_argument("--dev", action="store_true", default=False, help="dev mode")
    parser.add_argument("--modules", nargs="*", help="list of modules name")
    parser.add_argument("--override_version", type=str, required=False, help="override version")
    parser.add_argument("--build_lynx_core", action="store_true", default=False, help="build lynx core")
    parser.add_argument("--build_bundle", action="store_true", default=False, help="build explorer bundle")
    parser.add_argument("--verbose", action="store_true", default=False, help="print all commands")
    parser.add_argument("--build_har", action="store_true", default=False, help=" build har")
    parser.add_argument("--build_hap", action="store_true", default=False, help=" build hap")
    args = parser.parse_args()

    envs = ['COMMANDLINE_TOOL_DIR', 'HARMONY_HOME', 'PATH']
    env_info = {k: os.environ[k] for k in os.environ if k in envs}
    print(f'start build with args {args}')
    print(f'env info: {env_info}')

    if args.modules:
        if len(args.modules) == 1 and args.modules[0].lower() == "default":
            if args.verbose:
                print("Using default module list as '--modules default' was specified.")
            modules = DEFAULT_MODULES
        else:
            modules = args.modules
    else:
        modules = []

    if args.build_lynx_core:
        run_build_lynx_core(args.verbose, args.override_version if args.override_version else '0.0.1')
        run_copy_devtool_resources(args.verbose)
    if args.build_bundle:
        build_explorer_bundle(args.verbose)

    for abi in ABI_LIST:
        gn_out_dir = get_out_dir(args, abi)
        run_gn(args, gn_out_dir, abi)
        run_build_so(gn_out_dir, args)
        run_cp_so(gn_out_dir, args, abi)

    if args.build_har and len(modules) > 0:
        commit_hash = os.popen('git rev-parse HEAD').read().strip()
        print('commit hash is ' + commit_hash)

        add_node_home_env()

        if args.override_version:
            publish_version = args.override_version
        else:
            # default version, just for package test
            publish_version = '0.0.1-placeholder'

        # remove .gitignore file before build har
        # since har package will ignore files in .gitignore
        delete_gitignore_file()

        print('publish version is ' + publish_version)

        module_paths = {}
        for module in modules:
            module_config_list = collect_module_config_list(args)
            for module_config in module_config_list:
                if module_config['name'] == module:
                    module_path = module_config['srcPath']
                    module_paths[module] = module_path
                    break
            else:
                raise Exception(f'module {module} not found in build-profile.json5')
            module_full_path = os.path.join(HARMONY_DIR, module_path)
            patch_lynx_version(publish_version, commit_hash, module_full_path)
            run_package_har(module, module_full_path, args.verbose)

    if args.build_hap:
        add_node_home_env()
        run_package_hap(args)

    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
