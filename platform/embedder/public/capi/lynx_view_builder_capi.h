// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_VIEW_BUILDER_CAPI_H_
#define PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_VIEW_BUILDER_CAPI_H_

#include "lynx_export.h"
#include "lynx_extension_module_types_capi.h"
#include "lynx_generic_resource_fetcher_capi.h"
#include "lynx_group_capi.h"
#include "lynx_native_module_capi.h"
#include "lynx_native_view_capi.h"
#include "lynx_windowless_renderer_capi.h"

LYNX_EXTERN_C_BEGIN

typedef void* NativeWindow;

typedef struct lynx_view_builder_t lynx_view_builder_t;
LYNX_CAPI_EXPORT lynx_view_builder_t* lynx_view_builder_create();

// Sets the screen size and pixel ratio for the LynxView being built. This
// function allows you to specify the screen size and pixel ratio of the device
// for the LynxView that is being constructed using the provided builder. The
// screen size affects the layout and rendering of the LynxView, and the pixel
// ratio is used to adjust the rendering quality of high - DPI displays.
LYNX_CAPI_EXPORT void lynx_view_builder_set_screen_size(
    lynx_view_builder_t*, const float& width, const float& height,
    const float& pixel_ratio);

// Set the initial position and size of the LynxView. This function allows you
// to specify the initial position and dimensions of the LynxView being built
// using the provided builder. The position is defined by the `x` and `y`
// coordinates, and the size is defined by the `width` and `height` parameters.
LYNX_CAPI_EXPORT void lynx_view_builder_set_frame(lynx_view_builder_t*,
                                                  const float& x,
                                                  const float& y,
                                                  const float& width,
                                                  const float& height);

// Sets the font scaling ratio for the LynxView being built. This function
// allows you to specify a scaling factor for the font size within the LynxView.
// The text size in the LynxView will be multiplied by this scaling ratio. For
// example, a value of 1.5 will increase the text size by 50%, while 0.8 will
// reduce it by 20%.
LYNX_CAPI_EXPORT void lynx_view_builder_set_font_scale(lynx_view_builder_t*,
                                                       const float& scale);

// Sets the ICU data path for the LynxView being built. This function allows you
// to specify the path to the ICU (International Components for Unicode) data
// file that the LynxView will use for Unicode-related operations such as text
// processing, collation, and formatting. The ICU data is essential for proper
// handling of international text and multilingual support in the LynxView.
LYNX_CAPI_EXPORT void lynx_view_builder_set_icu_data_path(
    lynx_view_builder_t*, const char* icu_data_path);

// Sets the fixed WebView2 runtime path for the LynxView being built. Windows
// WebView native views first try the system WebView2 runtime, then retry with
// this fixed runtime path if the system runtime is unavailable or fails.
LYNX_CAPI_EXPORT void lynx_view_builder_set_webview2_fixed_runtime_path(
    lynx_view_builder_t*, const char* webview2_fixed_runtime_path);

// Associates a Lynx group with the LynxView being built. This function allows
// you to assign a specific Lynx group to the LynxView that is currently under
// construction using the provided builder. The Lynx group can contain shared
// resources, configurations, or state that the LynxView may utilize. By
// associating a Lynx group, the LynxView can inherit and interact with the
// group's context, which can be useful for managing resources and coordinating
// behavior across multiple views.
LYNX_CAPI_EXPORT void lynx_view_builder_set_lynx_group(lynx_view_builder_t*,
                                                       lynx_group_t* group);

// Sets the parent window for the Lynx view being built. This function assigns a
// parent native window to the Lynx view that is being constructed using the
// provided builder. The parent window will contain the Lynx view, and the
// view's position and behavior may be influenced by its parent.
LYNX_CAPI_EXPORT void lynx_view_builder_set_parent(lynx_view_builder_t*,
                                                   NativeWindow parent);

// Sets the windowless renderer for the Lynx view being built. This function
// assigns a windowless renderer to the Lynx view that is being constructed
// using the provided builder. The windowless renderer is responsible for
// rendering the Lynx view without a visible window, which can be useful for
// offscreen rendering or headless scenarios.
LYNX_CAPI_EXPORT void lynx_view_builder_set_windowless_renderer(
    lynx_view_builder_t*, lynx_windowless_renderer_t*);

// generic fetcher.
LYNX_CAPI_EXPORT void lynx_view_builder_set_generic_resource_fetcher(
    lynx_view_builder_t*, lynx_generic_resource_fetcher_t* fetcher);

// Register instance-level native module, which have a higher priority than
// global modules.
LYNX_CAPI_EXPORT void lynx_view_builder_register_native_module(
    lynx_view_builder_t* builder, const char* name, napi_module_creator creator,
    void* opaque);

// Register instance-level extension module, which have a higher priority than
// global extension modules.
LYNX_CAPI_EXPORT void lynx_view_builder_register_extension_module(
    lynx_view_builder_t* builder, const char* name,
    extension_module_creator creator, bool is_lazy_create, void* opaque);

// Register instance-level view factory.
LYNX_CAPI_EXPORT void lynx_view_builder_register_native_view(
    lynx_view_builder_t* builder, const char* name,
    lynx_native_view_creator creator, void* opaque);

LYNX_CAPI_EXPORT void lynx_view_builder_release(lynx_view_builder_t* builder);

LYNX_EXTERN_C_END

#endif  // PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_VIEW_BUILDER_CAPI_H_
