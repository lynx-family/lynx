# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
set -e

root_dir=$(pwd)/../../../../
root_dir=$(readlink -f $root_dir)
echo "root_dir: $root_dir"
project_name="LynxExplorer.xcodeproj"
enable_trace=true

usage() {
    echo "Usage: $0 [OPTIONS]"
    echo "Options:"
    echo " -h, --help         Show this help message"
    echo " --skip-card-build  Skip card build task"
    echo " --integration-test  Build integration test demo pages"
    echo " --disable-trace    Disable trace"
}

build_card_resources() {
    mkdir -p $root_dir/explorer/darwin/ios/lynx_explorer/LynxExplorer/Resource
    # build home page card
    pushd $root_dir/explorer/homepage
    pnpm install --no-frozen-lockfile
    pnpm run build
    cp $root_dir/explorer/homepage/dist/main.lynx.bundle $root_dir/explorer/darwin/ios/lynx_explorer/LynxExplorer/Resource/homepage.lynx.bundle
    popd

    if [[ "$SKIP_CARD_BUILD" == "false" ]]; then
        # build showcase cards
        python3 $root_dir/explorer/showcase/build_and_copy.py
    fi

    if [[ "$INTEGRATION_TEST" == "true" ]]; then
        # build integration test demo pages
        python3 $root_dir/testing/integration_test/demo_pages/build_and_copy.py
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
            --disable-trace)
                enable_trace=false
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

enable_trace_param=$([ $enable_trace == true ] && echo "--enable-trace" || echo "")

handle_options "$@"
build_card_resources

pushd $root_dir
gn_root_dir=$(readlink -f $root_dir)
echo "gn_root_dir: $gn_root_dir"
generate_ios_podspec_cmd="python3 tools/ios_tools/generate_podspec_scripts_by_gn.py --root $gn_root_dir $enable_trace_param --enable-autosync-version"
echo $generate_ios_podspec_cmd
eval "$generate_ios_podspec_cmd"
popd

# prepare source cache
export COCOAPODS_CONVERT_GIT_TO_HTTP=false
export LANG=en_US.UTF-8
BUNDLE_GEMFILE="$root_dir/Gemfile"
pod_cmd="pod"
if command -v bundle >/dev/null 2>&1 && [ -f "$BUNDLE_GEMFILE" ]; then
    pushd "$root_dir"
    bundle config set path "$root_dir/.bundle"
    SDKROOT=/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk bundle install
    popd
    pod_cmd="bundle exec pod"
fi

repo_update_flag="--no-repo-update"
if [ ! -d "$HOME/.cocoapods/repos/trunk" ]; then
    repo_update_flag="--repo-update"
fi

$pod_cmd deintegrate "$project_name" || true
if [ "${LYNX_POD_RESET_LOCK:-0}" == "1" ]; then
    rm -rf Podfile.lock
fi

if [ -n "${source_cache_dir:-}" ] && [ -d "$source_cache_dir/.git" ]; then
    COCOAPODS_LOCAL_SOURCE_REPO="$source_cache_dir/.git" $pod_cmd install $repo_update_flag
else
    $pod_cmd install $repo_update_flag
fi
