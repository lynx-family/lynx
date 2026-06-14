// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_TRAIL_SERVICE_CAPI_H_
#define PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_TRAIL_SERVICE_CAPI_H_

#include "lynx_export.h"

LYNX_EXTERN_C_BEGIN

typedef struct lynx_trail_service_t lynx_trail_service_t;

// Returns the string value for `key`. The returned pointer must remain valid
// for this callback invocation, and callers must copy it immediately.
typedef const char* (*lynx_trail_string_value_func)(
    lynx_trail_service_t* trail_service, const char* key);

LYNX_CAPI_EXPORT lynx_trail_service_t* lynx_trail_service_create(
    void* user_data);

LYNX_CAPI_EXPORT lynx_trail_service_t* lynx_trail_service_create_with_finalizer(
    void* user_data, void (*finalizer)(lynx_trail_service_t*, void*));

LYNX_CAPI_EXPORT void* lynx_trail_service_get_user_data(
    lynx_trail_service_t* trail_service);

LYNX_CAPI_EXPORT void lynx_trail_service_bind(
    lynx_trail_service_t* trail_service, lynx_trail_string_value_func f);

LYNX_CAPI_EXPORT void lynx_trail_service_release(
    lynx_trail_service_t* trail_service);

LYNX_EXTERN_C_END

#endif  // PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_TRAIL_SERVICE_CAPI_H_
