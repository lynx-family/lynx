// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/host_script_binding_harmony.h"

namespace lynx {
namespace harmony {

// Selected by GN for builds without Host Script support. No JS bindings or
// runtime implementation are linked into these builds.
void InitHostScriptBinding(napi_env env, napi_value exports) {}

}  // namespace harmony
}  // namespace lynx
