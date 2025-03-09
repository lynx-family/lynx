#!/bin/bash
#
# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

pushd $(dirname $0)

demo_page_root_dir=$(pwd)
explorer_dir=$(cd "$(dirname $(dirname $(dirname "$demo_page_root_dir")))"; pwd)
echo "explorer_dir: " $explorer_dir
android_assets_dir="$explorer_dir/explorer/android/lynx_explorer/src/main/assets"
if [ ! -d $android_assets_dir ]; then
    mkdir -p $android_assets_dir
fi
ios_resource_dir="$explorer_dir/explorer/darwin/ios/lynx_explorer/LynxExplorer/Resource"
if [ ! -d $ios_resource_dir ]; then
    mkdir -p $ios_resource_dir
fi
rm -rf $android_assets_dir/automation
mkdir $android_assets_dir/automation
rm -rf $ios_resource_dir/automation
mkdir $ios_resource_dir/automation

echo "========== build integration test demo pages =========="
pnpm install --frozen-lockfile
pnpm run build

echo "========== copy integration test demo pages resource=========="
for path in $(ls ${demo_page_root_dir}); do
    if [ -d "$path" -a "$path" != "node_modules" ]; then
        mkdir -p $android_assets_dir/automation/$path
        mkdir -p $ios_resource_dir/automation/$path
        for filename in $(ls ./$path/dist/); do
            if [[ "$filename" == *.lynx.bundle ]]; then
                cp ./$path/dist/$filename $android_assets_dir/automation/$path/
                cp ./$path/dist/$filename $ios_resource_dir/automation/$path/
            fi
        done
    fi
done
popd
