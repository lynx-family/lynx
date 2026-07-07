// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/lynx_adaptor/ui_delegate_clay.h"
#include "clay/ui/component/page_view.h"
#include "clay/ui/component/view_context.h"
#include "platform/embedder/lynx_view_priv.h"
#include "platform/embedder/public/capi/lynx_view_builder_capi.h"
#include "platform/embedder/public/capi/lynx_view_capi.h"

namespace {

clay::PageView* GetPageView(lynx_view_t* view) {
  if (!view || !view->lynx_ui_renderer) {
    return nullptr;
  }
  auto* ui_delegate = view->lynx_ui_renderer->GetUIDelegate();
  auto* clay_delegate = static_cast<lynx::tasm::UIDelegateClay*>(ui_delegate);
  if (!clay_delegate || !clay_delegate->GetViewContext()) {
    return nullptr;
  }
  return clay_delegate->GetViewContext()->GetPageView();
}

}  // namespace

extern "C" __attribute__((visibility("default"))) void
lynx_rust_view_builder_set_screen_size(lynx_view_builder_t* builder,
                                       float width, float height,
                                       float pixel_ratio) {
  lynx_view_builder_set_screen_size(builder, width, height, pixel_ratio);
}

extern "C" __attribute__((visibility("default"))) void
lynx_rust_view_builder_set_frame(lynx_view_builder_t* builder, float x, float y,
                                 float width, float height) {
  lynx_view_builder_set_frame(builder, x, y, width, height);
}

extern "C" __attribute__((visibility("default"))) void
lynx_rust_view_builder_set_font_scale(lynx_view_builder_t* builder,
                                      float scale) {
  lynx_view_builder_set_font_scale(builder, scale);
}

extern "C" __attribute__((visibility("default"))) void
lynx_rust_view_update_screen_metrics(lynx_view_t* view, float width,
                                     float height, float pixel_ratio) {
  lynx_view_update_screen_metrics(view, width, height, pixel_ratio);
}

extern "C" __attribute__((visibility("default"))) void lynx_rust_view_set_frame(
    lynx_view_t* view, float x, float y, float width, float height) {
  lynx_view_set_frame(view, x, y, width, height);
}

extern "C" __attribute__((visibility("default"))) void
lynx_rust_view_set_font_scale(lynx_view_t* view, float font_scale) {
  lynx_view_set_font_scale(view, font_scale);
}

extern "C" __attribute__((visibility("default"))) bool
lynx_rust_view_set_use_texture_backend(lynx_view_t* view,
                                       bool use_texture_backend) {
  auto* page_view = GetPageView(view);
  if (!page_view) {
    return false;
  }
  page_view->SetUseTextureBackend(use_texture_backend);
  return true;
}
