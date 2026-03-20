// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_NATIVE_MODULE_CAPI_H_
#define PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_NATIVE_MODULE_CAPI_H_

#include "lynx_export.h"
#include "third_party/weak-node-api/headers/node_api.h"

#ifdef USE_WEAK_SUFFIX_NAPI
#include "third_party/weak-node-api/headers/weak_napi_defines.h"
#endif

LYNX_EXTERN_C_BEGIN

#define LYNX_NAPI_ENV_LYNX_VIEW_TAG 0xAC7F

typedef napi_value (*napi_module_creator)(napi_env, napi_value exports,
                                          const char* module_name,
                                          void* opaque);

// Stores a data with key to the global map.
LYNX_CAPI_EXPORT void lynx_napi_set_instance_data(napi_env env, uint64_t key,
                                                  void* data,
                                                  napi_finalize finalize_cb,
                                                  void* finalize_hint);
LYNX_CAPI_EXPORT void lynx_napi_get_instance_data(napi_env env, uint64_t key,
                                                  void** data);

LYNX_EXTERN_C_END

#ifdef USE_WEAK_SUFFIX_NAPI
#include "third_party/weak-node-api/headers/weak_napi_undefs.h"
#endif

#endif  // PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_NATIVE_MODULE_CAPI_H_
