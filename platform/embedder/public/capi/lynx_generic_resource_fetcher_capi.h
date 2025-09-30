// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_GENERIC_RESOURCE_FETCHER_CAPI_H_
#define PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_GENERIC_RESOURCE_FETCHER_CAPI_H_

#include "lynx_export.h"
#include "lynx_resource_request_capi.h"
#include "lynx_resource_response_capi.h"

LYNX_EXTERN_C_BEGIN

typedef struct lynx_generic_resource_fetcher_t lynx_generic_resource_fetcher_t;

// Function pointer type for the resource fetching callback. This type defines a
// callback function used to fetch resources. The callback takes a pointer to a
// generic resource fetcher, a resource request, and a resource response object.
// It is responsible for populating the response based on the request.
typedef void (*fetch_resource_func)(lynx_generic_resource_fetcher_t*,
                                    lynx_resource_request_t* request,
                                    lynx_resource_response_t* response);

// Function pointer type for the resource fetch cancellation callback. This type
// defines a callback function used to cancel an ongoing resource fetch request.
// The callback takes a pointer to a generic resource fetcher and the unique
// identifier of the resource request to be cancelled.
typedef void (*cancel_fetch_func)(lynx_generic_resource_fetcher_t*,
                                  lynx_resource_request_id request_id);

// Creates a new generic resource fetcher instance. This function allocates and
// initializes a new generic resource fetcher object. It associates the provided
// user data with the fetcher, which can be retrieved later using
// `lynx_generic_resource_fetcher_get_user_data`.
LYNX_CAPI_EXPORT lynx_generic_resource_fetcher_t*
lynx_generic_resource_fetcher_create(void* user_data);

// Creates a new generic resource fetcher instance with a finalizer. This
// function allocates and initializes a new generic resource fetcher object. It
// associates the provided user data with the fetcher and sets a finalizer
// function that will be called when the fetcher is released.
LYNX_CAPI_EXPORT lynx_generic_resource_fetcher_t*
lynx_generic_resource_fetcher_create_with_finalizer(
    void* user_data,
    void (*finalizer)(lynx_generic_resource_fetcher_t*, void*));

LYNX_CAPI_EXPORT void* lynx_generic_resource_fetcher_get_user_data(
    lynx_generic_resource_fetcher_t*);

// Binds a resource fetching callback to a generic resource fetcher. This
// function sets the resource fetching callback function for the given generic
// resource fetcher instance. The callback will be invoked when a resource needs
// to be fetched.
LYNX_CAPI_EXPORT void lynx_generic_resource_fetcher_bind_fetch_resource(
    lynx_generic_resource_fetcher_t*, fetch_resource_func f);

// Binds a resource path fetching callback to a generic resource fetcher. This
// function sets the resource path fetching callback function for the given
// generic resource fetcher instance. The callback will be invoked when the path
// of a resource needs to be fetched.
LYNX_CAPI_EXPORT void lynx_generic_resource_fetcher_bind_fetch_resource_path(
    lynx_generic_resource_fetcher_t*, fetch_resource_func f);

// Binds a resource fetch cancellation callback to a generic resource fetcher.
// This function sets the resource fetch cancellation callback function for the
// given generic resource fetcher instance. The callback will be invoked when a
// resource fetch request needs to be cancelled.
LYNX_CAPI_EXPORT void lynx_generic_resource_fetcher_bind_cancel_fetch(
    lynx_generic_resource_fetcher_t*, cancel_fetch_func f);

// Releases a generic resource fetcher instance. This function deallocates the
// memory used by the given generic resource fetcher instance and calls the
// finalizer function if one was set during creation. After calling this
// function, the provided pointer becomes invalid.
LYNX_CAPI_EXPORT void lynx_generic_resource_fetcher_release(
    lynx_generic_resource_fetcher_t*);

LYNX_EXTERN_C_END

#endif  // PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_GENERIC_RESOURCE_FETCHER_CAPI_H_
