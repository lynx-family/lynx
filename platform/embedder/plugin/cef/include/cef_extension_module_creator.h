// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_EMBEDDER_PLUGIN_CEF_INCLUDE_CEF_EXTENSION_MODULE_CREATOR_H_
#define PLATFORM_EMBEDDER_PLUGIN_CEF_INCLUDE_CEF_EXTENSION_MODULE_CREATOR_H_

#include "platform/embedder/public/capi/lynx_export.h"
#include "platform/embedder/public/lynx_extension_module.h"

LYNX_EXTERN_C_BEGIN
LYNX_CAPI_EXPORT bool cef_extension_module_initialize();

LYNX_CAPI_EXPORT lynx_extension_module_t*
cef_extension_module_create_extension_module(void* opaque);
LYNX_EXTERN_C_END

#endif  // PLATFORM_EMBEDDER_PLUGIN_CEF_INCLUDE_CEF_EXTENSION_MODULE_CREATOR_H_
