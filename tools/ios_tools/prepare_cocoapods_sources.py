# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
"""Prepare a local CocoaPods Specs cache from OSS-hosted sources.

Reads a source list file whose lines follow the same format used by the
internal ``tools/ios_tools/prepare_cocoapods_source.py``::

    <source_url>:<PodName>:<Version> pod [auto]

For each entry the script downloads ``<PodName>.podspec.json`` (falling
back to ``<PodName>.podspec``) and writes it into ``--cache-dir`` under
a flat ``<PodName>/<Version>/`` layout, then commits the result to a
local git repository. A Podfile can then consume the cache via::

    source "file://" + ENV["COCOAPODS_LOCAL_SOURCE_REPO"]

Because ``git archive --remote=`` is not supported by GitHub, this
open-source variant fetches each podspec over HTTPS from a CocoaPods
CDN-style source (typically ``https://cdn.cocoapods.org/``), following
the ``prefix_lengths`` layout advertised in ``CocoaPods-version.yml``.
"""

import argparse
import hashlib
import http.client
import os
import shutil
import ssl
import subprocess
import sys
import urllib.error
import urllib.parse
import urllib.request
from concurrent.futures import ThreadPoolExecutor, wait

COCOAPODS_VERSION_FILE = 'CocoaPods-version.yml'
SPEC_FILE_SUFFIXES = ['podspec.json', 'podspec']
DEFAULT_PREFIX_LENGTHS = [1, 1, 1]
DEFAULT_REQUEST_TIMEOUT_SECONDS = 30
MAX_RETRY_ATTEMPTS = 5


def log(msg):
    print(msg)
    sys.stdout.flush()


def log_r(msg):
    log(f'\033[31m {msg} \033[0m')


def log_y(msg):
    log(f'\033[33m {msg} \033[0m')


def _http_get(url, timeout=DEFAULT_REQUEST_TIMEOUT_SECONDS):
    request = urllib.request.Request(
        url,
        headers={'User-Agent': 'lynx-prepare-cocoapods-source/1.0'},
    )
    with urllib.request.urlopen(request, timeout=timeout) as response:
        return response.read()


def _parse_prefix_lengths(yaml_text):
    """Extract ``prefix_lengths`` from a small CocoaPods-version.yml file."""
    prefix_lengths = []
    in_block = False
    for raw_line in yaml_text.splitlines():
        line = raw_line.rstrip()
        stripped = line.strip()
        if not stripped or stripped.startswith('#'):
            continue
        if stripped.startswith('prefix_lengths'):
            remainder = stripped.split(':', 1)[1].strip()
            if remainder.startswith('['):
                for part in remainder.strip('[]').split(','):
                    part = part.strip()
                    if part:
                        try:
                            prefix_lengths.append(int(part))
                        except ValueError:
                            pass
                return prefix_lengths
            in_block = True
            continue
        if in_block:
            if stripped.startswith('- '):
                try:
                    prefix_lengths.append(int(stripped[2:].strip()))
                except ValueError:
                    pass
            else:
                break
    return prefix_lengths


class CdnSource:
    """A CocoaPods CDN-style Specs source served over HTTPS."""

    def __init__(self, url):
        self.url = url.rstrip('/') + '/'
        self._prefix_lengths = None

    def __str__(self):
        return self.url

    def _load_prefix_lengths(self):
        if self._prefix_lengths is not None:
            return self._prefix_lengths
        try:
            content = _http_get(
                urllib.parse.urljoin(self.url, COCOAPODS_VERSION_FILE)
            )
        except Exception as e:  # noqa: BLE001 - network fallback path
            log_y(
                f'Failed to load {COCOAPODS_VERSION_FILE} from {self.url}: '
                f'{type(e).__name__}: {e}; falling back to {DEFAULT_PREFIX_LENGTHS}.'
            )
            self._prefix_lengths = list(DEFAULT_PREFIX_LENGTHS)
            return self._prefix_lengths
        parsed = _parse_prefix_lengths(content.decode('utf-8', errors='replace'))
        self._prefix_lengths = parsed if parsed else list(DEFAULT_PREFIX_LENGTHS)
        return self._prefix_lengths

    def _spec_path(self, pod_name, version, suffix):
        digest = hashlib.md5(pod_name.encode()).hexdigest()
        components = ['Specs']
        offset = 0
        for length in self._load_prefix_lengths():
            components.append(digest[offset:offset + length])
            offset += length
        components += [pod_name, version, f'{pod_name}.{suffix}']
        return '/'.join(components)

    def download_podspec(self, pod_name, version, target_dir):
        last_error = None
        attempted_suffixes = []
        for suffix in SPEC_FILE_SUFFIXES:
            attempted_suffixes.append(suffix)
            url = urllib.parse.urljoin(self.url, self._spec_path(pod_name, version, suffix))
            for attempt in range(MAX_RETRY_ATTEMPTS):
                try:
                    content = _http_get(url)
                except urllib.error.HTTPError as e:
                    last_error = e
                    if e.code == 404:
                        break
                    log_y(
                        'HTTP error downloading podspec: '
                        f'pod_name={pod_name}, version={version}, suffix={suffix}, '
                        f'attempt={attempt}, status={e.code}, reason={e.reason}'
                    )
                    continue
                except (urllib.error.URLError, http.client.HTTPException,
                        OSError, ssl.SSLError) as e:
                    last_error = e
                    log_y(
                        'Network error downloading podspec: '
                        f'pod_name={pod_name}, version={version}, suffix={suffix}, '
                        f'attempt={attempt}, error={type(e).__name__}: {e}'
                    )
                    continue

                base_dir = os.path.join(target_dir, pod_name, version)
                os.makedirs(base_dir, exist_ok=True)
                spec_path = os.path.join(base_dir, f'{pod_name}.{suffix}')
                with open(spec_path, 'wb') as f:
                    f.write(content)
                return spec_path

        attempted = ', '.join(attempted_suffixes)
        raise Exception(
            'failed to download podspec: '
            f'pod_name={pod_name}, version={version}, target_dir={target_dir}, '
            f'suffixes_tried=[{attempted}], last_error={last_error}'
        )


class Pod:
    def __init__(self, name, version, source):
        self.name = name
        self.version = version
        self.source = source
        self.spec_path = None

    def __str__(self):
        return f'{self.source}:{self.name}:{self.version}'

    def download_podspec(self, target_dir):
        log(f'download podspec: {self} to {target_dir}')
        try:
            self.spec_path = self.source.download_podspec(self.name, self.version, target_dir)
        except Exception as e:  # noqa: BLE001 - return to main thread
            return e


def load_pods(source_list):
    pods = []
    sources = {}
    with open(source_list, 'r') as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith('#'):
                continue
            parts = line.rsplit(maxsplit=2)
            if len(parts) < 2 or parts[1] != 'pod':
                log(f'Invalid source {line} is ignored')
                continue
            source_url, pod_name, version = parts[0].rsplit(':', maxsplit=2)
            resolved = os.getenv(version.replace('$', ''))
            if resolved:
                version = resolved
            if '$' in version:
                continue
            source = sources.setdefault(source_url, CdnSource(source_url))
            pods.append(Pod(pod_name, version, source))
    return pods


def clear_outdated_specs(pod_name, keep_version, cache_dir):
    pod_dir = os.path.join(cache_dir, pod_name)
    if not os.path.isdir(pod_dir):
        return
    for entry in os.listdir(pod_dir):
        version_path = os.path.join(pod_dir, entry)
        if os.path.isdir(version_path) and entry != keep_version:
            shutil.rmtree(version_path)


def spec_cache_valid(pod_name, version, cache_dir):
    for suffix in SPEC_FILE_SUFFIXES:
        spec_path = os.path.join(cache_dir, pod_name, version, f'{pod_name}.{suffix}')
        if os.path.exists(spec_path):
            return True
    return False


def _has_git_identity(cache_dir):
    for key in ('user.name', 'user.email'):
        try:
            subprocess.check_output(
                ['git', 'config', '--get', key],
                cwd=cache_dir,
                stderr=subprocess.DEVNULL,
            )
        except subprocess.CalledProcessError:
            return False
    return True


def _ensure_git_identity(cache_dir):
    if _has_git_identity(cache_dir):
        return
    subprocess.check_call(
        ['git', 'config', 'user.email', 'ci@github.com'], cwd=cache_dir
    )
    subprocess.check_call(
        ['git', 'config', 'user.name', 'ci'], cwd=cache_dir
    )


def prepare_source_repo(pods, cache_dir):
    log('Preparing local source repository for cocoapods (OSS)')
    if os.path.exists(cache_dir):
        shutil.rmtree(cache_dir, ignore_errors=True)
    os.makedirs(cache_dir)
    subprocess.check_call(['git', 'init', '.'], cwd=cache_dir)
    _ensure_git_identity(cache_dir)

    thread_pool = ThreadPoolExecutor()
    futures = []
    for pod in pods:
        clear_outdated_specs(pod.name, pod.version, cache_dir)
        if spec_cache_valid(pod.name, pod.version, cache_dir):
            continue
        futures.append(thread_pool.submit(pod.download_podspec, cache_dir))

    if not futures:
        return

    wait(futures)
    for future in futures:
        result = future.result()
        if isinstance(result, Exception):
            log_r(str(result))
            raise result

    blocking = os.get_blocking(1)
    os.set_blocking(1, True)
    try:
        subprocess.check_call(['git', 'add', '-A'], cwd=cache_dir)
        try:
            subprocess.check_output(
                ['git', 'rev-parse', 'HEAD'], cwd=cache_dir, stderr=subprocess.STDOUT
            )
            subprocess.check_output(
                ['git', 'commit', '--amend', '--no-edit'],
                cwd=cache_dir,
                stderr=subprocess.STDOUT,
            )
        except subprocess.CalledProcessError:
            subprocess.check_call(
                ['git', 'commit', '-m', 'Init'], cwd=cache_dir
            )
    finally:
        os.set_blocking(1, blocking)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-list', required=True, help='Path to source list file')
    parser.add_argument('--cache-dir', required=True, help='Path to cache directory')
    args = parser.parse_args()

    pods = load_pods(args.source_list)
    prepare_source_repo(pods, args.cache_dir)


if __name__ == '__main__':
    main()
