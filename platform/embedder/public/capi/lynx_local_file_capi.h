// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_LOCAL_FILE_CAPI_H_
#define PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_LOCAL_FILE_CAPI_H_

#include <stddef.h>
#include <stdint.h>

#include "lynx_export.h"

LYNX_EXTERN_C_BEGIN

// Reads a local file URL. The returned data must be released with lynx_free().
LYNX_CAPI_EXPORT uint8_t* lynx_read_local_file(const char* file_url,
                                               size_t* size);

LYNX_EXTERN_C_END

#endif  // PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_LOCAL_FILE_CAPI_H_
