// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_TEMPLATE_DATA_CAPI_H_
#define PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_TEMPLATE_DATA_CAPI_H_

#include "lynx_export.h"

LYNX_EXTERN_C_BEGIN

typedef struct lynx_template_data_t lynx_template_data_t;

// Create a template data from json string.
LYNX_CAPI_EXPORT lynx_template_data_t* lynx_template_data_create_from_json(
    const char* json);

// Mark the name of data processor. The data processor will be called when
// lynx_view_update_data is called.
LYNX_CAPI_EXPORT void lynx_template_data_mark_state(lynx_template_data_t*,
                                                    const char* name);

// Sets the read-only flag for a lynx template data object.
LYNX_CAPI_EXPORT void lynx_template_data_set_read_only(lynx_template_data_t*,
                                                       int read_only);

// Release the template data. This will be called automatically if
// lynx_view_load_template is called, otherwise the caller is responsible for
// calling this function to release the template data.
LYNX_CAPI_EXPORT void lynx_template_data_release(lynx_template_data_t*);

LYNX_EXTERN_C_END

#endif  // PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_TEMPLATE_DATA_CAPI_H_
