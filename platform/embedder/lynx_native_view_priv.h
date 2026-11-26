// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_EMBEDDER_LYNX_NATIVE_VIEW_PRIV_H_
#define PLATFORM_EMBEDDER_LYNX_NATIVE_VIEW_PRIV_H_

#include "clay/public/clay.h"
#include "platform/embedder/public/capi/lynx_native_view_capi.h"

LYNX_EXTERN_C_BEGIN

// The native view owns the returned sink.
LYNX_CAPI_EXPORT ClaySharedImageSinkRef
lynx_native_view_get_shared_image_sink(lynx_native_view_t* view);

LYNX_EXTERN_C_END

#endif  // PLATFORM_EMBEDDER_LYNX_NATIVE_VIEW_PRIV_H_
