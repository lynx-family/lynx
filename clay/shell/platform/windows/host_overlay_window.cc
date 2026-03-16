// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/platform/windows/host_overlay_window.h"

#include <string>

namespace {

constexpr wchar_t kWindowClassName[] = L"FLUTTER_HOST_WINDOW";

// Checks whether the window class of name |class_name| is registered for the
// current application.
bool IsClassRegistered(LPCWSTR class_name) {
  WNDCLASSEX window_class = {};
  return GetClassInfoEx(GetModuleHandle(nullptr), class_name, &window_class) !=
         0;
}

// Updates the window frame's theme to match the system theme.
void UpdateTheme(HWND window) {
  // Registry key for app theme preference.
  const wchar_t kGetPreferredBrightnessRegKey[] =
      L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize";
  const wchar_t kGetPreferredBrightnessRegValue[] = L"AppsUseLightTheme";

  // A value of 0 indicates apps should use dark mode. A non-zero or missing
  // value indicates apps should use light mode.
  DWORD light_mode;
  DWORD light_mode_size = sizeof(light_mode);
  LSTATUS const result =
      RegGetValue(HKEY_CURRENT_USER, kGetPreferredBrightnessRegKey,
                  kGetPreferredBrightnessRegValue, RRF_RT_REG_DWORD, nullptr,
                  &light_mode, &light_mode_size);

  if (result == ERROR_SUCCESS) {
    BOOL enable_dark_mode = light_mode == 0;
    DwmSetWindowAttribute(window, DWMWA_USE_IMMERSIVE_DARK_MODE,
                          &enable_dark_mode, sizeof(enable_dark_mode));
  }
}

// Inserts |content| into the window tree.
void SetChildContent(HWND content, HWND window) {
  SetParent(content, window);
  RECT client_rect;
  GetClientRect(window, &client_rect);
  MoveWindow(content, client_rect.left, client_rect.top,
             client_rect.right - client_rect.left,
             client_rect.bottom - client_rect.top, true);
}

// Retrieves the calling thread's last-error code message as a string,
// or a fallback message if the error message cannot be formatted.
std::string GetLastErrorAsString() {
  LPWSTR message_buffer = nullptr;

  if (DWORD const size = FormatMessage(
          FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
              FORMAT_MESSAGE_IGNORE_INSERTS,
          nullptr, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
          reinterpret_cast<LPTSTR>(&message_buffer), 0, nullptr)) {
    std::wstring const wide_message(message_buffer, size);
    LocalFree(message_buffer);
    message_buffer = nullptr;

    if (int const buffer_size =
            WideCharToMultiByte(CP_UTF8, 0, wide_message.c_str(), -1, nullptr,
                                0, nullptr, nullptr)) {
      std::string message(buffer_size, 0);
      WideCharToMultiByte(CP_UTF8, 0, wide_message.c_str(), -1, &message[0],
                          buffer_size, nullptr, nullptr);
      return message;
    }
  }

  if (message_buffer) {
    LocalFree(message_buffer);
  }
  std::ostringstream oss;
  oss << "Format message failed with 0x" << std::hex << std::setfill('0')
      << std::setw(8) << GetLastError();
  return oss.str();
}

}  // namespace

namespace clay {

HostOverlayWindow::HostOverlayWindow(FlutterWindowsEngine* engine)
    : engine_(engine) {}

void HostOverlayWindow::InitializeFlutterView(
    HostOverlayWindowInitializationParams const& params) {
  // Set up the view.
  auto view_window = std::make_unique<FlutterWindow>(
      nullptr, 0, 0, params.initial_window_rect.width(),
      params.initial_window_rect.height());

  view_ = engine_->CreateOverlayView(std::move(view_window));
  FML_CHECK(view_ != nullptr);
  FML_CHECK(engine_->running());

  // Register the window class.
  if (!IsClassRegistered(kWindowClassName)) {
    auto const idi_app_icon = 101;
    WNDCLASSEX window_class = {};
    window_class.cbSize = sizeof(WNDCLASSEX);
    window_class.style = CS_HREDRAW | CS_VREDRAW;
    window_class.lpfnWndProc = HostOverlayWindow::WndProc;
    window_class.hInstance = GetModuleHandle(nullptr);
    window_class.hIcon =
        LoadIcon(window_class.hInstance, MAKEINTRESOURCE(idi_app_icon));
    if (!window_class.hIcon) {
      window_class.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    }
    window_class.hCursor = LoadCursor(nullptr, IDC_ARROW);
    window_class.lpszClassName = kWindowClassName;

    FML_CHECK(RegisterClassEx(&window_class));
  }

  // Create the native window.
  window_handle_ = CreateWindowEx(
      params.extended_window_style, kWindowClassName, params.title,
      params.window_style, params.initial_window_rect.x(),
      params.initial_window_rect.y(), params.initial_window_rect.width(),
      params.initial_window_rect.height(),
      params.owner_window ? *params.owner_window : nullptr, nullptr,
      GetModuleHandle(nullptr), nullptr);
  FML_CHECK(window_handle_ != nullptr);

  // Adjust the window position so its origin aligns with the top-left corner
  // of the window frame, not the window rectangle (which includes the
  // drop-shadow). This adjustment must be done post-creation since the frame
  // rectangle is only available after the window has been created.
  RECT frame_rect;
  DwmGetWindowAttribute(window_handle_, DWMWA_EXTENDED_FRAME_BOUNDS,
                        &frame_rect, sizeof(frame_rect));
  RECT window_rect;
  GetWindowRect(window_handle_, &window_rect);
  LONG const left_dropshadow_width = frame_rect.left - window_rect.left;
  LONG const top_dropshadow_height = window_rect.top - frame_rect.top;
  SetWindowPos(window_handle_, nullptr,
               window_rect.left - left_dropshadow_width,
               window_rect.top - top_dropshadow_height, 0, 0,
               SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

  UpdateTheme(window_handle_);

  SetChildContent(view_->GetWindowHandle(), window_handle_);

  // TODO(loicsharma): Hide the window until the first frame is rendered.
  // Single window apps use the engine's next frame callback to show the
  // window. This doesn't work for multi window apps as the engine cannot have
  // multiple next frame callbacks. If multiple windows are created, only the
  // last one will be shown.
  ShowWindow(window_handle_, params.nCmdShow);
  SetWindowLongPtr(window_handle_, GWLP_USERDATA,
                   reinterpret_cast<LONG_PTR>(this));
}

void HostOverlayWindow::FocusRootViewOf(HostOverlayWindow* window) {
  auto child_content = window->view_->GetWindowHandle();
  if (window != nullptr && child_content != nullptr) {
    SetFocus(child_content);
  }
};

LRESULT HostOverlayWindow::WndProc(HWND hwnd, UINT message, WPARAM wparam,
                                   LPARAM lparam) {
  if (message == WM_NCCREATE) {
  } else if (HostOverlayWindow* const window = GetThisFromHandle(hwnd)) {
    return window->HandleMessage(hwnd, message, wparam, lparam);
  }

  return DefWindowProc(hwnd, message, wparam, lparam);
}

LRESULT HostOverlayWindow::HandleMessage(HWND hwnd, UINT message, WPARAM wparam,
                                         LPARAM lparam) {
  auto result = engine_->window_proc_delegate_manager()->OnTopLevelWindowProc(
      window_handle_, message, wparam, lparam);
  if (result) {
    return *result;
  }

  switch (message) {
    case WM_DESTROY:
      break;

    case WM_NCLBUTTONDOWN: {
      // Fix for 500ms hang after user clicks on the title bar, but before
      // moving mouse. Reference:
      // https://gamedev.net/forums/topic/672094-keeping-things-moving-during-win32-moveresize-events/5254386/
      if (SendMessage(window_handle_, WM_NCHITTEST, wparam, lparam) ==
          HTCAPTION) {
        POINT cursorPos;
        // Get the current cursor position and synthesize WM_MOUSEMOVE to
        // unblock default window proc implementation for WM_NCLBUTTONDOWN at
        // HTCAPTION.
        GetCursorPos(&cursorPos);
        ScreenToClient(window_handle_, &cursorPos);
        PostMessage(window_handle_, WM_MOUSEMOVE, 0,
                    MAKELPARAM(cursorPos.x, cursorPos.y));
      }
      break;
    }

    case WM_DPICHANGED: {
      auto* const new_scaled_window_rect = reinterpret_cast<RECT*>(lparam);
      LONG const width =
          new_scaled_window_rect->right - new_scaled_window_rect->left;
      LONG const height =
          new_scaled_window_rect->bottom - new_scaled_window_rect->top;
      SetWindowPos(hwnd, nullptr, new_scaled_window_rect->left,
                   new_scaled_window_rect->top, width, height,
                   SWP_NOZORDER | SWP_NOACTIVATE);
      return 0;
    }

    case WM_SIZE: {
      auto child_content = view_->GetWindowHandle();
      if (child_content != nullptr) {
        // Resize and reposition the child content window.
        RECT client_rect;
        GetClientRect(hwnd, &client_rect);
        MoveWindow(child_content, client_rect.left, client_rect.top,
                   client_rect.right - client_rect.left,
                   client_rect.bottom - client_rect.top, TRUE);
      }
      return 0;
    }

    case WM_ACTIVATE:
      FocusRootViewOf(this);
      return 0;

    case WM_DWMCOLORIZATIONCOLORCHANGED:
      UpdateTheme(hwnd);
      return 0;

    default:
      break;
  }

  if (!view_) {
    return 0;
  }

  return DefWindowProc(hwnd, message, wparam, lparam);
}

HostOverlayWindow* HostOverlayWindow::GetThisFromHandle(HWND hwnd) {
  wchar_t class_name[256];
  if (!GetClassName(hwnd, class_name, sizeof(class_name) / sizeof(wchar_t))) {
    FML_LOG(ERROR) << "Failed to get class name for window handle " << hwnd
                   << ": " << GetLastErrorAsString();
    return nullptr;
  }
  // Ignore window handles that do not match the expected class name.
  if (wcscmp(class_name, kWindowClassName) != 0) {
    return nullptr;
  }

  return reinterpret_cast<HostOverlayWindow*>(
      GetWindowLongPtr(hwnd, GWLP_USERDATA));
}

FlutterViewId HostOverlayWindow::GetFlutterViewId() const {
  return view_->view_id();
}

HWND HostOverlayWindow::GetWindowHandle() const { return window_handle_; }

HWND HostOverlayWindow::GetFlutterViewWindowHandle() const {
  return view_->GetWindowHandle();
}

HostOverlayWindow::~HostOverlayWindow() {
  if (view_) {
    // Unregister the window class. Fail silently if other windows are still
    // using the class, as only the last window can successfully unregister it.
    if (!UnregisterClass(kWindowClassName, GetModuleHandle(nullptr))) {
      // Clear the error state after the failed unregistration.
      SetLastError(ERROR_SUCCESS);
    }
  }
}

}  // namespace clay
