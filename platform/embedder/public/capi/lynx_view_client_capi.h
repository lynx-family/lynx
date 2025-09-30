// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_VIEW_CLIENT_CAPI_H_
#define PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_VIEW_CLIENT_CAPI_H_

#include "lynx_export.h"

LYNX_EXTERN_C_BEGIN

// The LynxViewClient implemented by the client and registered to the LynxView
// instance is used to obtain the callbacks of each process in the LynxView
// lifecycle.
typedef struct lynx_view_client_t lynx_view_client_t;

// Callbacks of LynxViewClient
// Page starts preparing to load.
typedef void (*on_page_start)(lynx_view_client_t* client, const char* url);
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
typedef void (*on_received_error)(lynx_view_client_t* client, int error_code,
                                  const char* message);
typedef void (*on_timing_setup)(lynx_view_client_t* client,
                                const char* timing_info);
typedef void (*on_timing_update)(lynx_view_client_t* client,
                                 const char* timing_info,
                                 const char* update_timing,
                                 const char* update_flag);
typedef void (*on_enter_foreground)(lynx_view_client_t* client);
typedef void (*on_enter_background)(lynx_view_client_t* client);

LYNX_CAPI_EXPORT lynx_view_client_t* lynx_view_client_create(void* user_data);
LYNX_CAPI_EXPORT void* lynx_view_client_get_user_data(lynx_view_client_t*);
LYNX_CAPI_EXPORT void lynx_view_client_bind_on_page_start(lynx_view_client_t*,
                                                          on_page_start f);
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
LYNX_CAPI_EXPORT void lynx_view_client_bind_on_timing_setup(lynx_view_client_t*,
                                                            on_timing_setup f);
LYNX_CAPI_EXPORT void lynx_view_client_bind_on_timing_update(
    lynx_view_client_t*, on_timing_update f);
LYNX_CAPI_EXPORT void lynx_view_client_bind_on_enter_foreground(
    lynx_view_client_t*, on_enter_foreground f);
LYNX_CAPI_EXPORT void lynx_view_client_bind_on_enter_background(
    lynx_view_client_t*, on_enter_background f);
LYNX_CAPI_EXPORT void lynx_view_client_release(lynx_view_client_t*);

LYNX_EXTERN_C_END

#endif  // PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_VIEW_CLIENT_CAPI_H_
