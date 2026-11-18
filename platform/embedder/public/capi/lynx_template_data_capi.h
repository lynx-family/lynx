// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_TEMPLATE_DATA_CAPI_H_
#define PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_TEMPLATE_DATA_CAPI_H_

#include "base/include/value/lynx_value_types.h"
#include "lynx_export.h"

LYNX_EXTERN_C_BEGIN

typedef struct lynx_template_data_t lynx_template_data_t;

// Create a template data from json string.
LYNX_CAPI_EXPORT lynx_template_data_t* lynx_template_data_create_from_json(
    const char* json);

// Create template data by retaining a portable lynx_value. The caller keeps
// ownership of value and may release it after this call. Runtime-backed
// extended values are not supported.
LYNX_CAPI_EXPORT lynx_template_data_t* lynx_template_data_create_from_value(
    lynx_value value);

// Mark the name of data processor. The data processor will be called when
// lynx_view_update_data is called.
LYNX_CAPI_EXPORT void lynx_template_data_mark_state(lynx_template_data_t*,
                                                    const char* name);

// Sets the read-only flag for a lynx template data object.
LYNX_CAPI_EXPORT void lynx_template_data_set_read_only(lynx_template_data_t*,
                                                       int read_only);

// Release the caller's data handle. Load/update metadata retain their own
// shared ownership; loading or updating does not consume this handle.
LYNX_CAPI_EXPORT void lynx_template_data_release(lynx_template_data_t*);

LYNX_EXTERN_C_END

#endif  // PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_TEMPLATE_DATA_CAPI_H_
