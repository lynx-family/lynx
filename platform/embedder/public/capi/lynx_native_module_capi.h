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

// Runs a script from the provided source content in the given NAPI
// environment. This is not part of the standard NAPI surface, but Lynx needs
// to expose it as an embedder-specific helper for loading and evaluating script
// content with explicit source metadata. `filename` is used for script
// identification in diagnostics and stack traces.
LYNX_CAPI_EXPORT napi_status
lynx_napi_run_script_from_file(napi_env env, const char* script, size_t length,
                               const char* filename, napi_value* result);

// Reports the current exception associated with the given NAPI environment and
// then clears the recorded exception state. This is also a Lynx-specific
// extension instead of a standard NAPI API, provided because the embedder needs
// an explicit hook to consume and reset pending exception information.
LYNX_CAPI_EXPORT void lynx_napi_report_and_clear_exception(napi_env env);

LYNX_EXTERN_C_END

#ifdef USE_WEAK_SUFFIX_NAPI
#include "third_party/weak-node-api/headers/weak_napi_undefs.h"
#endif

#endif  // PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_NATIVE_MODULE_CAPI_H_
