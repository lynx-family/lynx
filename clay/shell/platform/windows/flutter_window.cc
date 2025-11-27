// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/platform/windows/flutter_window.h"

#include <WinUser.h>
#include <dwmapi.h>

#include <chrono>
#include <map>
#include <utility>

#include "clay/fml/logging.h"
#include "clay/gfx/geometry/float_point.h"
#include "clay/gfx/geometry/float_size.h"
#include "clay/shell/platform/windows/flutter_windows_engine.h"
#include "clay/shell/platform/windows/flutter_windows_view.h"
#include "clay/ui/platform/cursor_types.h"

// cspell:disable
namespace clay {

namespace {

// The Windows DPI system is based on this
// constant for machines running at 100% scaling.
constexpr int base_dpi = 96;

// Maps a Flutter cursor name to an HCURSOR.
//
// Returns the arrow cursor for unknown constants.
//
// This map must be kept in sync with Flutter framework's
// services/mouse_cursor.dart.

static HCURSOR GetCursorByType(clay::CursorTypes cursor_type) {
  static auto* cursors = new std::map<clay::CursorTypes, const wchar_t*>{
      {clay::CursorTypes::kClick, IDC_HAND},
      {clay::CursorTypes::kForbidden, IDC_NO},
      {clay::CursorTypes::kHelp, IDC_HELP},
      {clay::CursorTypes::kMove, IDC_SIZEALL},
      {clay::CursorTypes::kNone, nullptr},
      {clay::CursorTypes::kNodrop, IDC_NO},
      {clay::CursorTypes::kPrecise, IDC_CROSS},
      {clay::CursorTypes::kProgress, IDC_APPSTARTING},
      {clay::CursorTypes::kText, IDC_IBEAM},
      {clay::CursorTypes::kResizecolumn, IDC_SIZEWE},
      {clay::CursorTypes::kResizedown, IDC_SIZENS},
      {clay::CursorTypes::kResizedownleft, IDC_SIZENESW},
      {clay::CursorTypes::kResizedownright, IDC_SIZENWSE},
      {clay::CursorTypes::kResizeleft, IDC_SIZEWE},
      {clay::CursorTypes::kResizeleftright, IDC_SIZEWE},
      {clay::CursorTypes::kResizeright, IDC_SIZEWE},
      {clay::CursorTypes::kResizerow, IDC_SIZENS},
      {clay::CursorTypes::kResizeup, IDC_SIZENS},
      {clay::CursorTypes::kResizeupdown, IDC_SIZENS},
      {clay::CursorTypes::kResizeupleft, IDC_SIZENWSE},
      {clay::CursorTypes::kResizeupright, IDC_SIZENESW},
      {clay::CursorTypes::kResizeupleftdownright, IDC_SIZENWSE},
      {clay::CursorTypes::kResizeuprightdownleft, IDC_SIZENESW},
      {clay::CursorTypes::kWait, IDC_WAIT},
  };

  const wchar_t* idc_name = IDC_ARROW;
  auto it = cursors->find(cursor_type);
  if (it != cursors->end()) {
    idc_name = it->second;
  }
  return ::LoadCursor(nullptr, idc_name);
}

}  // namespace

FlutterWindow::FlutterWindow(HWND parent_hwnd, int x, int y, int width,
                             int height)
    : binding_handler_delegate_(nullptr) {
  Window::InitializeChild("FLUTTERVIEW", parent_hwnd, x, y, width, height);
  auto cursor = ::LoadCursor(nullptr, IDC_ARROW);
  SetClassLongPtr(GetWindowHandle(), GCLP_HCURSOR,
                  static_cast<LONG>(reinterpret_cast<LONG_PTR>(cursor)));
}

FlutterWindow::~FlutterWindow() {}

void FlutterWindow::SetView(WindowBindingHandlerDelegate* window) {
  binding_handler_delegate_ = window;
  direct_manipulation_owner_->SetBindingHandlerDelegate(window);
}

WindowsRenderTarget FlutterWindow::GetRenderTarget() {
  return WindowsRenderTarget(GetWindowHandle());
}

PlatformWindow FlutterWindow::GetPlatformWindow() { return GetWindowHandle(); }

float FlutterWindow::GetDpiScale() {
  return static_cast<float>(GetCurrentDPI()) / static_cast<float>(base_dpi);
}

bool FlutterWindow::IsVisible() { return IsWindowVisible(GetWindowHandle()); }

PhysicalWindowBounds FlutterWindow::GetPhysicalWindowBounds() {
  return {GetCurrentWidth(), GetCurrentHeight()};
}

void FlutterWindow::UpdateFlutterCursor(clay::CursorTypes cursor_type) {
  auto cursor = GetCursorByType(cursor_type);
  SetClassLongPtr(GetWindowHandle(), GCLP_HCURSOR,
                  static_cast<LONG>(reinterpret_cast<LONG_PTR>(cursor)));
}

void FlutterWindow::SetFlutterCursor(HCURSOR cursor) {
  SetClassLongPtr(GetWindowHandle(), GCLP_HCURSOR,
                  static_cast<LONG>(reinterpret_cast<LONG_PTR>(cursor)));
  ::SetCursor(cursor);
}

void FlutterWindow::OnWindowResized() {
  // Blocking the raster thread until DWM flushes alleviates glitches where
  // previous size surface is stretched over current size view.
  DwmFlush();
}

// Translates button codes from Win32 API to ClayPointerMouseButtons.
static uint64_t ConvertWinButtonToFlutterButton(UINT button) {
  switch (button) {
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
      return kClayPointerMouseButtonsMousePrimary;
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
      return kClayPointerMouseButtonsMouseSecondary;
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
      return kClayPointerMouseButtonsMouseMiddle;
    case XBUTTON1:
      return kClayPointerMouseButtonsMouseBack;
    case XBUTTON2:
      return kClayPointerMouseButtonsMouseForward;
  }
  FML_LOG(WARNING) << "Mouse button not recognized: " << button;
  return 0;
}

void FlutterWindow::OnDpiScale(unsigned int dpi) {}

// When DesktopWindow notifies that a WM_Size message has come in
// lets FlutterEngine know about the new size.
void FlutterWindow::OnResize(unsigned int width, unsigned int height) {
  if (binding_handler_delegate_ != nullptr) {
    binding_handler_delegate_->OnWindowSizeChanged(width, height);
  }
}

void FlutterWindow::OnPaint() {
  if (binding_handler_delegate_ != nullptr) {
    binding_handler_delegate_->OnWindowRepaint();
  }
}

void FlutterWindow::OnPointerMove(double x, double y,
                                  ClayPointerDeviceKind device_kind,
                                  int32_t device_id, int modifiers_state) {
  binding_handler_delegate_->OnPointerMove(x, y, device_kind, device_id,
                                           modifiers_state);
}

void FlutterWindow::OnPointerDown(double x, double y,
                                  ClayPointerDeviceKind device_kind,
                                  int32_t device_id, UINT button) {
  uint64_t flutter_button = ConvertWinButtonToFlutterButton(button);
  if (flutter_button != 0) {
    binding_handler_delegate_->OnPointerDown(
        x, y, device_kind, device_id,
        static_cast<ClayPointerMouseButtons>(flutter_button));
  }
}

void FlutterWindow::OnPointerUp(double x, double y,
                                ClayPointerDeviceKind device_kind,
                                int32_t device_id, UINT button) {
  uint64_t flutter_button = ConvertWinButtonToFlutterButton(button);
  if (flutter_button != 0) {
    binding_handler_delegate_->OnPointerUp(
        x, y, device_kind, device_id,
        static_cast<ClayPointerMouseButtons>(flutter_button));
  }
}

void FlutterWindow::OnPointerLeave(double x, double y,
                                   ClayPointerDeviceKind device_kind,
                                   int32_t device_id) {
  binding_handler_delegate_->OnPointerLeave(x, y, device_kind, device_id);
}

void FlutterWindow::OnSetCursor() {
  auto cursor = GetClassLongPtr(GetWindowHandle(), GCLP_HCURSOR);
  ::SetCursor(reinterpret_cast<HCURSOR>(cursor));
}

void FlutterWindow::OnText(const std::u16string& text) {
  binding_handler_delegate_->OnText(text);
}

void FlutterWindow::OnKey(int key, int scancode, int action, char32_t character,
                          bool extended, bool was_down,
                          KeyEventCallback callback) {
  binding_handler_delegate_->OnKey(key, scancode, action, character, extended,
                                   was_down, std::move(callback));
}

void FlutterWindow::OnComposeBegin() {
  binding_handler_delegate_->OnComposeBegin();
}

void FlutterWindow::OnComposeCommit() {
  binding_handler_delegate_->OnComposeCommit();
}

void FlutterWindow::OnComposeEnd() {
  binding_handler_delegate_->OnComposeEnd();
}

void FlutterWindow::OnComposeChange(const std::u16string& text,
                                    int cursor_pos) {
  binding_handler_delegate_->OnComposeChange(text, cursor_pos);
}

void FlutterWindow::OnScroll(double delta_x, double delta_y,
                             ClayPointerDeviceKind device_kind,
                             int32_t device_id) {
  POINT point;
  GetCursorPos(&point);

  ScreenToClient(GetWindowHandle(), &point);
  binding_handler_delegate_->OnScroll(point.x, point.y, delta_x, delta_y,
                                      GetScrollOffsetMultiplier(), device_kind,
                                      device_id);
}

void FlutterWindow::OnCursorRectUpdated(const FloatRect& rect) {
  // Convert the rect from Flutter logical coordinates to device coordinates.
  auto scale = GetDpiScale();
  FloatPoint origin(rect.left() * scale, rect.top() * scale);
  FloatSize size(rect.width() * scale, rect.height() * scale);
  UpdateCursorRect(FloatRect(origin, size));
}

void FlutterWindow::OnResetImeComposing() { AbortImeComposing(); }

void FlutterWindow::OnTextInputClientChange(int client_id) {
  UpdateTextInputClientFocused(client_id);
}

bool FlutterWindow::OnBitmapSurfaceUpdated(const void* allocation,
                                           size_t row_bytes, size_t height) {
  HDC dc = ::GetDC(GetWindowHandle());
  BITMAPINFO bmi = {};
  bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bmi.bmiHeader.biWidth = row_bytes / 4;
  bmi.bmiHeader.biHeight = -height;
  bmi.bmiHeader.biPlanes = 1;
  bmi.bmiHeader.biBitCount = 32;
  bmi.bmiHeader.biCompression = BI_RGB;
  bmi.bmiHeader.biSizeImage = 0;
  int ret = SetDIBitsToDevice(dc, 0, 0, row_bytes / 4, height, 0, 0, 0, height,
                              allocation, &bmi, DIB_RGB_COLORS);
  ::ReleaseDC(GetWindowHandle(), dc);
  return ret != 0;
}

PointerLocation FlutterWindow::GetPrimaryPointerLocation() {
  POINT point;
  GetCursorPos(&point);
  ScreenToClient(GetWindowHandle(), &point);
  return {(size_t)point.x, (size_t)point.y};
}

void FlutterWindow::OnThemeChange() {
  binding_handler_delegate_->UpdateHighContrastEnabled(
      GetHighContrastEnabled());
}

bool FlutterWindow::NeedsVSync() {
  // If the Desktop Window Manager composition is enabled,
  // the system itself synchronizes with v-sync.
  // See: https://learn.microsoft.com/windows/win32/dwm/composition-ovw
  BOOL composition_enabled;
  if (SUCCEEDED(::DwmIsCompositionEnabled(&composition_enabled))) {
    return !composition_enabled;
  }

  return true;
}

}  // namespace clay
