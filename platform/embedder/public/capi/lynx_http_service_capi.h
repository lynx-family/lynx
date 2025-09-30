// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_HTTP_SERVICE_CAPI_H_
#define PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_HTTP_SERVICE_CAPI_H_

#include <stdint.h>
#include <stdlib.h>

#include "lynx_export.h"

LYNX_EXTERN_C_BEGIN

// Forward declaration of the lynx_http_request_t struct.
typedef struct lynx_http_request_t lynx_http_request_t;
// Retrieves the URL associated with the HTTP request. This function extracts
// and returns the URL of the specified HTTP request. The caller should not free
// the returned pointer.
LYNX_CAPI_EXPORT const char* lynx_http_request_get_url(lynx_http_request_t*);
// Retrieves the HTTP method of the specified request. This function extracts
// and returns the HTTP method (such as "GET", "POST", "PUT", etc.) associated
// with the given HTTP request. The caller should not free the returned pointer.
LYNX_CAPI_EXPORT const char* lynx_http_request_get_method(lynx_http_request_t*);
// Retrieves the HTTP method of the specified request. This function extracts
// and returns the HTTP method (such as "GET", "POST", "PUT", etc.) associated
// with the given HTTP request. The caller should not free the returned pointer.
LYNX_CAPI_EXPORT size_t lynx_http_request_get_body(lynx_http_request_t*,
                                                   const uint8_t** data);
// Retrieves the number of headers in the specified HTTP request. This function
// counts and returns the number of headers associated with the given HTTP
// request. The returned value can be used to iterate over all headers using
// `lynx_http_request_get_header`.
LYNX_CAPI_EXPORT size_t
lynx_http_request_get_header_count(lynx_http_request_t*);
// Retrieves a header from the specified HTTP request by index. This function
// fetches the header key and value at the given index from the HTTP request.
// The index should be in the range `[0,
// lynx_http_request_get_header_count(request))`. The caller should not free the
// returned pointers, as their lifetimes are managed by the
// `lynx_http_request_t` object.
LYNX_CAPI_EXPORT void lynx_http_request_get_header(lynx_http_request_t*,
                                                   uint32_t index,
                                                   const char** key,
                                                   const char** value);
// Releases the resources associated with an HTTP request.
LYNX_CAPI_EXPORT void lynx_http_request_release(lynx_http_request_t*);

// Forward declaration of the lynx_http_response_t struct.
typedef struct lynx_http_response_t lynx_http_response_t;
// Sets the URL of an HTTP response. This function allows you to specify the URL
// of the given HTTP response object.
LYNX_CAPI_EXPORT void lynx_http_response_set_url(lynx_http_response_t*,
                                                 const char* url);
// Sets the status code of an HTTP response. This function allows you to specify
// the status code of the given HTTP response object. Status codes are used to
// indicate the outcome of an HTTP request, such as 200 for success, 404 for not
// found, etc.
LYNX_CAPI_EXPORT void lynx_http_response_set_status_code(lynx_http_response_t*,
                                                         int status_code);
// Sets the status text of an HTTP response. This function allows you to specify
// the status text associated with the HTTP response. The status text provides a
// human - readable description corresponding to the status code. For example,
// "OK" for status code 200, "Not Found" for status code 404. The caller should
// not free the memory of the passed `status_text` pointer.
LYNX_CAPI_EXPORT void lynx_http_response_set_status_text(
    lynx_http_response_t*, const char* status_text);
// Adds a header to the specified HTTP response. This function allows you to add
// a new header to the given HTTP response object. Headers are used to provide
// additional information about the response, such as content type, caching
// directives, etc.
LYNX_CAPI_EXPORT void lynx_http_response_add_header(lynx_http_response_t*,
                                                    const char* key,
                                                    const char* value);
// Sets the body content of an HTTP response. This function allows you to
// specify the body content of the given HTTP response object. You can also
// provide a custom destructor function to handle the cleanup of the content
// memory, and pass an opaque pointer for additional context.
LYNX_CAPI_EXPORT void lynx_http_response_set_body(
    lynx_http_response_t*, uint8_t* content, size_t length,
    void (*dtor)(uint8_t*, size_t, void*), void* opaque);
// Triggers the callback associated with an HTTP response. This function invokes
// the callback that is related to the specified HTTP response object. This
// function must be called when the response is ready or failed.
LYNX_CAPI_EXPORT void lynx_http_response_callback(lynx_http_response_t*);
// Releases the resources associated with an HTTP response.
LYNX_CAPI_EXPORT void lynx_http_response_release(lynx_http_response_t*);

typedef struct lynx_http_service_t lynx_http_service_t;
// Define a function pointer type for HTTP request handling.
typedef void (*lynx_http_request_func)(lynx_http_service_t*,
                                       lynx_http_request_t* request,
                                       lynx_http_response_t* response);
// Creates a new HTTP service instance.
LYNX_CAPI_EXPORT lynx_http_service_t* lynx_http_service_create(void* user_data);
// Create a new HTTP service instance with a finalizer.
// The finalizer will be called when the service is released.
// The instance's lifecycle is based on reference count.
LYNX_CAPI_EXPORT lynx_http_service_t* lynx_http_service_create_with_finalizer(
    void* user_data, void (*finalizer)(lynx_http_service_t*, void*));
// Retrieves the user data associated with an HTTP service.
LYNX_CAPI_EXPORT void* lynx_http_service_get_user_data(lynx_http_service_t*);
// Binds an HTTP request handling function to an HTTP service. This function
// associates the specified HTTP request handling function with the given HTTP
// service. Whenever an HTTP request is received by the service, the bound
// function will be invoked to process the request.
LYNX_CAPI_EXPORT void lynx_http_service_bind(lynx_http_service_t*,
                                             lynx_http_request_func f);
// Releases the resources associated with an HTTP service.
LYNX_CAPI_EXPORT void lynx_http_service_release(lynx_http_service_t*);

LYNX_EXTERN_C_END

#endif  // PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_HTTP_SERVICE_CAPI_H_
