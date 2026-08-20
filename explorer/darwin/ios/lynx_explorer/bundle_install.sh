# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
set -e

script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
root_dir=$(readlink -f "$script_dir/../../../..")
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
    echo " --sparkling-mode enable_sparkling|disable_sparkling"
    echo "                    Select pinned Sparkling sources (default: disable_sparkling)"
}

build_card_resources() {
    resource_dir=$root_dir/explorer/darwin/ios/lynx_explorer/LynxExplorer/Resource
    sparkling_go_resource_dir=$resource_dir/extensions/sparkling-go
    mkdir -p "$resource_dir"
    # build home page card
    pushd $root_dir/explorer/homepage
    pnpm install --no-frozen-lockfile
    pnpm run build
    cp $root_dir/explorer/homepage/dist/main.lynx.bundle $root_dir/explorer/darwin/ios/lynx_explorer/LynxExplorer/Resource/homepage.lynx.bundle
    popd

    if [[ "$SPARKLING_MODE" == "enable_sparkling" ]]; then
        # Sparkling Go is owned and built by the pinned upstream Sparkling
        # checkout. Explorer only packages its official playground bundles.
        sparkling_go_dist=$SPARKLING_SOURCE_ROOT/packages/playground/dist
        if [[ ! -f "$sparkling_go_dist/main.lynx.bundle" ]]; then
            echo "error: Sparkling Go bundles are missing; build the pinned playground first" >&2
            exit 1
        fi
        rm -rf "$sparkling_go_resource_dir"
        mkdir -p "$sparkling_go_resource_dir"
        cp -R "$sparkling_go_dist"/. "$sparkling_go_resource_dir/"
    else
        # A mode switch must not leave an enabled-only extension in the app.
        rm -rf "$sparkling_go_resource_dir"
    fi

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
    while [[ $# -gt 0 ]]; do
        case "$1" in
            -h | --help)
                usage
                exit 0
                ;;
            --skip-card-build)
                SKIP_CARD_BUILD=true
                shift
                ;;
            --integration-test)
                INTEGRATION_TEST=true
                shift
                ;;
            --disable-trace)
                enable_trace=false
                shift
                ;;
            --sparkling-mode)
                if [[ $# -lt 2 ]]; then
                    echo "error: --sparkling-mode requires enable_sparkling or disable_sparkling" >&2
                    exit 1
                fi
                SPARKLING_MODE=$2
                shift 2
                ;;
            --sparkling-mode=*)
                SPARKLING_MODE=${1#*=}
                shift
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
SPARKLING_MODE=${SPARKLING_MODE:-disable_sparkling}
SPARKLING_SOURCE_ROOT=${SPARKLING_SOURCE_ROOT:-generated/sparkling-source}

handle_options "$@"

if [[ "$SPARKLING_MODE" != "enable_sparkling" && "$SPARKLING_MODE" != "disable_sparkling" ]]; then
    echo "error: sparkling mode must be enable_sparkling or disable_sparkling, got '$SPARKLING_MODE'" >&2
    exit 1
fi

SPARKLING_SOURCE_ROOT=$(python3 -c \
    'import os, sys; print(os.path.abspath(os.path.join(sys.argv[1], sys.argv[2])))' \
    "$script_dir" "$SPARKLING_SOURCE_ROOT")
export SPARKLING_MODE
export SPARKLING_SOURCE_ROOT

enable_trace_param=$([ "$enable_trace" == true ] && echo "--enable-trace" || echo "")

if [[ "$SPARKLING_MODE" == "enable_sparkling" ]]; then
    python3 "$script_dir/scripts/sync_sparkling_source.py" \
        --manifest "$script_dir/sparkling-source.json" \
        --source-root "$SPARKLING_SOURCE_ROOT"
fi

build_card_resources

pushd $root_dir
gn_root_dir=$(readlink -f $root_dir)
echo "gn_root_dir: $gn_root_dir"
generate_ios_podspec_cmd="python3 tools/ios_tools/generate_podspec_scripts_by_gn.py --root $gn_root_dir $enable_trace_param --enable-autosync-version"
echo $generate_ios_podspec_cmd
eval "$generate_ios_podspec_cmd"
popd

export COCOAPODS_CONVERT_GIT_TO_HTTP=false
export LANG=en_US.UTF-8
pushd "$script_dir"

source_list="$script_dir/Podfile.flatten"
if [[ -f "$source_list" ]] && grep -qE '^[[:space:]]*[^#[:space:]]' "$source_list"; then
    source_cache_dir="$HOME/.cocoapods/spec-repo-lynx-explorer"
    python3 "$root_dir/tools/ios_tools/prepare_cocoapods_sources.py" \
        --source-list "$source_list" \
        --cache-dir "$source_cache_dir"
    export COCOAPODS_LOCAL_SOURCE_REPO="$source_cache_dir/.git"
fi

pod deintegrate "$project_name"
pod install
python3 "$script_dir/scripts/verify_sparkling_ownership.py" \
    --mode "$SPARKLING_MODE" \
    --lockfile "$script_dir/Podfile.lock" \
    --manifest "$script_dir/sparkling-source.json" \
    --source-root "$SPARKLING_SOURCE_ROOT" \
    --lynx-root "$root_dir"
popd
