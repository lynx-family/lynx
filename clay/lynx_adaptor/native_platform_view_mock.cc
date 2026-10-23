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
void NativePlatformView::OnPropertiesChanged(const lynx::pub::Value& attrs,
                                             const lynx::pub::Value& events) {}
void NativePlatformView::OnMouseClickEvent(int x, int y, int buttons,
                                           bool mouse_up) {}
void NativePlatformView::OnMouseMoveEvent(int x, int y, int modifiers,
                                          bool mouse_leave) {}
void NativePlatformView::OnMouseWheelEvent(int x, int y, int modifiers,
                                           double delta_x, double delta_y) {}
void NativePlatformView::OnFocusChanged(bool focused, bool is_leaf) {}
void NativePlatformView::OnMethodInvoked(
    const std::string& method, const lynx::pub::Value& attrs,
    std::function<void(int, const lynx::pub::Value&)> callback) {
  callback(kMethodNotFound, attrs);
}
void NativePlatformView::TriggerEvent(const char* name,
                                      const lynx::pub::Value& params) {}
ClaySharedImageSinkRef NativePlatformView::GetSharedImageSink() {
  return nullptr;
}
bool NativePlatformView::PresentSurface(
    int width, int height, const ClayTransformation* transform,
    ClaySharedImageNativeHandle gfx_handle) {
  return false;
}
ClaySharedImageNativeHandle NativePlatformView::AcquireSurface(
    int width, int height, const ClayTransformation* transform) {
  return nullptr;
}
bool NativePlatformView::SwapBack() { return false; }
void NativePlatformView::MarkDirty() {}
void NativePlatformView::RequestFocus() {}
}  // namespace clay
