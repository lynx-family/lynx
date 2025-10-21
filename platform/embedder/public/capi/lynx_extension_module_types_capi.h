// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_EXTENSION_MODULE_TYPES_CAPI_H_
#define PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_EXTENSION_MODULE_TYPES_CAPI_H_

#include "lynx_export.h"

LYNX_EXTERN_C_BEGIN

typedef struct lynx_extension_module_t lynx_extension_module_t;

typedef lynx_extension_module_t* (*extension_module_creator)(void* opaque);

LYNX_EXTERN_C_END

#endif  // PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_EXTENSION_MODULE_TYPES_CAPI_H_
