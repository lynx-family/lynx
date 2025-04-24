#!/usr/bin/env bash
# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

set -e

echo "Start diff changed files for publish"

projectRootPath="$(cd "$(dirname "$0")"; pwd -P)/../.."
python3 "$projectRootPath/tools/js_tools/modify_version_and_changelog.py" "$projectRootPath/package.json" "$projectRootPath/js_libraries/types"
if [ $? -eq 0 ]; then
   echo "Modify version and changelog success"
   pushd "$projectRootPath/js_libraries/types"
   npm publish --dry-run
   cat ./CHANGELOG.md
   popd
else
   echo "Modify version and changelog failed"
   exit 1
fi
