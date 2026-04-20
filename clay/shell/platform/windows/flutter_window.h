// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_PLATFORM_WINDOWS_FLUTTER_WINDOW_H_
#define CLAY_SHELL_PLATFORM_WINDOWS_FLUTTER_WINDOW_H_

#include <iostream>
#include <string>
#include <vector>

#include "clay/gfx/geometry/float_rect.h"
#include "clay/shell/platform/windows/flutter_windows_view.h"
#include "clay/shell/platform/windows/window.h"
#include "clay/shell/platform/windows/window_binding_handler.h"

namespace clay {

// A win32 flutter child window used as implementations for flutter view.  In
// the future, there will likely be a CoreWindow-based FlutterWindow as well.
// At the point may make sense to dependency inject the native window rather
// than inherit.
class FlutterWindow : public Window, public WindowBindingHandler {
 public:
  // Create flutter Window for use as child window
  FlutterWindow(HWND parent_hwnd, int x, int y, int width, int height);

  virtual ~FlutterWindow();

  // |Window|
  void OnDpiScale(unsigned int dpi) override;

  // |Window|
  void OnResize(unsigned int width, unsigned int height) override;

  // |Window|
  void OnPaint() override;

  // |Window|
  void OnPointerMove(double x, double y, ClayPointerDeviceKind device_kind,
                     int32_t device_id, int modifiers_state) override;

  // |Window|
  void OnPointerDown(double x, double y, ClayPointerDeviceKind device_kind,
                     int32_t device_id, UINT button) override;

  // |Window|
  void OnPointerUp(double x, double y, ClayPointerDeviceKind device_kind,
                   int32_t device_id, UINT button) override;

  // |Window|
  void OnPointerLeave(double x, double y, ClayPointerDeviceKind device_kind,
                      int32_t device_id) override;

  // |Window|
  void OnSetCursor() override;

  // |Window|
  void OnText(const std::u16string& text) override;

  // |Window|
  void OnKey(int key, int scancode, int action, char32_t character,
             bool extended, bool was_down, KeyEventCallback callback) override;

  // |Window|
  void OnComposeBegin() override;

  // |Window|
  void OnComposeCommit() override;

  // |Window|
  void OnComposeEnd() override;

  // |Window|
  void OnComposeChange(const std::u16string& text, int cursor_pos) override;

  // |FlutterWindowBindingHandler|
  void OnCursorRectUpdated(const FloatRect& rect) override;

  // |FlutterWindowBindingHandler|
  void OnResetImeComposing() override;

  // |FlutterWindowBindingHandler|
  void OnTextInputClientChange(int client_id) override;

  // |Window|
  void OnScroll(double delta_x, double delta_y,
                ClayPointerDeviceKind device_kind, int32_t device_id) override;

  // |FlutterWindowBindingHandler|
  void SetView(WindowBindingHandlerDelegate* view) override;

  // |FlutterWindowBindingHandler|
  WindowsRenderTarget GetRenderTarget() override;

  // |FlutterWindowBindingHandler|
  HWND GetWindowHandle() override;

  // |FlutterWindowBindingHandler|
  float GetDpiScale() override;

  // |FlutterWindowBindingHandler|
  bool IsVisible() override;

  // |FlutterWindowBindingHandler|
  PhysicalWindowBounds GetPhysicalWindowBounds() override;

  // |FlutterWindowBindingHandler|
  void UpdateFlutterCursor(clay::CursorTypes cursor_name) override;

  // |FlutterWindowBindingHandler|
  void SetFlutterCursor(HCURSOR cursor) override;

  // |FlutterWindowBindingHandler|
  void OnWindowResized() override;

  // |FlutterWindowBindingHandler|
  bool OnBitmapSurfaceUpdated(const void* allocation, size_t row_bytes,
                              size_t height) override;

  // |FlutterWindowBindingHandler|
  PointerLocation GetPrimaryPointerLocation() override;

  // |Window|
  void OnThemeChange() override;

  // |WindowBindingHandler|
  bool NeedsVSync() override;

 private:
  // A pointer to a FlutterWindowsView that can be used to update engine
  // windowing and input state.
  WindowBindingHandlerDelegate* binding_handler_delegate_;
};

}  // namespace clay

#endif  // CLAY_SHELL_PLATFORM_WINDOWS_FLUTTER_WINDOW_H_
