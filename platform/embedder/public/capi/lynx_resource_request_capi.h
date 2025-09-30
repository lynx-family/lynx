// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_RESOURCE_REQUEST_CAPI_H_
#define PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_RESOURCE_REQUEST_CAPI_H_

#include <stdint.h>

#include "lynx_export.h"

LYNX_EXTERN_C_BEGIN

typedef struct lynx_resource_request_t lynx_resource_request_t;

// Enumeration defining different types of resource requests.
// Each enumerator represents a specific type of resource that can be requested.
typedef enum lynx_resource_type_e {
  kLynxResourceTypeGeneric = 0,
  kLynxResourceTypeImage,
  kLynxResourceTypeFont,
  kLynxResourceTypeLottie,
  kLynxResourceTypeVideo,
  kLynxResourceTypeSVG,
  kLynxResourceTypeTemplate,
  kLynxResourceTypeLynxCoreJS,
  kLynxResourceTypeLazyBundle,
  kLynxResourceTypeI18NText,
  kLynxResourceTypeTheme,
  kLynxResourceTypeExternalJSSource,
  kLynxResourceTypeExternalByteCode,
} lynx_resource_type_e;

typedef uint64_t lynx_resource_request_id;
#define kLynxResourceRequestIdInvalid 0

// This function fetches the unique identifier associated with the specified
// `lynx_resource_request_t` object. Each resource request is assigned a
// distinct identifier upon creation, which can be used to track and manage the
// request throughout its lifecycle, such as canceling it later.
LYNX_CAPI_EXPORT lynx_resource_request_id
lynx_resource_request_get_id(lynx_resource_request_t*);

// Retrieves the type of a resource request. This function determines and
// returns the type of the resource request represented by the provided
// `lynx_resource_request_t` object. The type is identified using the
// `lynx_resource_type_e` enumeration, where each enumerator corresponds to a
// specific type of resource that can be requested.
LYNX_CAPI_EXPORT lynx_resource_type_e
lynx_resource_request_get_type(lynx_resource_request_t*);

// Retrieves the URL of the resource request. This function returns a pointer to
// a null-terminated string representing the URL of the given resource request.
// The returned string is owned by the `lynx_resource_request_t` object and
// should not be modified or freed by the caller.
LYNX_CAPI_EXPORT const char* lynx_resource_request_get_url(
    lynx_resource_request_t*);

LYNX_CAPI_EXPORT void lynx_resource_request_release(lynx_resource_request_t*);

LYNX_EXTERN_C_END

#endif  // PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_RESOURCE_REQUEST_CAPI_H_
