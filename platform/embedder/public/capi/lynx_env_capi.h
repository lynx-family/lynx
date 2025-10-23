// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_ENV_CAPI_H_
#define PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_ENV_CAPI_H_

#include "lynx_export.h"
#include "lynx_extension_module_types_capi.h"
#include "lynx_native_module_capi.h"

LYNX_EXTERN_C_BEGIN

// Retrieves the SDK version of Lynx sdk.
LYNX_CAPI_EXPORT const char* lynx_env_get_sdk_version();

// Lynx devtools
LYNX_CAPI_EXPORT void lynx_env_set_devtool_app_info(const char* name,
                                                    const char* value);
LYNX_CAPI_EXPORT void lynx_env_enable_devtool(int enable);
LYNX_CAPI_EXPORT int lynx_env_is_devtool_enabled();
LYNX_CAPI_EXPORT int lynx_env_connect_devtool(const char* url);
// logbox
LYNX_CAPI_EXPORT void lynx_env_enable_logbox(int enable);
LYNX_CAPI_EXPORT int lynx_env_is_logbox_enabled();
// Global native module
LYNX_CAPI_EXPORT void lynx_env_register_native_module(
    const char* name, napi_module_creator creator, void* opaque);
// Global extension module
LYNX_CAPI_EXPORT void lynx_env_register_extension_module(
    const char* name, extension_module_creator creator, bool is_lazy_create,
    void* opaque);

LYNX_EXTERN_C_END

#endif  // PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_ENV_CAPI_H_
