// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_REPLAY_CAPI_H_
#define PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_REPLAY_CAPI_H_

#include <stdbool.h>
#include <stddef.h>

#include "lynx_export.h"

LYNX_EXTERN_C_BEGIN

// Evaluates an already extracted Fixture at a UTF-8 directory path on the
// calling thread. Available on macOS and Windows builds. The callback runs
// synchronously, exactly once, with a JSON record on success or an error
// message on failure. Its bytes are valid only during the callback. The
// callback must be non-null. Returns false when replay is disabled or
// evaluation fails. This does not create the persistent module replay context.
LYNX_CAPI_EXPORT bool lynx_replay_evaluate_fixture(
    const char* directory, void* context,
    void (*callback)(void* context, const char* bytes, size_t length));

LYNX_EXTERN_C_END

#endif  // PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_REPLAY_CAPI_H_
