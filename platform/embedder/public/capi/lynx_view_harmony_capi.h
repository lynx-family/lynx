// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_VIEW_HARMONY_CAPI_H_
#define PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_VIEW_HARMONY_CAPI_H_

#include "lynx_export.h"

LYNX_EXTERN_C_BEGIN

typedef struct lynx_view_builder_t lynx_view_builder_t;
typedef struct lynx_view_t lynx_view_t;

// Creates a builder for detached Harmony views. ark_ui_context is a borrowed
// ArkUI_ContextHandle and must outlive the builder and its views. Supply
// positive, finite viewport dimensions in logical pixels, or pass (0, 0) to
// defer sizing until lynx_view_set_viewport(). Screen dimensions and pixel
// ratio must be positive and finite. Returns NULL for invalid arguments.
// Create views with lynx_view_create(). Run all view and builder operations
// on the UI thread.
LYNX_CAPI_EXPORT lynx_view_builder_t* lynx_view_builder_create_harmony(
    void* ark_ui_context, float viewport_width, float viewport_height,
    float screen_width, float screen_height, float pixel_ratio);

// Attaches or moves the root to a borrowed ArkUI_NodeHandle. Returns nonzero on
// success. Detach or release the view before destroying the parent node.
LYNX_CAPI_EXPORT int lynx_view_attach_harmony(lynx_view_t* view,
                                              void* parent_node);

// Removes the root from its parent without destroying the page engine.
LYNX_CAPI_EXPORT void lynx_view_detach_harmony(lynx_view_t* view);

LYNX_EXTERN_C_END

#endif  // PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_VIEW_HARMONY_CAPI_H_
