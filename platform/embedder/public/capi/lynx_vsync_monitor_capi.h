// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_VSYNC_MONITOR_CAPI_H_
#define PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_VSYNC_MONITOR_CAPI_H_

#include <stdint.h>

#include "lynx_export.h"

LYNX_EXTERN_C_BEGIN

typedef struct lynx_vsync_monitor_t lynx_vsync_monitor_t;

typedef void (*lynx_vsync_monitor_on_vsync_callback)(void* user_data, int64_t,
                                                     int64_t);
typedef void (*lynx_vsync_monitor_request_vsync)(
    lynx_vsync_monitor_t*, lynx_vsync_monitor_on_vsync_callback callback,
    void* user_data);

// Creates a lynx_vsync_monitor_t instance with a finalizer. It associates
// the provided user data with the monitor, which can be retrieved later using
// `lynx_vsync_monitor_get_user_data`. The finalizer function will be called
// when the monitor is released.
LYNX_CAPI_EXPORT lynx_vsync_monitor_t* lynx_vsync_monitor_create_with_finalizer(
    void* user_data, void (*finalizer)(lynx_vsync_monitor_t*, void*));

LYNX_CAPI_EXPORT void* lynx_vsync_monitor_get_user_data(lynx_vsync_monitor_t*);

// Binds a request_vsync function to lynx_vsync_monitor_t. This function is used
// to synchronize a Vsync signal.
LYNX_CAPI_EXPORT void lynx_vsync_monitor_bind_request_vsync_func(
    lynx_vsync_monitor_t*, lynx_vsync_monitor_request_vsync func);

LYNX_EXTERN_C_END

#endif  // PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_VSYNC_MONITOR_CAPI_H_
