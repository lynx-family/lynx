// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_MEMORY_CAPI_H_
#define PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_MEMORY_CAPI_H_

#include <stddef.h>

#include "lynx_export.h"

LYNX_EXTERN_C_BEGIN

// Allocates memory of `size` bytes. The memory is not initialized.
// The returned pointer must be deallocated with `lynx_free()`.
LYNX_CAPI_EXPORT void* lynx_malloc(size_t size);

// Deallocates the memory previously allocated by `lynx_malloc()` or
// `lynx_strdup()`. If `ptr` is a null pointer, no operation is performed.
LYNX_CAPI_EXPORT void lynx_free(void* ptr);

// Duplicates the string `s`. Memory for the new string is obtained with
// `lynx_malloc()`, and can be freed with `lynx_free()`.
LYNX_CAPI_EXPORT char* lynx_strdup(const char* s);

LYNX_EXTERN_C_END

#endif  // PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_MEMORY_CAPI_H_
