#!/usr/bin/env bash
# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PACKAGE_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
export npm_config_cache="${NODE_LYNX_NPM_CACHE:-${TMPDIR:-/tmp}/node-lynx-npm-cache}"
mkdir -p "${npm_config_cache}"

case "$(uname -s)" in
  Darwin)
    PLATFORM="darwin"
    ARCH="arm64"
    ;;
  Linux)
    PLATFORM="linux"
    ARCH="x64"
    ;;
  *)
    echo "unsupported os type: $(uname -s)" >&2
    exit 1
    ;;
esac

NATIVE_ADDON="${PACKAGE_ROOT}/build/${PLATFORM}/Release/node_lynx.node"
PLATFORM_DIR="${PACKAGE_ROOT}/platform/${PLATFORM}-${ARCH}"

cd "${PACKAGE_ROOT}"
pnpm run build
cp "${NATIVE_ADDON}" "${PLATFORM_DIR}/"
pnpm exec tsc --noEmit

node test/cli_debug_router_no_initial_template.js
node test/remote_template_smoke.js
node test/local_template_dom_tree.js
node test/open_card_manager.js
node test/protocol_smoke.js
