// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_TEMPLATE_BUNDLE_CAPI_H_
#define PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_TEMPLATE_BUNDLE_CAPI_H_

#include <stddef.h>
#include <stdint.h>

#include "lynx_export.h"

LYNX_EXTERN_C_BEGIN

typedef struct lynx_template_bundle_t lynx_template_bundle_t;

// Create a template bundle from the given content. You can also
// provide a custom destructor function to handle the cleanup of the content
// memory, and pass an opaque pointer for additional context.
LYNX_CAPI_EXPORT lynx_template_bundle_t* lynx_template_bundle_create(
    uint8_t* content, size_t length, void (*dtor)(uint8_t*, size_t, void*),
    void* opaque);

// Check if the template bundle is valid. If not, the error message can be
// obtained by calling lynx_template_bundle_get_error_message.
// Return 1 if valid, 0 otherwise.
LYNX_CAPI_EXPORT int lynx_template_bundle_is_valid(lynx_template_bundle_t*);

// Get the error message if the template bundle is not valid.
LYNX_CAPI_EXPORT const char* lynx_template_bundle_get_error_message(
    lynx_template_bundle_t*);

// Release the template bundle. This will be called automatically if
// lynx_view_load_template is called, otherwise the caller is responsible for
// calling this function to release the bundle.
LYNX_CAPI_EXPORT void lynx_template_bundle_release(lynx_template_bundle_t*);

LYNX_EXTERN_C_END

#endif  // PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_TEMPLATE_BUNDLE_CAPI_H_
