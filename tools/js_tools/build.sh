#!/bin/bash
# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

set -e
set -x

rootPath="$(cd "$(dirname "$0")"; pwd -P)/../.."
outputPath="$rootPath/js_libraries/lynx-core/output"

function usage() {
    cat << EOS
    Usage: $0 platform [release_output] [dev_output] [version]
    Options:
        platform        The platform to build [android, ios]
        release_output  The output of lynx_core.js
        dev_output      The output of lynx_core_dev.js
        version         The version of lynx_core.js
EOS
}

platform="$1"
releaseOutput="$2"
devOutput="$3"
version="$4"

function build() {
    pushd "$rootPath"
    pnpm --filter @lynx-js/runtime-shared build

    # bundle lynx_core.js and lynx_core_dev.js
    NODE_OPTIONS="--max-old-space-size=8192" \
    version="$version" pnpm --filter @lynx-js/lynx-core "build:$platform"

    if [[ ! -z "$releaseOutput" ]]; then
        mkdir -p "$(dirname "$releaseOutput")"
        cp "$outputPath/lynx_core.js" "$releaseOutput"
    fi

    if [[ ! -z "$devOutput" ]]; then
        mkdir -p "$(dirname "$devOutput")"
        cp "$outputPath/lynx_core_dev.js" "$devOutput"
    fi
    popd
}

if [[ -z "$1" ]] || [[ "$1" == "--help" ]] || [[ "$1" == "-h" ]]; then
    usage
    exit 1
fi

build
