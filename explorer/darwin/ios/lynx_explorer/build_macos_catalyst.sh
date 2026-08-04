#!/bin/bash
# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
set -euo pipefail

build_arch="${1:-arm64}"
derived_data_path="${DERIVED_DATA_PATH:-iOSCoreBuild/DerivedData-Catalyst-$build_arch}"
result_bundle_path="${RESULT_BUNDLE_PATH:-results_bundle_catalyst_$build_arch.xcresult}"

case "$build_arch" in
  arm64 | x86_64) ;;
  *)
    echo "Unsupported Mac Catalyst architecture: $build_arch" >&2
    exit 1
    ;;
esac

rm -rf "$result_bundle_path"

xcodebuild \
  -workspace LynxExplorer.xcworkspace \
  -scheme LynxExplorer \
  -configuration Debug \
  -destination "generic/platform=macOS,variant=Mac Catalyst" \
  -derivedDataPath "$derived_data_path" \
  -resultBundlePath "$result_bundle_path" \
  -showBuildTimingSummary \
  ARCHS="$build_arch" \
  SUPPORTS_MACCATALYST=YES \
  CODE_SIGNING_ALLOWED=NO \
  COMPILER_INDEX_STORE_ENABLE=NO \
  -jobs 24 \
  build

product_path="$derived_data_path/Build/Products/Debug-maccatalyst/LynxExplorer.app"
if [[ ! -d "$product_path" ]]; then
  echo "Mac Catalyst app was not produced at $product_path" >&2
  exit 1
fi
