// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_PUBLIC_LYNX_RUNTIME_LIFECYCLE_C_API_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_PUBLIC_LYNX_RUNTIME_LIFECYCLE_C_API_H_

#include <stdint.h>

#if defined(_WIN32) && defined(LYNX_RUNTIME_C_API_IMPLEMENTATION)
#define LYNX_RUNTIME_C_API __declspec(dllexport)
#elif defined(_WIN32)
#define LYNX_RUNTIME_C_API __declspec(dllimport)
#else
#define LYNX_RUNTIME_C_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Initial ABI version of the Lynx runtime lifecycle C API. Bump whenever the
// struct layout below changes. Third-party callers should assert the value
// they were compiled against before registering a listener.
#define LYNX_RUNTIME_LIFECYCLE_C_API_VERSION 1

// Listener struct passed to LynxRuntimeWrapperAddLifecycleListener. The struct
// is versioned by size: callers must set size to sizeof(the version they were
// built against). All callbacks are invoked on the JS thread. The struct
// itself is owned by the caller and must outlive the underlying
// LynxRuntimeWrapper or be unregistered before it is destroyed.
//
// user_data is an opaque pointer supplied by the caller and passed back
// verbatim to each callback. Lynx does not read or free it.
typedef struct {
  int32_t size;
  void* user_data;
  void (*on_runtime_attach)(void* user_data, void* napi_env,
                            const char* runtime_type, void* platform_loop);
  void (*on_runtime_detach)(void* user_data);
} LynxRuntimeLifecycleListener;

// Registers a lifecycle listener on the given LynxRuntimeWrapper.
//
// wrapper is the opaque pointer value obtained from
// LynxBackgroundRuntime.getNativePtr() on the ETS side. listener is borrowed
// for the lifetime of the registration; the caller keeps ownership and must
// keep the struct (and its user_data) alive until either the underlying
// LynxRuntimeWrapper is destroyed or listener is explicitly unregistered.
//
// The call is a no-op when wrapper is 0, listener is NULL, or listener->size
// does not match a known ABI version.
LYNX_RUNTIME_C_API void LynxRuntimeWrapperAddLifecycleListener(
    uint64_t wrapper, LynxRuntimeLifecycleListener* listener);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_PUBLIC_LYNX_RUNTIME_LIFECYCLE_C_API_H_
