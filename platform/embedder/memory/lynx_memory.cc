// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <stdlib.h>
#include <string.h>

#include "platform/embedder/public/capi/lynx_memory_capi.h"

LYNX_EXTERN_C void* lynx_malloc(size_t size) { return malloc(size); }

LYNX_EXTERN_C void lynx_free(void* ptr) { free(ptr); }

LYNX_EXTERN_C char* lynx_strdup(const char* s) {
  if (!s) {
    return nullptr;
  }
  size_t len = strlen(s) + 1;
  void* new_mem = lynx_malloc(len);
  if (!new_mem) {
    return nullptr;
  }
  return static_cast<char*>(memcpy(new_mem, s, len));
}
