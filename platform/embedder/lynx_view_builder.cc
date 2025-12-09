// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/lynx_view_builder_priv.h"

LYNX_EXTERN_C lynx_view_builder_t* lynx_view_builder_create() {
  return new lynx_view_builder_t();
}

LYNX_EXTERN_C void lynx_view_builder_set_screen_size(
    lynx_view_builder_t* builder, const float& width, const float& height,
    const float& pixel_ratio) {
  builder->screen_size.width = width;
  builder->screen_size.height = height;
  builder->screen_size.pixel_ratio = pixel_ratio;
}

LYNX_EXTERN_C void lynx_view_builder_set_frame(lynx_view_builder_t* builder,
                                               const float& x, const float& y,
                                               const float& width,
                                               const float& height) {
  builder->frame.x = x;
  builder->frame.y = y;
  builder->frame.width = width;
  builder->frame.height = height;
}

LYNX_EXTERN_C void lynx_view_builder_set_font_scale(
    lynx_view_builder_t* builder, const float& scale) {
  builder->font_scale = scale;
}

LYNX_EXTERN_C void lynx_view_builder_set_lynx_group(
    lynx_view_builder_t* builder, lynx_group_t* group) {
  builder->group = group;
}

LYNX_EXTERN_C void lynx_view_builder_set_parent(lynx_view_builder_t* builder,
                                                NativeWindow parent) {
  builder->parent = parent;
}

LYNX_EXTERN_C void lynx_view_builder_set_generic_resource_fetcher(
    lynx_view_builder_t* builder,
    lynx_generic_resource_fetcher_t* generic_fetcher) {
  builder->generic_fetcher = generic_fetcher;
}

LYNX_EXTERN_C void lynx_view_builder_register_native_module(
    lynx_view_builder_t* builder, const char* module_name,
    napi_module_creator module_creator, void* opaque) {
  builder->native_modules[module_name] = {module_creator, opaque};
}

LYNX_EXTERN_C void lynx_view_builder_release(lynx_view_builder_t* builder) {
  delete builder;
}
