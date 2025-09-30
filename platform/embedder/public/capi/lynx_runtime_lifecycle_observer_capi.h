// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_RUNTIME_LIFECYCLE_OBSERVER_CAPI_H_
#define PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_RUNTIME_LIFECYCLE_OBSERVER_CAPI_H_

#include "lynx_export.h"
#include "third_party/napi/include/js_native_api.h"
#include "third_party/napi/include/js_native_api_types.h"
#ifdef USE_PRIMJS_NAPI
#include "third_party/napi/include/primjs_napi_defines.h"
#endif

LYNX_EXTERN_C_BEGIN

typedef struct lynx_runtime_lifecycle_observer_t
    lynx_runtime_lifecycle_observer_t;

// Define a function pointer type for the runtime attach callback.
// This callback is invoked when the runtime is attached.
typedef void (*runtime_attach_callback)(lynx_runtime_lifecycle_observer_t*,
                                        napi_env);

// Define a function pointer type for the runtime detach callback.
// This callback is invoked when the runtime is detached.
typedef void (*runtime_detach_callback)(lynx_runtime_lifecycle_observer_t*);

// Create a new lynx_runtime_lifecycle_observer_t instance. The instance's
// lifecycle is based on reference count.
LYNX_CAPI_EXPORT lynx_runtime_lifecycle_observer_t*
lynx_runtime_lifecycle_observer_create(void* user_data);

// Create a new lynx_runtime_lifecycle_observer_t instance with a finalizer.
// The finalizer will be called when the observer is released.
// The instance's lifecycle is based on reference count.
LYNX_CAPI_EXPORT lynx_runtime_lifecycle_observer_t*
lynx_runtime_lifecycle_observer_create_with_finalizer(
    void* user_data,
    void (*finalizer)(lynx_runtime_lifecycle_observer_t*, void*));

LYNX_CAPI_EXPORT void* lynx_runtime_lifecycle_observer_get_user_data(
    lynx_runtime_lifecycle_observer_t*);

// Bind an attach callback to a lynx_runtime_lifecycle_observer_t instance.
LYNX_CAPI_EXPORT void lynx_runtime_lifecycle_observer_bind_attach_callback(
    lynx_runtime_lifecycle_observer_t*, runtime_attach_callback);

// Bind a detach callback to a lynx_runtime_lifecycle_observer_t instance.
LYNX_CAPI_EXPORT void lynx_runtime_lifecycle_observer_bind_detach_callback(
    lynx_runtime_lifecycle_observer_t*, runtime_detach_callback);

// Release a lynx_runtime_lifecycle_observer_t instance. The release
// operation is based on the reference count.
LYNX_CAPI_EXPORT void lynx_runtime_lifecycle_observer_release(
    lynx_runtime_lifecycle_observer_t*);

LYNX_EXTERN_C_END

#ifdef USE_PRIMJS_NAPI
#include "third_party/napi/include/primjs_napi_undefs.h"
#endif

#endif  // PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_RUNTIME_LIFECYCLE_OBSERVER_CAPI_H_
