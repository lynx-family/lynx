#!/bin/bash
# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
set -e

current_dir=$(cd "$(dirname "$0")"; pwd)
root_path=$(cd "$(dirname $(dirname $(dirname $(dirname "$current_dir"))))"; pwd)
dist_path="$root_path/devtool/lynx_devtool/resources/lynx-logbox/dist"
android_target_path="$root_path/platform/android/lynx_devtool/src/main/assets/logbox"
ios_target_path="$root_path/platform/darwin/ios/lynx_devtool/assets/logbox"

function build() {
    pushd "$root_path"
    mkdir -p "$dist_path"
    pnpm --filter @lynx-js/logbox build
    
    rm -rf "$android_target_path"
    rm -rf "$ios_target_path"
    mkdir -p "$android_target_path"
    mkdir -p "$ios_target_path"

    cp -r "$dist_path/." "$android_target_path/"
    cp -r "$dist_path/." "$ios_target_path/"
}

build