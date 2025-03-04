#!/bin/bash
#
# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

pushd $(dirname $0)

demo_page_root_dir=$(pwd)
android_assets_dir=$(readlink -f $demo_page_root_dir/../../../explorer/android/lynx_explorer/src/main/assets)
ios_resource_dir=$(readlink -f $demo_page_root_dir/../../../explorer/darwin/ios/lynx_explorer/LynxExplorer/Resource)
mkdir -p $android_assets_dir/automation
mkdir -p $ios_resource_dir/automation

pnpm install
pnpm run build

for path in $(ls ${demo_page_root_dir}); do
    if [ -d "$path" -a "$path" != "node_modules" ]; then
        for dir in ./$path/dist/*/; do
            if [ -d "$dir" ]; then
                template_path="${dir}template.js"
                if [ -e "$template_path" ]; then
                    folder_name=$(basename "$dir")
                    if [ "$folder_name" = "main" ]; then
                        cp $template_path $android_assets_dir/automation/$path.js
                        cp $template_path $ios_resource_dir/automation/$path.js
                    else
                        mkdir -p $android_assets_dir/automation/$folder_name
                        mkdir -p $ios_resource_dir/automation/$folder_name
                        cp $template_path $android_assets_dir/automation/$folder_name/$path.js
                        cp $template_path $ios_resource_dir/automation/$folder_name/$path.js
                    fi
                fi
            fi
        done
    fi
done

popd
