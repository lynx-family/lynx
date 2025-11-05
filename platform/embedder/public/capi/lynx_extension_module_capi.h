// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_EXTENSION_MODULE_CAPI_H_
#define PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_EXTENSION_MODULE_CAPI_H_

#include <stdint.h>

#include "lynx_export.h"
#include "lynx_extension_module_types_capi.h"
#include "lynx_generic_resource_fetcher_capi.h"
#include "lynx_native_module_capi.h"
#include "lynx_view_capi.h"
#include "third_party/napi/include/js_native_api.h"
#ifdef USE_PRIMJS_NAPI
#include "third_party/napi/include/primjs_napi_defines.h"
#endif

LYNX_EXTERN_C_BEGIN

typedef struct lynx_vsync_observer_t lynx_vsync_observer_t;

typedef void (*vsync_observer_callback)(void* user_data, int64_t, int64_t);

// To synchronize with a given VSync, a callback is set, which is called back
// when the next VSync signal arrives. This callback will be called before any
// normal callback.
LYNX_CAPI_EXPORT void lynx_vsync_observer_request_before_animation_frame(
    lynx_vsync_observer_t*, uintptr_t id, vsync_observer_callback callback,
    void* user_data);
// To synchronize with a given VSync, a callback is set, which is normal
// callback and is called back when the next VSync signal arrives.
LYNX_CAPI_EXPORT void lynx_vsync_observer_request_animation_frame(
    lynx_vsync_observer_t*, uintptr_t id, vsync_observer_callback callback,
    void* user_data);
// To synchronize with a given VSync, a callback is set, which is called back
// when the next VSync signal arrives. This callback will be called after any
// normal callback.
LYNX_CAPI_EXPORT void
lynx_vsync_observer_register_after_animation_frame_listener(
    lynx_vsync_observer_t*, vsync_observer_callback callback, void* user_data);

typedef void (*lynx_extension_module_on_lynx_view_create_func)(
    lynx_extension_module_t*, lynx_view_t*);
typedef void (*lynx_extension_module_on_lynx_view_destroy_func)(
    lynx_extension_module_t*);
typedef void (*lynx_extension_module_on_runtime_init_func)(
    lynx_extension_module_t*);
typedef void (*lynx_extension_module_on_runtime_attach_func)(
    lynx_extension_module_t*, napi_env, lynx_vsync_observer_t*);
typedef void (*lynx_extension_module_on_runtime_ready_func)(
    lynx_extension_module_t*, napi_env, napi_value, const char* url);
typedef void (*lynx_extension_module_on_runtime_detach_func)(
    lynx_extension_module_t*);
typedef void (*lynx_extension_module_on_enter_foreground_func)(
    lynx_extension_module_t*);
typedef void (*lynx_extension_module_on_enter_background_func)(
    lynx_extension_module_t*);
typedef void (*lynx_extension_module_on_destroy_func)(lynx_extension_module_t*);
typedef void (*lynx_extension_module_post_task_func)(void* user_data);

// Creates a lynx_extension_module_t instance. It associates the provided
// user data with the module, which can be retrieved later using
// `lynx_extension_module_get_user_data`.
LYNX_CAPI_EXPORT lynx_extension_module_t* lynx_extension_module_create(
    void* user_data);

// Creates a lynx_extension_module_t instance with a finalizer. It associates
// the provided user data with the module, which can be retrieved later using
// `lynx_extension_module_get_user_data`. The finalizer function will be called
// when the module is released.
LYNX_CAPI_EXPORT lynx_extension_module_t*
lynx_extension_module_create_with_finalizer(
    void* user_data, void (*finalizer)(lynx_extension_module_t*, void*));

LYNX_CAPI_EXPORT void* lynx_extension_module_get_user_data(
    lynx_extension_module_t*);
// Binds a lynx_view_create state function to lynx_extension_module_t. This
// function will be invoked on lynx_view_t create. It is always called on the
// main thread.
LYNX_CAPI_EXPORT void lynx_extension_module_bind_lynx_view_create(
    lynx_extension_module_t*,
    lynx_extension_module_on_lynx_view_create_func func);
// Binds a lynx_view_destroy state function to lynx_extension_module_t. This
// function will be invoked on lynx_view_t destroy. It is always called on the
// main thread.
LYNX_CAPI_EXPORT void lynx_extension_module_bind_lynx_view_destroy(
    lynx_extension_module_t*,
    lynx_extension_module_on_lynx_view_destroy_func func);
// Binds a runtime_init state function to lynx_extension_module_t. This
// function will be invoked on runtime init. It is always called on the main
// thread.
LYNX_CAPI_EXPORT void lynx_extension_module_bind_runtime_init(
    lynx_extension_module_t*, lynx_extension_module_on_runtime_init_func func);
// Binds a runtime_attach state function to lynx_extension_module_t. This
// function will be invoked on runtime attached. It is always called on the BTS
// thread.
LYNX_CAPI_EXPORT void lynx_extension_module_bind_runtime_attach(
    lynx_extension_module_t*,
    lynx_extension_module_on_runtime_attach_func func);
// Binds a runtime_ready state function to lynx_extension_module_t. This
// function will be invoked on runtime ready. It is always called on the BTS
// thread.
LYNX_CAPI_EXPORT void lynx_extension_module_bind_runtime_ready(
    lynx_extension_module_t*, lynx_extension_module_on_runtime_ready_func func);
// Binds a runtime_detach state function to lynx_extension_module_t. This
// function will be invoked on runtime detached. It is always called on the BTS
// thread.
LYNX_CAPI_EXPORT void lynx_extension_module_bind_runtime_detach(
    lynx_extension_module_t*,
    lynx_extension_module_on_runtime_detach_func func);
// Binds application did enter foreground state function to
// lynx_extension_module_t.
LYNX_CAPI_EXPORT void lynx_extension_module_bind_enter_foreground(
    lynx_extension_module_t*,
    lynx_extension_module_on_enter_foreground_func func);
// Binds application did enter background state function to
// lynx_extension_module_t.
LYNX_CAPI_EXPORT void lynx_extension_module_bind_enter_background(
    lynx_extension_module_t*,
    lynx_extension_module_on_enter_background_func func);
// Binds on lynx_extension_module_t instance destroy state. It will be called
// before the lynx_extension_module_t is released.
LYNX_CAPI_EXPORT void lynx_extension_module_bind_on_destroy(
    lynx_extension_module_t*, lynx_extension_module_on_destroy_func func);
// Set a napi_module_creator into lynx_extension_module_t. napi_module_creator
// can be used to create modules that bind napi exports.
LYNX_CAPI_EXPORT void lynx_extension_module_set_napi_module_creator(
    lynx_extension_module_t*, napi_module_creator creator);
// Post task to Runtime thread.
LYNX_CAPI_EXPORT void lynx_extension_module_post_task_to_runtime(
    lynx_extension_module_t*, lynx_extension_module_post_task_func func,
    void* user_data);
// Is running tasks on current thread.
LYNX_CAPI_EXPORT bool lynx_extension_module_is_running_on_bts_thread(
    lynx_extension_module_t*);

LYNX_CAPI_EXPORT void lynx_extension_module_ref(lynx_extension_module_t*);
LYNX_CAPI_EXPORT void lynx_extension_module_unref(lynx_extension_module_t*);

LYNX_EXTERN_C_END

#ifdef USE_PRIMJS_NAPI
#include "third_party/napi/include/primjs_napi_undefs.h"
#endif

#endif  // PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_EXTENSION_MODULE_CAPI_H_
