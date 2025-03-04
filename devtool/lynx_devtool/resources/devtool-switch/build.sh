#!/bin/bash
# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

set -e

current_dir=$(cd "$(dirname "$0")"; pwd)
root_dir=$(cd "$(dirname $(dirname $(dirname $(dirname "$current_dir"))))"; pwd)
template_dir="dist/devtoolSwitch.lynx.bundle"
android_target_dir="$root_dir/platform/android/lynx_devtool/src/main/assets/devtool_switch"
ios_target_dir="$root_dir/platform/darwin/ios/lynx_devtool/assets"
switch_page_dir="switchPage/"

output=$1

echo "========== build devtool switch page =========="
cd $current_dir
pnpm build

echo "========== copy devtool switch resource =========="
if [[ ! -z "$output" ]]; then
    echo $output/$switch_page_dir
    mkdir -p $output/$switch_page_dir
    cp $current_dir/$template_dir $output/$switch_page_dir
fi

rm -rf $android_target_dir
rm -rf $ios_target_dir/$switch_page_dir
mkdir -p $android_target_dir/$switch_page_dir
mkdir -p $ios_target_dir/$switch_page_dir
cp $current_dir/$template_dir $android_target_dir/$switch_page_dir
cp $current_dir/$template_dir $ios_target_dir/$switch_page_dir