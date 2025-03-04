# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
set -e

root_dir=$(pwd)/../../../../
root_dir=$(readlink -f $root_dir)
echo "root_dir: $root_dir"
command="pod install --verbose --repo-update"
project_name="LynxExplorer.xcodeproj"

usage() {
    echo "Usage: $0 [OPTIONS]"
    echo "Options:"
    echo " -h, --help         Show this help message"
    echo " --skip-card-build  Skip card build task"
    echo " --integration-test  Build integration test demo pages"
}

build_card_resources() {
    mkdir -p $root_dir/explorer/darwin/ios/lynx_explorer/LynxExplorer/Resource

    # build home page card
    pushd $root_dir/explorer/homepage
    pnpm install
    pnpm run build
    cp $root_dir/explorer/homepage/dist/main.lynx.bundle $root_dir/explorer/darwin/ios/lynx_explorer/LynxExplorer/Resource/homepage.lynx.bundle
    popd

    if [[ "$SKIP_CARD_BUILD" == "false" ]]; then
        # build showcase cards
        $root_dir/explorer/showcase/build_and_copy.sh
    fi

    if [[ "$INTEGRATION_TEST" == "true" ]]; then
        # build integration test demo pages
        $root_dir/testing/integration_test/demo_pages/build_and_copy.sh
    fi
}

handle_options() {
    for i in "$@"; do
        case $i in
            -h | --help)
                usage
                exit 0
                ;;
            --skip-card-build)
                SKIP_CARD_BUILD=true
                ;;
            --integration-test)
                INTEGRATION_TEST=true
                ;;
            *)
                usage
                exit 1
                ;;
        esac
    done
}

SKIP_CARD_BUILD=false
INTEGRATION_TEST=false

handle_options "$@"
build_card_resources

pushd $root_dir
gn_root_dir=$root_dir/../
gn_root_dir=$(readlink -f $gn_root_dir)
generate_ios_podspec_cmd="python3 tools/ios_tools/generate_podspec_scripts_by_gn.py --root $gn_root_dir"
echo $generate_ios_podspec_cmd
eval "$generate_ios_podspec_cmd"
for file in ./*.podspec; do
    if [ -e "$file" ]; then
        mv "$file" "$gn_root_dir"
    fi
done
popd

# prepare source cache
export COCOAPODS_CONVERT_GIT_TO_HTTP=false
export LANG=en_US.UTF-8
SDKROOT=/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk bundle install -V --path="$root_dir"
bundle exec pod deintegrate "$project_name"
rm -rf Podfile.lock
COCOAPODS_LOCAL_SOURCE_REPO=$source_cache_dir/.git bundle exec "$command"
