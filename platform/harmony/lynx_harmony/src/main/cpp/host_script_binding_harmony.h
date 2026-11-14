// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_HOST_SCRIPT_BINDING_HARMONY_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_HOST_SCRIPT_BINDING_HARMONY_H_

#include <node_api.h>

namespace lynx {
namespace harmony {
// Release builds intentionally export no Host Script binding.
void InitHostScriptBinding(napi_env env, napi_value exports);
}  // namespace harmony
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_HOST_SCRIPT_BINDING_HARMONY_H_
