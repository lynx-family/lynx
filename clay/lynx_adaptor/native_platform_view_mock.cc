// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/lynx_adaptor/native_platform_view.h"

namespace clay {
NativePlatformView::~NativePlatformView() {}
bool NativePlatformView::OnCreate() { return false; }
void NativePlatformView::OnAttach() {}
void NativePlatformView::OnDetach() {}
void NativePlatformView::OnDestroy() {}
void NativePlatformView::OnLayoutChanged(float left, float top, float width,
                                         float height, float pixel_ratio) {}
void NativePlatformView::OnPropertiesChanged(lynx_value attrs,
                                             lynx_value events) {}
void NativePlatformView::OnMouseClickEvent(int x, int y, int buttons,
                                           bool mouse_up) {}
void NativePlatformView::OnMouseMoveEvent(int x, int y, int modifiers,
                                          bool mouse_leave) {}
void NativePlatformView::OnMouseWheelEvent(int x, int y, int modifiers,
                                           double delta_x, double delta_y) {}
void NativePlatformView::OnFocusChanged(bool focused, bool is_leaf) {}
void NativePlatformView::OnMethodInvoked(
    const std::string& method, lynx_value attrs,
    std::function<void(int code, lynx_value data)> callback) {
  callback(kMethodNotFound, {.val_ptr = 0, .type = lynx_value_undefined});
}
void NativePlatformView::TriggerEvent(const char* name, lynx_value params) {}
ClaySharedImageSinkRef NativePlatformView::GetSharedImageSink() {
  return nullptr;
}
bool NativePlatformView::PresentSurface(
    int width, int height, const ClayTransformation* transform,
    ClaySharedImageNativeHandle gfx_handle) {
  return false;
}
ClaySharedImageNativeHandle NativePlatformView::AcquireSurface(int width,
                                                               int height) {
  return nullptr;
}
bool NativePlatformView::SwapBack() { return false; }
void NativePlatformView::MarkDirty() {}
void NativePlatformView::RequestFocus() {}
}  // namespace clay
