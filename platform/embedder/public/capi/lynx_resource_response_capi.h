// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_RESOURCE_RESPONSE_CAPI_H_
#define PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_RESOURCE_RESPONSE_CAPI_H_

#include "lynx_export.h"
#include "lynx_template_bundle_capi.h"

LYNX_EXTERN_C_BEGIN

typedef struct lynx_resource_response_t lynx_resource_response_t;

// Sets the response code for a lynx resource response. The response code can be
// used to indicate the status of the resource fetch operation, such as success,
// failure, or a specific error condition.
LYNX_CAPI_EXPORT void lynx_resource_response_set_code(lynx_resource_response_t*,
                                                      int code);

// Sets the error message for a lynx resource response. The error message can be
// used to provide detailed information about the failure of the resource fetch
// operation when the response code indicates an error condition.
LYNX_CAPI_EXPORT void lynx_resource_response_set_error_message(
    lynx_resource_response_t*, const char* msg);

// Sets the data content for a lynx resource response. It also provides an
// option to specify a destructor function that will be called when the data is
// no longer needed, typically when the response object is released, and pass an
// opaque pointer for additional context.
LYNX_CAPI_EXPORT void lynx_resource_response_set_data(
    lynx_resource_response_t*, uint8_t* content, size_t length,
    void (*dtor)(uint8_t*, size_t, void*), void* opaque);

// Sets the template bundle for a Lynx resource response. This function is
// designed for use with `LynxTemplateResourceFetcher`. It allows you to
// associate a `lynx_template_bundle_t` object. The template bundle typically
// contains template - related resources that are fetched during the resource
// loading process.
LYNX_CAPI_EXPORT void lynx_resource_response_set_template_bundle(
    lynx_resource_response_t*, lynx_template_bundle_t*);

// Invokes the callback associated with a lynx resource response. This function
// triggers the callback functionality for the given `lynx_resource_response_t`
// object. It is typically used to notify the caller about the completion of the
// resource fetch operation, allowing the caller to handle the response
// accordingly, such as processing the data, checking the response code, or
// handling errors.
LYNX_CAPI_EXPORT void lynx_resource_response_callback(
    lynx_resource_response_t*);

LYNX_CAPI_EXPORT void lynx_resource_response_release(lynx_resource_response_t*);

LYNX_EXTERN_C_END

#endif  // PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_RESOURCE_RESPONSE_CAPI_H_
