// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_EMBEDDER_MODULE_LYNX_FETCH_MODULE_H_
#define PLATFORM_EMBEDDER_MODULE_LYNX_FETCH_MODULE_H_

#include "platform/embedder/public/capi/lynx_native_module_capi.h"

#ifdef USE_WEAK_SUFFIX_NAPI
#include "third_party/weak-node-api/headers/weak_napi_defines.h"
#endif

namespace lynx {
namespace embedder {

napi_value LynxFetchModuleCreator(napi_env, napi_value exports,
                                  const char* module_name, void* opaque);

}
}  // namespace lynx

#ifdef USE_WEAK_SUFFIX_NAPI
#include "third_party/weak-node-api/headers/weak_napi_undefs.h"
#endif

#endif  // PLATFORM_EMBEDDER_MODULE_LYNX_FETCH_MODULE_H_
