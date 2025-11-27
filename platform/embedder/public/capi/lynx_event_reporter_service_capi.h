// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_EVENT_REPORTER_SERVICE_CAPI_H_
#define PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_EVENT_REPORTER_SERVICE_CAPI_H_

#include <stdint.h>
#include <stdlib.h>

#include "base/include/value/lynx_value_types.h"
#include "lynx_export.h"

LYNX_EXTERN_C_BEGIN

// Forward declaration of the lynx_event_reporter_service_t struct.
typedef struct lynx_event_reporter_service_t lynx_event_reporter_service_t;

// Define a function pointer type for event reporter handling.
typedef void (*lynx_event_report_func)(lynx_event_reporter_service_t*,
                                       const char* event_name,
                                       const lynx_value& params);

// Creates a new event reporter service instance.
LYNX_CAPI_EXPORT lynx_event_reporter_service_t*
lynx_event_reporter_service_create(void* user_data);

// Create a new event reporter service instance with a finalizer.
// The finalizer will be called when the service is released.
// The instance's lifecycle is based on reference count.
LYNX_CAPI_EXPORT lynx_event_reporter_service_t*
lynx_event_reporter_service_create_with_finalizer(
    void* user_data, void (*finalizer)(lynx_event_reporter_service_t*, void*));

// Binds a report function to the event reporter service.
LYNX_CAPI_EXPORT void lynx_event_reporter_service_bind(
    lynx_event_reporter_service_t*, lynx_event_report_func f);

// Retrieves the user data associated with the event reporter service.
LYNX_CAPI_EXPORT void* lynx_event_reporter_service_get_user_data(
    lynx_event_reporter_service_t* event_report_service);

// Releases the resources associated with the event reporter service.
LYNX_CAPI_EXPORT void lynx_event_reporter_service_release(
    lynx_event_reporter_service_t*);

// Call the event reporter handler associated with the event reporter service.
LYNX_CAPI_EXPORT int lynx_event_reporter_service_on_event(
    lynx_event_reporter_service_t* event_report_service, const char* event_name,
    const lynx_value& params);

LYNX_EXTERN_C_END

#endif  // PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_EVENT_REPORTER_SERVICE_CAPI_H_
