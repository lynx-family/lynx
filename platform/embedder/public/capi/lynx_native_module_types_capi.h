// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_NATIVE_MODULE_TYPES_CAPI_H_
#define PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_NATIVE_MODULE_TYPES_CAPI_H_

#include "lynx_export.h"

LYNX_EXTERN_C_BEGIN

typedef struct napi_env__* napi_env;
typedef struct napi_value__* napi_value;

typedef napi_value (*napi_module_creator)(napi_env, napi_value exports,
                                          const char* module_name,
                                          void* opaque);

LYNX_EXTERN_C_END

#endif  // PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_NATIVE_MODULE_TYPES_CAPI_H_
