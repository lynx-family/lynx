#!/usr/bin/env python3
# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
import os
import sys
import shutil
import stat
import argparse

ITEMS_TO_COPY = ["generated", "headers", "shim"]
OBSOLETE_ITEMS = ["prebuilt"]


class SyncError(Exception):
    pass


def _format_delete_error(path, error):
    reason = f"{type(error).__name__}: {error}"
    return (
        f"Failed to remove existing weak-node-api dependency output: {path}\n"
        f"Reason: {reason}\n"
        "The dependency package changed, so the local copied output must be "
        "replaced. On Windows this usually means a file is read-only or is "
        "currently held open by another build/process. Stop concurrent builds "
        "or close processes using this directory, then rerun GN/Ninja."
    )


def _remove_existing(path):
    def onerror(function, failed_path, exc_info):
        error = exc_info[1]
        if isinstance(error, PermissionError):
            try:
                os.chmod(failed_path, stat.S_IWRITE)
                function(failed_path)
                return
            except Exception as retry_error:
                raise SyncError(_format_delete_error(failed_path, retry_error))
        raise SyncError(_format_delete_error(failed_path, error))

    try:
        if os.path.islink(path):
            os.unlink(path)
        elif os.path.isdir(path):
            shutil.rmtree(path, onerror=onerror)
        else:
            os.remove(path)
    except SyncError:
        raise
    except Exception as error:
        raise SyncError(_format_delete_error(path, error))


def _copy_item(src_item, dst_item):
    if os.path.exists(dst_item):
        _remove_existing(dst_item)

    os.makedirs(os.path.dirname(dst_item), exist_ok=True)
    if os.path.isdir(src_item):
        shutil.copytree(src_item, dst_item, symlinks=True)
    else:
        shutil.copy2(src_item, dst_item)


def _iter_dependency_files(source_dir):
    package_json = os.path.join(source_dir, "package.json")
    if os.path.isfile(package_json):
        yield package_json

    for item in ITEMS_TO_COPY:
        item_path = os.path.join(source_dir, item)
        if os.path.isfile(item_path) or os.path.islink(item_path):
            yield item_path
            continue

        for current_root, dirnames, filenames in os.walk(item_path):
            dirnames.sort()
            filenames.sort()
            for filename in filenames:
                yield os.path.join(current_root, filename)


def _escape_depfile_path(path):
    return path.replace("\\", "\\\\").replace(" ", "\\ ")


def _write_depfile(depfile, output, dependencies, root_build_dir):
    output_path = os.path.relpath(output, root_build_dir)
    dependency_paths = [
        os.path.relpath(dependency, root_build_dir) for dependency in dependencies
    ]
    os.makedirs(os.path.dirname(depfile), exist_ok=True)
    with open(depfile, "w", encoding="utf-8") as file:
        file.write(_escape_depfile_path(output_path))
        file.write(":")
        for dependency_path in dependency_paths:
            file.write(" ")
            file.write(_escape_depfile_path(dependency_path))
        file.write("\n")


def _write_stamp(stamp_path):
    os.makedirs(os.path.dirname(stamp_path), exist_ok=True)
    with open(stamp_path, "w", encoding="utf-8") as file:
        file.write("weak-node-api dependency outputs synced\n")


def _copy_weak_napi_headers_to_shim(target_dir):
    weak_napi_headers = ["weak_napi_defines.h", "weak_napi_undefs.h"]
    shim_dir = os.path.join(target_dir, "shim")
    os.makedirs(shim_dir, exist_ok=True)
    for header in weak_napi_headers:
        src_header = os.path.join(target_dir, "headers", header)
        dst_header = os.path.join(shim_dir, header)
        if not os.path.exists(src_header):
            print(f"Error: weak-node-api header is missing: {src_header}")
            sys.exit(1)
        shutil.copy2(src_header, dst_header)


def _parse_args():
    parser = argparse.ArgumentParser(
        description="Sync @lynx-js/weak-node-api files for GN builds."
    )
    parser.add_argument("--source-dir", required=True)
    parser.add_argument("--target-dir", required=True)
    parser.add_argument("--depfile", required=True)
    parser.add_argument("--stamp", required=True)
    parser.add_argument("--root-build-dir", required=True)
    return parser.parse_args()


def main():
    args = _parse_args()

    # Move the weak-node-api package to the current directory
    source_dir = os.path.abspath(args.source_dir)
    target_dir = os.path.abspath(args.target_dir)
    depfile = os.path.abspath(args.depfile)
    stamp_path = os.path.abspath(args.stamp)
    root_build_dir = os.path.abspath(args.root_build_dir)

    if not os.path.isdir(source_dir):
        print(
            "Error: weak-node-api dependency is missing under the local workspace "
            f"package: {source_dir}"
        )
        print(
            "Please ensure pnpm install includes the workspace package "
            '"./lynx/third_party/weak-node-api" so that '
            '"@lynx-js/weak-node-api" is installed before running GN/Ninja.'
        )
        sys.exit(1)

    # Copy the directory contents directly to current directory
    try:
        missing_items = []

        for item in ITEMS_TO_COPY:
            src_item = os.path.join(source_dir, item)
            if not os.path.exists(src_item):
                missing_items.append(src_item)

        if missing_items:
            print("Error: weak-node-api package is incomplete. Missing required items:")
            for item in missing_items:
                print(f"  - {item}")
            sys.exit(1)

        print(
            "Syncing weak-node-api dependency outputs "
            f"from {source_dir} to {target_dir}"
        )

        for item in ITEMS_TO_COPY:
            src_item = os.path.join(source_dir, item)
            dst_item = os.path.join(target_dir, item)
            _copy_item(src_item, dst_item)

        _copy_weak_napi_headers_to_shim(target_dir)

        for item in OBSOLETE_ITEMS:
            dst_item = os.path.join(target_dir, item)
            if os.path.exists(dst_item):
                _remove_existing(dst_item)

        _write_stamp(stamp_path)
        _write_depfile(
            depfile, stamp_path, _iter_dependency_files(source_dir), root_build_dir
        )

        print("Successfully synced weak-node-api dependency outputs")
    except SyncError as e:
        print(f"Error: {e}")
        sys.exit(1)
    except Exception as e:
        print(f"Error copying items from {source_dir} to {target_dir}: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()
