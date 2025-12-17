#!/usr/bin/env python3
# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

# This is a helper script for cache key calculation on GitHub Actions.
import argparse
import os
import importlib.util as iu
import importlib.machinery as im
import hashlib

from pathlib import Path


# tools/ci/habitat_cache_helper.py
LYNX_ROOT_DIR = Path(__file__).parents[2]

# for cache action's field "restore-keys"
HABITAT_TARGETS = "HABITAT_TARGETS"
HABITAT_DEPS_FILE_DIGEST = "HABITAT_DEPS_FILE_DIGEST"


def set_output(name: str, value: str) -> None:
    with open(os.environ["GITHUB_OUTPUT"], "a") as f:
        f.write(f"{name}={value}{os.linesep}")


def split_targets(target: str) -> list[str]:
    targets = []
    for t in target.split(","):
        if t.strip():
            targets.append(t.strip())

    return sorted(targets)


def deps_files_path(targets: list) -> list[Path]:
    habitat_path = LYNX_ROOT_DIR / ".habitat"
    spec = iu.spec_from_loader(
        "habitat", im.SourceFileLoader("habitat", str(habitat_path))
    )
    habitat = iu.module_from_spec(spec)
    spec.loader.exec_module(habitat)

    # take .habitat file into account as well
    res = [habitat_path, LYNX_ROOT_DIR / habitat.solutions[0]["deps_file"]]
    res.extend(
        [
            LYNX_ROOT_DIR / habitat.solutions[0]["target_deps_files"][target]
            for target in targets
        ]
    )

    return res


def get_digest(paths: list[Path]) -> str:
    digest = hashlib.md5()
    for path in paths:
        with open(path, "rb") as f:
            digest.update(f.read())

        current = digest.hexdigest()
        print(f"after calculating {path}, the current digest is {current}")

    return digest.hexdigest()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--target", type=str, default="")
    args = parser.parse_args()

    targets = split_targets(args.target)
    paths = deps_files_path(targets)

    # ...-${{ runner.os }}-${{ steps.hab.outputs.HABITAT_TARGETS }}${{ steps.hab.outputs.HABITAT_DEPS_FILE_DIGEST }}
    # for the cache key of deps with no target:
    #  - key: hab-macOS-[]-abc123
    #  - restore-keys: hab-macOS-[]-
    # for the cache key of deps with extra targets:
    #  - key: hab-macOS-[dev]-abc123
    #  - restore-keys: hab-macOS-[dev]-
    target_key = f"[{'-'.join(targets)}]-"
    digest_key = get_digest(paths)

    set_output(HABITAT_TARGETS, target_key)
    set_output(HABITAT_DEPS_FILE_DIGEST, digest_key)


main()
