#!/usr/bin/env bash
# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"
RTF_PATH="${PATH}"
if ! command -v sysctl >/dev/null 2>&1; then
  RTF_TOOLS="$(mktemp -d)"
  trap 'rm -rf "${RTF_TOOLS}"' EXIT
  printf '#!/usr/bin/env sh\nexit 0\n' > "${RTF_TOOLS}/sysctl"
  chmod +x "${RTF_TOOLS}/sysctl"
  RTF_PATH="${RTF_TOOLS}:${RTF_PATH}"
fi

"${ROOT_DIR}/tools/env.sh" pnpm --filter @lynx-js/node-lynx run build
cp \
  "${ROOT_DIR}/oliver/node-lynx/build/linux/Release/node_lynx.node" \
  "${ROOT_DIR}/oliver/node-lynx/platform/linux-x64/"
PATH="${RTF_PATH}" "${ROOT_DIR}/tools/env.sh" tools/rtf/rtf native-ut run \
  --names lynx \
  --target starlight_geometry_unittest_exec \
  --disable-flutter-cxx
"${ROOT_DIR}/tools/env.sh" pnpm \
  --filter @lynx-js/grid-lanes-conformance run install:browsers
"${ROOT_DIR}/tools/env.sh" pnpm --filter @lynx-js/grid-lanes-conformance run build
"${ROOT_DIR}/tools/env.sh" pnpm --filter @lynx-js/grid-lanes-conformance run test:unit
"${ROOT_DIR}/tools/env.sh" pnpm --filter @lynx-js/grid-lanes-conformance run test:calibration
"${ROOT_DIR}/tools/env.sh" pnpm --filter @lynx-js/grid-lanes-conformance run test
