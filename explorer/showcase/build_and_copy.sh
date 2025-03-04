#!/bin/bash
#
# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

LYNX_EXAMPLE_DIR_NAME="@lynx-example"

showcase_root_dir=$(cd "$(dirname "$0")"; pwd)
explorer_dir=$(cd "$(dirname "$showcase_root_dir")"; pwd)

pushd $(dirname $0)
android_assets_dir="$explorer_dir/android/lynx_explorer/src/main/assets"
if [ ! -d $android_assets_dir ]; then
    mkdir -p $android_assets_dir
fi
ios_resource_dir="$explorer_dir/darwin/ios/lynx_explorer/LynxExplorer/Resource"
if [ ! -d $ios_resource_dir ]; then
    mkdir -p $ios_resource_dir
fi
rm -rf $android_assets_dir/showcase
rm -rf $ios_resource_dir/showcase
mkdir -p $android_assets_dir/showcase
mkdir -p $ios_resource_dir/showcase

echo "========== build showcase page =========="

pnpm install --frozen-lockfile
pnpm run build

echo "========== copy showcase resource =========="
pushd "$showcase_root_dir/node_modules/$LYNX_EXAMPLE_DIR_NAME"
for path in $(ls); do
    mkdir -p $android_assets_dir/showcase/$path
    mkdir -p $ios_resource_dir/showcase/$path
    for filename in $(ls ./$path/dist/); do
        if [[ "$filename" == *.lynx.bundle ]]; then
            cp ./$path/dist/$filename $android_assets_dir/showcase/$path/
            cp ./$path/dist/$filename $ios_resource_dir/showcase/$path/
        fi
    done
done
popd

pushd $showcase_root_dir
mkdir -p $android_assets_dir/showcase/menu/
mkdir -p $ios_resource_dir/showcase/menu/
for filename in $(ls ./menu/dist); do
    if [[ "$filename" == *.lynx.bundle ]]; then
        cp ./menu/dist/$filename $android_assets_dir/showcase/menu/
        cp ./menu/dist/$filename $ios_resource_dir/showcase/menu/
    fi
done
popd
