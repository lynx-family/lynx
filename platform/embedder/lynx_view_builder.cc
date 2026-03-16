// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/log/logging.h"
#include "platform/embedder/lynx_view_builder_priv.h"
#include "platform/embedder/native_view/lynx_texture_view.h"

LYNX_EXTERN_C lynx_view_builder_t* lynx_view_builder_create() {
  auto builder = new lynx_view_builder_t();

  lynx_view_builder_register_native_view(
      builder, "x-texture-view",
      [](void* opaque) -> lynx_native_view_t* {
        return (new LynxTextureView(opaque))->native_view();
      },
      nullptr);
  return builder;
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

LYNX_EXTERN_C void lynx_view_builder_set_icu_data_path(
    lynx_view_builder_t* builder, const char* icu_data_path) {
  builder->icu_data_path = icu_data_path ? icu_data_path : "";
}

LYNX_EXTERN_C void lynx_view_builder_set_lynx_group(
    lynx_view_builder_t* builder, lynx_group_t* group) {
  builder->group = group;
}

LYNX_EXTERN_C void lynx_view_builder_set_parent(lynx_view_builder_t* builder,
                                                NativeWindow parent) {
  builder->parent = parent;
}

LYNX_EXTERN_C void lynx_view_builder_set_windowless_renderer(
    lynx_view_builder_t* builder,
    lynx_windowless_renderer_t* windowless_renderer) {
#if defined(ENABLE_WINDOWLESS)
  builder->windowless_renderer =
      lynx::fml::RefPtr<lynx::embedder::LynxWindowlessRenderer>(
          reinterpret_cast<lynx::embedder::LynxWindowlessRenderer*>(
              windowless_renderer));
#else
  UNIMPLEMENTED();
#endif
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

LYNX_EXTERN_C void lynx_view_builder_register_extension_module(
    lynx_view_builder_t* builder, const char* name,
    extension_module_creator creator, bool is_lazy_create, void* opaque) {
  builder->extension_modules_.emplace(
      name, std::make_tuple(creator, is_lazy_create, opaque));
}

LYNX_EXTERN_C void lynx_view_builder_register_native_view(
    lynx_view_builder_t* builder, const char* name,
    lynx_native_view_creator creator, void* opaque) {
  builder->native_view_creators[name] = {creator, opaque};
}

LYNX_EXTERN_C void lynx_view_builder_release(lynx_view_builder_t* builder) {
  delete builder;
}
