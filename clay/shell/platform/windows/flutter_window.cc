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
#include <string>
#include <utility>

#include "clay/fml/file.h"
#include "clay/fml/logging.h"
#include "clay/fml/paths.h"
#include "clay/fml/platform/win/wstring_conversion.h"
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

struct CursorResourceMapping {
  const wchar_t* system_cursor;
  const char* chromium_asset;
};

constexpr char kChromiumCursorResourceDir[] = "resources/cursors/chromium";

const CursorResourceMapping* FindCursorResourceMapping(
    clay::CursorTypes cursor_type) {
  static const auto* mappings =
      new std::map<clay::CursorTypes, CursorResourceMapping>{
          {clay::CursorTypes::kAlias, {nullptr, "aliasb.cur"}},
          {clay::CursorTypes::kBasic, {IDC_ARROW, nullptr}},
          {clay::CursorTypes::kCell, {nullptr, "cell.cur"}},
          {clay::CursorTypes::kClick, {IDC_HAND, nullptr}},
          {clay::CursorTypes::kForbidden, {IDC_NO, nullptr}},
          {clay::CursorTypes::kGrab, {nullptr, "hand_grab.cur"}},
          {clay::CursorTypes::kGrabbing, {nullptr, "hand_grabbing.cur"}},
          {clay::CursorTypes::kHelp, {IDC_HELP, nullptr}},
          {clay::CursorTypes::kMove, {IDC_SIZEALL, nullptr}},
          {clay::CursorTypes::kNone, {nullptr, nullptr}},
          {clay::CursorTypes::kNodrop, {IDC_NO, nullptr}},
          {clay::CursorTypes::kPrecise, {IDC_CROSS, nullptr}},
          {clay::CursorTypes::kProgress, {IDC_APPSTARTING, nullptr}},
          {clay::CursorTypes::kResizedown, {IDC_SIZENS, nullptr}},
          {clay::CursorTypes::kResizedownleft, {IDC_SIZENESW, nullptr}},
          {clay::CursorTypes::kResizedownright, {IDC_SIZENWSE, nullptr}},
          {clay::CursorTypes::kResizeleft, {IDC_SIZEWE, nullptr}},
          {clay::CursorTypes::kResizeleftright, {IDC_SIZEWE, nullptr}},
          {clay::CursorTypes::kResizeright, {IDC_SIZEWE, nullptr}},
          {clay::CursorTypes::kResizeup, {IDC_SIZENS, nullptr}},
          {clay::CursorTypes::kResizeupdown, {IDC_SIZENS, nullptr}},
          {clay::CursorTypes::kResizeupleft, {IDC_SIZENWSE, nullptr}},
          {clay::CursorTypes::kResizeupleftdownright, {IDC_SIZENWSE, nullptr}},
          {clay::CursorTypes::kResizeupright, {IDC_SIZENESW, nullptr}},
          {clay::CursorTypes::kResizeuprightdownleft, {IDC_SIZENESW, nullptr}},
          {clay::CursorTypes::kResizecolumn, {IDC_SIZEWE, "col_resize.cur"}},
          {clay::CursorTypes::kResizerow, {IDC_SIZENS, "row_resize.cur"}},
          {clay::CursorTypes::kSystemmousecursor, {nullptr, "copy.cur"}},
          {clay::CursorTypes::kText, {IDC_IBEAM, nullptr}},
          {clay::CursorTypes::kVerticaltext, {nullptr, "vertical_text.cur"}},
          {clay::CursorTypes::kWait, {IDC_WAIT, nullptr}},
          {clay::CursorTypes::kZoomin, {nullptr, "zoom_in.cur"}},
          {clay::CursorTypes::kZoomout, {nullptr, "zoom_out.cur"}},
      };

  auto it = mappings->find(cursor_type);
  return it == mappings->end() ? nullptr : &it->second;
}

bool DirectoryExists(const std::string& path) {
  auto directory =
      fml::OpenDirectory(path.c_str(), false, fml::FilePermission::kRead);
  return directory.is_valid();
}

std::string FindChromiumCursorDirectory() {
  static const auto* cursor_directory = new std::string([] {
    auto executable_directory = fml::paths::GetExecutableDirectoryPath();
    if (!executable_directory.first || executable_directory.second.empty()) {
      return std::string();
    }

    const auto direct_path = fml::paths::JoinPaths(
        {executable_directory.second, kChromiumCursorResourceDir});
    const auto parent_path = fml::paths::JoinPaths(
        {fml::paths::GetDirectoryName(executable_directory.second),
         kChromiumCursorResourceDir});
    const auto candidates = {direct_path, parent_path};

    for (const auto& candidate : candidates) {
      if (DirectoryExists(candidate)) {
        return candidate;
      }
    }
    return std::string();
  }());
  return *cursor_directory;
}

HCURSOR LoadChromiumCursor(const char* asset_name) {
  if (asset_name == nullptr) {
    return nullptr;
  }

  static auto* cached_cursors = new std::map<std::string, HCURSOR>();
  auto it = cached_cursors->find(asset_name);
  if (it != cached_cursors->end()) {
    return it->second;
  }

  const auto cursor_directory = FindChromiumCursorDirectory();
  if (cursor_directory.empty()) {
    return nullptr;
  }

  const auto cursor_path =
      fml::paths::JoinPaths({cursor_directory, asset_name});
  if (!fml::IsFile(cursor_path)) {
    return nullptr;
  }

  auto cursor = reinterpret_cast<HCURSOR>(
      ::LoadImageW(nullptr, fml::Utf8ToWideString(cursor_path).c_str(),
                   IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE | LR_LOADFROMFILE));
  if (cursor == nullptr) {
    FML_LOG(WARNING) << "Failed to load Chromium cursor asset";
    return nullptr;
  }

  cached_cursors->emplace(asset_name, cursor);
  return cursor;
}

// Maps a Flutter cursor name to an HCURSOR.
//
// Returns the arrow cursor for unknown constants.
//
// This map must be kept in sync with Flutter framework's
// services/mouse_cursor.dart.
static HCURSOR GetCursorByType(clay::CursorTypes cursor_type) {
  const auto* mapping = FindCursorResourceMapping(cursor_type);
  if (mapping == nullptr) {
    return ::LoadCursor(nullptr, IDC_ARROW);
  }

  if (mapping->system_cursor != nullptr) {
    auto cursor = ::LoadCursor(nullptr, mapping->system_cursor);
    if (cursor != nullptr) {
      return cursor;
    }
  }

  if (mapping->chromium_asset != nullptr) {
    auto cursor = LoadChromiumCursor(mapping->chromium_asset);
    if (cursor != nullptr) {
      return cursor;
    }
  }

  if (cursor_type == clay::CursorTypes::kNone) {
    return nullptr;
  }

  return ::LoadCursor(nullptr, IDC_ARROW);
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
