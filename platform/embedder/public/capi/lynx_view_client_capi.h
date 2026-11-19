// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_VIEW_CLIENT_CAPI_H_
#define PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_VIEW_CLIENT_CAPI_H_

#include <stddef.h>
#include <stdint.h>

#include "lynx_export.h"

LYNX_EXTERN_C_BEGIN

// The LynxViewClient implemented by the client and registered to the LynxView
// instance is used to obtain the callbacks of each process in the LynxView
// lifecycle.
typedef struct lynx_view_client_t lynx_view_client_t;

// Callbacks of LynxViewClient
// Page starts preparing to load.
typedef void (*on_page_start)(lynx_view_client_t* client, const char* url);
typedef enum {
  kLynxPipelineOriginFirstScreen = 1,
  kLynxPipelineOriginReload = 2,
} lynx_pipeline_origin_t;
// The URL is borrowed for the duration of the callback.
typedef void (*on_page_start_v2)(lynx_view_client_t* client, const char* url,
                                 lynx_pipeline_origin_t origin);
// Page loaded successfully.
typedef void (*on_load_success)(lynx_view_client_t* client);
// First screen layout completed.
typedef void (*on_first_screen)(lynx_view_client_t* client);
// Page update callback.
typedef void (*on_page_updated)(lynx_view_client_t* client);
// Data update callback.
typedef void (*on_data_updated)(lynx_view_client_t* client);
// Callback after the page is destroyed.
typedef void (*on_destroy)(lynx_view_client_t* client);
// JS environment preparation completed.
// Note: The callback is in an asynchronous thread.
typedef void (*on_runtime_ready)(lynx_view_client_t* client);
// Error received.
// Do not release the view or client inside this callback. Schedule cleanup on
// the host UI thread after the callback and its enclosing API call return.
typedef void (*on_received_error)(lynx_view_client_t* client, int error_code,
                                  const char* message);
struct lynx_value;
// Read-only error snapshot borrowed during the callback. String sizes are in
// bytes. error_code is the category code; sub_code preserves the original code.
// Only codes recognized by the linked SDK metadata are categorized. level and
// fix_suggestion contain the metadata defaults or, when unspecified, the values
// at the call site. level: 0 fatal, 1 error, 2 warn, 3 undecided; other values
// are unknown. type is the error source (-1 for native delivery). custom_info
// is a lynx_value string dictionary. consumers contain integer enum values: 0
// front-end, 1 client, 2 lynx; unknown codes have no consumers. Check
// struct_size before reading fields. Keep existing field offsets and append
// future fields only at the end.
typedef struct lynx_error_t {
  size_t struct_size;
  int32_t error_code;
  int32_t sub_code;
  int32_t level;
  int32_t type;
  const char* summary_message;
  size_t summary_size;
  const char* fix_suggestion;
  size_t suggestion_size;
  const char* root_cause;
  size_t cause_size;
  const struct lynx_value* custom_info;
  const int32_t* consumers;
  size_t consumer_count;
} lynx_error_t;

// Business errors, excluding LogBox-only notifications.
// Do not release the view or client inside this callback. Schedule cleanup on
// the host UI thread after the callback and its enclosing API call return.
// Platforms assemble diagnostic text and inject their current template URL.
// All pointers, including strings and consumers, expire when the callback
// returns. Copy their contents before returning. custom_info can instead be
// retained with lynx_value_add_reference. Borrowed data must not be modified.
typedef void (*on_received_error_v2)(lynx_view_client_t* client,
                                     const lynx_error_t* error);
// Native module method invocation completed.
typedef void (*on_module_method_invoked)(lynx_view_client_t* client,
                                         const char* module, const char* method,
                                         int error_code);
typedef void (*on_timing_setup)(lynx_view_client_t* client,
                                const char* timing_info);
typedef void (*on_timing_update)(lynx_view_client_t* client,
                                 const char* timing_info,
                                 const char* update_timing,
                                 const char* update_flag);
typedef void (*on_enter_foreground)(lynx_view_client_t* client);
typedef void (*on_enter_background)(lynx_view_client_t* client);
typedef void (*on_frame_timing)(lynx_view_client_t* client,
                                int64_t frame_start_time_in_ns,
                                int64_t frame_finish_time_in_ns);
// Harmony intrinsic content dimensions in logical pixels, on the UI thread.
typedef void (*on_intrinsic_content_size_changed)(lynx_view_client_t* client,
                                                  float width, float height);

LYNX_CAPI_EXPORT lynx_view_client_t* lynx_view_client_create(void* user_data);
LYNX_CAPI_EXPORT void* lynx_view_client_get_user_data(lynx_view_client_t*);
LYNX_CAPI_EXPORT void lynx_view_client_bind_on_page_start(lynx_view_client_t*,
                                                          on_page_start f);
// Receives both template loads and in-place reloads. When both callbacks are
// bound on one client, only V2 is used; binding NULL restores legacy delivery.
LYNX_CAPI_EXPORT void lynx_view_client_bind_on_page_start_v2(
    lynx_view_client_t*, on_page_start_v2 f);
LYNX_CAPI_EXPORT void lynx_view_client_bind_on_load_success(lynx_view_client_t*,
                                                            on_load_success f);
LYNX_CAPI_EXPORT void lynx_view_client_bind_on_first_screen(lynx_view_client_t*,
                                                            on_first_screen f);
LYNX_CAPI_EXPORT void lynx_view_client_bind_on_page_updated(lynx_view_client_t*,
                                                            on_page_updated f);
LYNX_CAPI_EXPORT void lynx_view_client_bind_on_data_updated(lynx_view_client_t*,
                                                            on_data_updated f);
LYNX_CAPI_EXPORT void lynx_view_client_bind_on_destroy(lynx_view_client_t*,
                                                       on_destroy f);
LYNX_CAPI_EXPORT void lynx_view_client_bind_on_runtime_ready(
    lynx_view_client_t*, on_runtime_ready f);
LYNX_CAPI_EXPORT void lynx_view_client_bind_on_received_error(
    lynx_view_client_t*, on_received_error f);
// When both callbacks are bound, only v2 is used. Binding NULL restores legacy
// delivery, including its raw code, summary and LogBox-only notifications.
LYNX_CAPI_EXPORT void lynx_view_client_bind_on_received_error_v2(
    lynx_view_client_t*, on_received_error_v2 f);
LYNX_CAPI_EXPORT void lynx_view_client_bind_on_module_method_invoked(
    lynx_view_client_t*, on_module_method_invoked f);
LYNX_CAPI_EXPORT void lynx_view_client_bind_on_timing_setup(lynx_view_client_t*,
                                                            on_timing_setup f);
LYNX_CAPI_EXPORT void lynx_view_client_bind_on_timing_update(
    lynx_view_client_t*, on_timing_update f);
LYNX_CAPI_EXPORT void lynx_view_client_bind_on_enter_foreground(
    lynx_view_client_t*, on_enter_foreground f);
LYNX_CAPI_EXPORT void lynx_view_client_bind_on_enter_background(
    lynx_view_client_t*, on_enter_background f);
LYNX_CAPI_EXPORT void lynx_view_client_bind_on_frame_timing(lynx_view_client_t*,
                                                            on_frame_timing f);
LYNX_CAPI_EXPORT void lynx_view_client_release(lynx_view_client_t*);
LYNX_CAPI_EXPORT void lynx_view_client_bind_on_intrinsic_content_size_changed(
    lynx_view_client_t*, on_intrinsic_content_size_changed f);

LYNX_EXTERN_C_END

#endif  // PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_VIEW_CLIENT_CAPI_H_
