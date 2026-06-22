// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef EXPLORER_WINDOWS_LYNX_EXPLORER_MODULE_LYNX_NODE_API_MODULE_H_
#define EXPLORER_WINDOWS_LYNX_EXPLORER_MODULE_LYNX_NODE_API_MODULE_H_

#include "lynx_native_module.h"

#ifdef USE_WEAK_SUFFIX_NAPI
#include "third_party/weak-node-api/headers/weak_napi_defines.h"
#endif

napi_value LynxNodeAPIModuleCreator(napi_env env, napi_value exports,
                                    const char* module_name, void* opaque);

#ifdef USE_WEAK_SUFFIX_NAPI
#include "third_party/weak-node-api/headers/weak_napi_undefs.h"
#endif

#endif  // EXPLORER_WINDOWS_LYNX_EXPLORER_MODULE_LYNX_NODE_API_MODULE_H_
