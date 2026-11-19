// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_CAPI_MODULE_CAPI_H_
#define PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_CAPI_MODULE_CAPI_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "base/include/value/lynx_value_api.h"
#include "lynx_export.h"

LYNX_EXTERN_C_BEGIN

typedef struct lynx_view_builder_t lynx_view_builder_t;
typedef struct lynx_native_module_callback_t lynx_native_module_callback_t;

typedef struct lynx_capi_module_method_t {
  const char* name;
  size_t argument_count;
} lynx_capi_module_method_t;

typedef struct lynx_capi_module_callback_t {
  size_t argument_index;
  // Borrowed for the duration of the module invocation. Retain the handle
  // before returning when the callback will be invoked asynchronously.
  lynx_native_module_callback_t* callback;
} lynx_capi_module_callback_t;

enum {
  LYNX_CAPI_MODULE_SUCCESS = 0,
  LYNX_CAPI_MODULE_ERROR = 1,
};

// Arguments and callbacks are borrowed and remain valid only during this call.
// The host must eventually invoke every supplied callback during the module's
// lifetime, including on error or cancellation, using the method's defined
// completion result. Uninvoked script closures remain until the runtime is
// destroyed.
//
// Script arguments use the standard Lynx value conversion, which may update
// shared native containers. Cyclic script arguments are not supported.
// Values use the ordinary lynx_value representation, including nested values;
// runtime-backed extended values and functions are not supported.
// Containers must be acyclic.
// result is initialized to undefined. Writing a pointer-backed value into it
// transfers one reference to Lynx, including when the invocation returns error.
// Calls run on the calling runtime's thread. A module shared by MTS and BTS
// must synchronize access to its user data. C callbacks must not throw.
typedef int32_t (*lynx_capi_module_invoke)(
    void* opaque, const char* method_name, const lynx_value* arguments,
    size_t argument_count, const lynx_capi_module_callback_t* callbacks,
    size_t callback_count, lynx_value* result);

typedef void (*lynx_capi_module_finalizer)(void* opaque);

// The builder copies the module name and method metadata during registration.
// Its CAPI registrations are transferred to the next view created from it.
// CAPI modules take precedence over same-name native and extension NAPI modules
// on both MTS and BTS.
// Method names must be unique and argument_count is the required argument
// count. Returns SUCCESS on registration and ERROR for invalid input.
// If finalizer is non-null, the registration owns opaque on success and calls
// finalizer once, outside registry locks, after the builder or view releases
// the registration and all in-flight invocations finish. It may run on any
// thread. If finalizer is null, the caller owns opaque and must keep
// it alive until all invocations finish, including those still running when
// the view is released. On failure opaque remains owned by the caller and
// finalizer is not called.
LYNX_CAPI_EXPORT int32_t lynx_view_builder_register_capi_module(
    lynx_view_builder_t* builder, const char* name,
    const lynx_capi_module_method_t* methods, size_t method_count,
    lynx_capi_module_invoke invoke, void* opaque,
    lynx_capi_module_finalizer finalizer);

LYNX_CAPI_EXPORT void lynx_native_module_callback_retain(
    lynx_native_module_callback_t* callback);
// Releases the native handle only; this does not invoke the callback or remove
// its retained script closure.
LYNX_CAPI_EXPORT void lynx_native_module_callback_release(
    lynx_native_module_callback_t* callback);

// Invokes a callback at most once. Arguments are borrowed for this call.
// Arguments follow the same value restrictions as module results. Lynx retains
// their references before returning; container and buffer contents are shared,
// not cloned. Do not mutate them concurrently with callback execution.
// Retained handles may be used from any thread. Pending callbacks are cancelled
// when their module or view is destroyed. Handles must still be released by
// their owner. A module invocation returning ERROR does not cancel callbacks.
// Returns 1 when the callback was accepted, otherwise 0.
LYNX_CAPI_EXPORT int lynx_native_module_callback_invoke(
    lynx_native_module_callback_t* callback, const lynx_value* arguments,
    size_t argument_count);

LYNX_EXTERN_C_END

#endif  // PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_CAPI_MODULE_CAPI_H_
