// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_GROUP_CAPI_H_
#define PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_GROUP_CAPI_H_

#include <stddef.h>

#include "lynx_export.h"

LYNX_EXTERN_C_BEGIN

typedef struct lynx_group_t lynx_group_t;

#define LYNX_SINGLE_GROUP "-1"

// Create a group with the given. A unique ID will be generated as group ID.
LYNX_CAPI_EXPORT lynx_group_t* lynx_group_create(const char* name);

// Create a group with the given name and ID. If the ID is not provided, a
// unique ID will be generated. The ID should ideally be set to
// `LYNX_SINGLE_GROUP` Other values are considered experimental features and
// should be used with caution.
LYNX_CAPI_EXPORT lynx_group_t* lynx_group_create_with_id(const char* name,
                                                         const char* id);

// Sets the preload JavaScript file paths for a lynx group. This function allows
// you to specify an array of JavaScript file paths that should be preloaded for
// a given lynx group.
LYNX_CAPI_EXPORT void lynx_group_set_preload_js_paths(lynx_group_t*,
                                                      const char* paths[],
                                                      size_t size);
// Enables or disables the JavaScript group thread for a lynx group.
LYNX_CAPI_EXPORT void lynx_group_set_enable_js_group_thread(lynx_group_t*,
                                                            int enable);

// Release the lynx group.
LYNX_CAPI_EXPORT void lynx_group_release(lynx_group_t*);

LYNX_EXTERN_C_END

#endif  // PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_GROUP_CAPI_H_
