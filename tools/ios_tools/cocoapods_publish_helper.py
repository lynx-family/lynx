#!/usr/bin/env python3
# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import argparse
import datetime
import hashlib
import hmac
import http.client
import os
import shutil
import subprocess
import time
import json
import shlex
from urllib.error import HTTPError, URLError
from urllib.parse import quote
from urllib.request import urlopen
from skip_pod_lint import skip_pod_lint

STORAGE_TYPE_GITHUB_RELEASE = 'github_release'
STORAGE_TYPE_S3 = 's3'
S3_ACCESS_KEY_ENV = 'REPO_LYNX_ARTIFACTS_S3_AK'
S3_SECRET_KEY_ENV = 'REPO_LYNX_ARTIFACTS_S3_SK'
S3_BUCKET_ENV = 'REPO_LYNX_ARTIFACTS_S3_BUCKET'
S3_REGION_ENV = 'REPO_LYNX_ARTIFACTS_S3_REGION'
S3_UPLOAD_DOMAIN_ENV = 'REPO_LYNX_ARTIFACTS_S3_UPLOAD_DOMAIN'
S3_PUBLIC_DOMAIN_ENV = 'REPO_LYNX_ARTIFACTS_S3_PUBLIC_DOMAIN'
S3_PATH_PREFIX_ENV = 'REPO_LYNX_ARTIFACTS_S3_PATH_PREFIX'
COCOAPODS_TRUNK_API = 'https://trunk.cocoapods.org/api/v1'
PUBLISH_RETRY_ATTEMPTS = 3
PUBLISH_RETRY_INITIAL_DELAY_SECONDS = 30

def run_command(command, check=True):
    # When the "command" is a multi-line command, only the status of the last line of the command is checked.
    # Therefore, it is necessary to add "set -e" to ensure that any error in any line of the command will cause the script to exit immediately.
    command = 'set -e\n' + command

    print(f'run command: {command}')
    subprocess.run(['bash', '-c', command], stderr=subprocess.STDOUT, check=check, text=True)

def get_podspec_version(component):
    with open(f"{component}.podspec.json", 'r', encoding='utf8') as f:
        content = json.load(f)
    return content['version']

def get_selected_podspec_names(src_dir, component):
    podspec_names = []
    for filename in sorted(os.listdir(src_dir)):
        if filename.endswith('.podspec'):
            podspec_name = filename.split('.')[0]
            if component == podspec_name or component == 'all':
                podspec_names.append(podspec_name)
    return podspec_names

def is_pod_version_published(component, version):
    component_path = quote(component, safe='')
    version_path = quote(version, safe='')
    url = f'{COCOAPODS_TRUNK_API}/pods/{component_path}/versions/{version_path}'

    try:
        with urlopen(url, timeout=15) as response:
            content = json.load(response)
            return bool(content.get('data_url'))
    except HTTPError as e:
        if e.code != 404:
            print(f'Unable to check CocoaPods trunk for {component} {version}: HTTP {e.code}')
        return False
    except (URLError, TimeoutError) as e:
        print(f'Unable to check CocoaPods trunk for {component} {version}: {e}')
        return False
    except (json.JSONDecodeError, UnicodeDecodeError) as e:
        print(f'Unable to parse CocoaPods trunk response for {component} {version}: {e}')
        return False

def build_publish_command(component, sources):
    podspec_json = shlex.quote(f'{component}.podspec.json')
    command = f'COCOAPODS_TRUNK_TOKEN=$COCOAPODS_TRUNK_TOKEN bundle exec pod trunk push {podspec_json} --verbose --skip-import-validation --allow-warnings --skip-tests'
    if sources is not None:
        command += f' --sources={shlex.quote(sources)}'
    return command

def wait_before_publish_retry(component, version, attempt, delay_seconds):
    print(
        f'Failed to publish {component} {version} '
        f'(attempt {attempt}/{PUBLISH_RETRY_ATTEMPTS}); '
        f'retrying in {delay_seconds} seconds.'
    )
    time.sleep(delay_seconds)

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
    for podspec_name in get_selected_podspec_names(src_dir, component):
        print(f'Generating zip file for {podspec_name}')
        quoted_podspec_name = shlex.quote(podspec_name)
        quoted_tag = shlex.quote(tag)
        run_command(
            'export PACKAGE_ENV=prod && '
            f'geniospkg --output_type both --repo {quoted_podspec_name} '
            f'--tag {quoted_tag} --cache_path .'
        )

def validate_s3_storage_options():
    if not os.environ.get(S3_BUCKET_ENV):
        raise ValueError(f'{S3_BUCKET_ENV} is required when --storage-type=s3')
    if not os.environ.get(S3_REGION_ENV):
        raise ValueError(f'{S3_REGION_ENV} is required when --storage-type=s3')
    if not os.environ.get(S3_ACCESS_KEY_ENV):
        raise ValueError(f'{S3_ACCESS_KEY_ENV} is required when --storage-type=s3')
    if not os.environ.get(S3_SECRET_KEY_ENV):
        raise ValueError(f'{S3_SECRET_KEY_ENV} is required when --storage-type=s3')
    if not os.environ.get(S3_UPLOAD_DOMAIN_ENV):
        raise ValueError(f'{S3_UPLOAD_DOMAIN_ENV} is required when --storage-type=s3')
    if not os.environ.get(S3_PUBLIC_DOMAIN_ENV):
        raise ValueError(f'{S3_PUBLIC_DOMAIN_ENV} is required when --storage-type=s3')

def validate_storage_type_options(storage_type):
    if storage_type == STORAGE_TYPE_S3:
        validate_s3_storage_options()
    elif storage_type != STORAGE_TYPE_GITHUB_RELEASE:
        raise ValueError(f'Unsupported storage type: {storage_type}')

def set_http_pod_source(component, source_url):
    with open(f"{component}.podspec.json", 'r', encoding='utf8') as f:
        content = json.load(f)

    content["source"] = {"http": source_url}

    with open(f"{component}.podspec.json", "w", encoding='utf8') as f:
        json.dump(content, f, indent=4)

def get_file_sha256(file_path):
    digest = hashlib.sha256()
    with open(file_path, 'rb') as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b''):
            digest.update(chunk)
    return digest.hexdigest()

def get_signature_key(secret_key, date_stamp, region, service):
    date_key = hmac.new(
        ('AWS4' + secret_key).encode('utf-8'),
        date_stamp.encode('utf-8'),
        hashlib.sha256,
    ).digest()
    region_key = hmac.new(date_key, region.encode('utf-8'), hashlib.sha256).digest()
    service_key = hmac.new(region_key, service.encode('utf-8'), hashlib.sha256).digest()
    return hmac.new(service_key, b'aws4_request', hashlib.sha256).digest()

def build_s3_object_key(path_prefix, version, filename):
    return '/'.join(part.strip('/') for part in (path_prefix, version, filename) if part and part.strip('/'))

def build_s3_host(bucket, domain):
    return f'{bucket}.{domain.strip().strip(".")}'

def build_s3_public_url(bucket, public_domain, object_key):
    escaped_key = quote(object_key, safe='/~')
    return f'https://{build_s3_host(bucket, public_domain)}/{escaped_key}'

def upload_file_to_s3(file_path, bucket, region, upload_domain, object_key, access_key, secret_key):
    method = 'PUT'
    service = 's3'
    host = build_s3_host(bucket, upload_domain)
    canonical_uri = '/' + quote(object_key, safe='/~')
    file_size = os.path.getsize(file_path)
    payload_hash = get_file_sha256(file_path)
    timestamp = datetime.datetime.utcnow()
    amz_date = timestamp.strftime('%Y%m%dT%H%M%SZ')
    date_stamp = timestamp.strftime('%Y%m%d')
    credential_scope = f'{date_stamp}/{region}/{service}/aws4_request'
    signed_headers = 'content-length;content-type;host;x-amz-content-sha256;x-amz-date'
    headers = {
        'Content-Length': str(file_size),
        'Content-Type': 'application/zip',
        'Host': host,
        'x-amz-content-sha256': payload_hash,
        'x-amz-date': amz_date,
    }
    canonical_headers = ''.join(
        f'{name.lower()}:{headers[name]}\n'
        for name in sorted(headers, key=str.lower)
    )
    canonical_request = '\n'.join([
        method,
        canonical_uri,
        '',
        canonical_headers,
        signed_headers,
        payload_hash,
    ])
    string_to_sign = '\n'.join([
        'AWS4-HMAC-SHA256',
        amz_date,
        credential_scope,
        hashlib.sha256(canonical_request.encode('utf-8')).hexdigest(),
    ])
    signing_key = get_signature_key(secret_key, date_stamp, region, service)
    signature = hmac.new(signing_key, string_to_sign.encode('utf-8'), hashlib.sha256).hexdigest()
    headers['Authorization'] = (
        f'AWS4-HMAC-SHA256 Credential={access_key}/{credential_scope}, '
        f'SignedHeaders={signed_headers}, Signature={signature}'
    )

    connection = http.client.HTTPSConnection(host, timeout=120)
    try:
        with open(file_path, 'rb') as body:
            connection.request(method, canonical_uri, body=body, headers=headers)
        response = connection.getresponse()
        response_body = response.read().decode('utf-8', errors='replace')
        if response.status not in (200, 201, 204):
            raise RuntimeError(
                f'Failed to upload {file_path} to S3-compatible storage: '
                f'HTTP {response.status} {response.reason}\n{response_body}'
            )
    finally:
        connection.close()

def upload_zip_sources_to_s3(src_dir, component):
    access_key = os.environ[S3_ACCESS_KEY_ENV]
    secret_key = os.environ[S3_SECRET_KEY_ENV]
    bucket = os.environ[S3_BUCKET_ENV]
    region = os.environ[S3_REGION_ENV]
    upload_domain = os.environ[S3_UPLOAD_DOMAIN_ENV]
    public_domain = os.environ[S3_PUBLIC_DOMAIN_ENV]
    path_prefix = os.environ.get(S3_PATH_PREFIX_ENV, '')

    for podspec_name in get_selected_podspec_names(src_dir, component):
        version = get_podspec_version(podspec_name)
        zip_filename = f'{podspec_name}-{version}.zip'
        zip_path = os.path.join(src_dir, zip_filename)
        if not os.path.exists(zip_path):
            raise FileNotFoundError(f'Expected zip file does not exist: {zip_path}')

        object_key = build_s3_object_key(path_prefix, version, zip_filename)
        source_url = build_s3_public_url(bucket, public_domain, object_key)
        # CocoaPods trunk downloads source URLs without workflow credentials.
        # Keep this bucket or prefix publicly readable before publishing.
        print(f'Uploading {zip_filename} to {source_url}')
        upload_file_to_s3(zip_path, bucket, region, upload_domain, object_key, access_key, secret_key)
        set_http_pod_source(podspec_name, source_url)

def get_enable_trace_param(version: str) -> str:
    """
    Keep Perfetto events in every published CocoaPods source package.
    """
    return '--enable-trace'

def prepare_cocoapods_publish_source(
        version,
        tag,
        component,
        storage_type=STORAGE_TYPE_GITHUB_RELEASE):
    validate_storage_type_options(storage_type)

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

    if storage_type == STORAGE_TYPE_S3:
        print('5. Upload zip files to S3-compatible storage')
        upload_zip_sources_to_s3(root_path, component)

def use_local_pod_source(component):
    version = get_podspec_version(component)
    source_url = f"file:{os.getcwd()}/{component}-{version}.zip"
    set_http_pod_source(component, source_url)

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

def publish_component(component, sources):
    version = get_podspec_version(component)
    command = build_publish_command(component, sources)
    delay_seconds = PUBLISH_RETRY_INITIAL_DELAY_SECONDS

    for attempt in range(1, PUBLISH_RETRY_ATTEMPTS + 1):
        if is_pod_version_published(component, version):
            print(f'Skip {component} {version}; it is already published to CocoaPods trunk.')
            return

        try:
            run_command(command)
            return
        except subprocess.CalledProcessError:
            if is_pod_version_published(component, version):
                print(f'Treat {component} {version} as published after CocoaPods trunk confirmed it.')
                return

            if attempt == PUBLISH_RETRY_ATTEMPTS:
                raise

            wait_before_publish_retry(component, version, attempt, delay_seconds)
            delay_seconds *= 2


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
    parser.add_argument('--storage-type', default=STORAGE_TYPE_GITHUB_RELEASE, help='The zip storage backend used by --prepare-source')

    args = parser.parse_args()
    if args.prepare_source:
        try:
            prepare_cocoapods_publish_source(
                args.version,
                args.tag,
                args.component,
                args.storage_type,
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
