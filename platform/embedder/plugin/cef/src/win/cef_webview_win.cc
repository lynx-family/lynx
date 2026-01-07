// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/plugin/cef/src/win/cef_webview_win.h"

#include <VersionHelpers.h>
#include <Windows.h>

#include <set>

#include "platform/embedder/plugin/cef/src/win/cef_webview_client_win.h"
#include "platform/embedder/plugin/cef/src/win/dpi_utils_win32.h"
#include "platform/embedder/public/capi/lynx_view_capi.h"

#ifndef GWL_HWNDPARENT
#define GWL_HWNDPARENT -8
#endif

// For Windows 7
static auto* kClassName = L"ClayPlatformViewWnd";
static ATOM gAtomWindowType = 0;
static HHOOK gWinHook = nullptr;
static std::set<lynx::plugin::embedder::CEFWebviewWin*> gWatchList;
static bool isWindows7 = !IsWindows8OrGreater();

namespace lynx {
namespace plugin {
namespace embedder {

CEFWebviewWin::CEFWebviewWin(lynx_view_t* lynx_view) : CEFWebview(lynx_view) {
  if (!gAtomWindowType) {
    WNDCLASSEX class_ex = {sizeof(class_ex),
                           CS_HREDRAW | CS_VREDRAW,
                           DefWindowProc,
                           0,
                           0,
                           ::GetModuleHandle(nullptr),
                           nullptr,
                           ::LoadCursor(nullptr, IDC_ARROW),
                           0,
                           nullptr,
                           kClassName,
                           nullptr};
    gAtomWindowType = ::RegisterClassEx(&class_ex);
  }
}

void CEFWebviewWin::OnAttach() {
  if (win7_owned_win_) {
    ::ShowWindow(win7_owned_win_, SW_SHOW);
  }
  if (browser_) {
    CefRefPtr<CefBrowserHost> host = browser_->GetHost();
    if (hwnd_) {
      if (isWindows7) {
        ::SetWindowPos(win7_owned_win_, HWND_TOP, 0, 0, 0, 0,
                       SWP_NOSIZE | SWP_NOMOVE);
      } else {
        auto* native_window = lynx_view_get_native_window(lynx_view_);
        ::SetParent(hwnd_, reinterpret_cast<HWND>(native_window));
      }
      ::ShowWindow(hwnd_, SW_SHOW);
    }
    host->WasHidden(false);
  }
}

void CEFWebviewWin::OnDetach() {
  if (win7_owned_win_) {
    ::ShowWindow(win7_owned_win_, SW_HIDE);
  }
  if (browser_) {
    CefRefPtr<CefBrowserHost> host = browser_->GetHost();
    if (hwnd_) {
      ::ShowWindow(hwnd_, SW_HIDE);
    }
    host->WasHidden(true);
  }
}

void CEFWebviewWin::OnDestroy() {
  if (browser_) {
    CefRefPtr<CefBrowserHost> host = browser_->GetHost();
    if (hwnd_) {
      ::SetParent(hwnd_, nullptr);
    }
    host->CloseBrowser(true);
  }
  browser_.reset();
  if (client_) {
    client_->Reset();
    client_ = nullptr;
  }

  auto search = gWatchList.find(this);
  if (search != gWatchList.end()) {
    gWatchList.erase(search);
  }
  if (win7_owned_win_) {
    ::DestroyWindow(win7_owned_win_);
    win7_owned_win_ = nullptr;
  }
}

void CEFWebviewWin::OnLayoutChanged(float left, float top, float width,
                                    float height, float pixel_ratio) {
  if (use_osr_) {
    RECT prev = bounds_;
    bounds_.left = LONG(left);
    bounds_.top = LONG(top);
    bounds_.right = LONG(left) + LONG(width);
    bounds_.bottom = LONG(top) + LONG(height);
    if (browser_ &&
        (prev.right != bounds_.right || prev.bottom != bounds_.bottom)) {
      CefRefPtr<CefBrowserHost> host = browser_->GetHost();
      // host->NotifyScreenInfoChanged();
      host->WasResized();
      host->Invalidate(PET_VIEW);
    }
  } else {
    bounds_ = RECT{LONG(left * pixel_ratio), LONG(top * pixel_ratio),
                   LONG((left + width) * pixel_ratio),
                   LONG((top + height) * pixel_ratio)};
    if (isWindows7) {
      AdjustOwnedWinAndChild(hwnd_);
    } else if (hwnd_) {
      ::SetWindowPos(hwnd_, nullptr, bounds_.left, bounds_.top,
                     bounds_.right - bounds_.left, bounds_.bottom - bounds_.top,
                     SWP_NOZORDER);
    }
  }
}

float CEFWebviewWin::GetPixelRatio() const {
  auto* native_window = lynx_view_get_native_window(lynx_view_);
  return lynx::embedder::plugin::GetDpiForHWND(
             reinterpret_cast<HWND>(native_window)) /
         96.0;
}

void CEFWebviewWin::SetupClient() {
  CefBrowserSettings settings;
  CefWindowInfo window_info;
  settings.chrome_status_bubble = STATE_DISABLED;
  // window_info.runtime_style = CEF_RUNTIME_STYLE_ALLOY;
  if (use_osr_) {
    auto* native_window = lynx_view_get_native_window(lynx_view_);
    window_info.SetAsWindowless(reinterpret_cast<HWND>(native_window));
    window_info.shared_texture_enabled = 1;
    settings.windowless_frame_rate = fps_;
  } else if (isWindows7) {
    window_info.SetAsChild(GetOrCreateOwnedWin(),
                           CefRect(0, 0, bounds_.right - bounds_.left,
                                   bounds_.bottom - bounds_.top));
  } else {
    auto* native_window = lynx_view_get_native_window(lynx_view_);
    window_info.SetAsChild(
        reinterpret_cast<HWND>(native_window),
        CefRect(bounds_.left, bounds_.top, bounds_.right - bounds_.left,
                bounds_.bottom - bounds_.top));
  }
  client_ = new CEFWebviewClientWin(this);
  CefBrowserHost::CreateBrowser(window_info, client_, url_, settings, nullptr,
                                nullptr);
}

namespace {
static LRESULT CALLBACK CallWndProc(int code, WPARAM wparam, LPARAM lparam) {
  if (code >= 0 && !gWatchList.empty()) {
    CWPSTRUCT* cwp = reinterpret_cast<CWPSTRUCT*>(lparam);
    if (GetParent(cwp->hwnd) == nullptr) {  // Only for top level window
      if (cwp->message == WM_MOVE) {
        for (auto* v : gWatchList) {
          v->MoveOwnedWinOnly();
        }
      } else if (cwp->message == WM_SIZE) {
        for (auto* v : gWatchList) {
          v->AdjustOwnedWinAndChild(nullptr);
        }
      }
    }
  }
  return CallNextHookEx(gWinHook, code, wparam, lparam);
}
}  // namespace

// private
HWND CEFWebviewWin::GetOrCreateOwnedWin() {
  if (!win7_owned_win_) {
    auto* native_window = lynx_view_get_native_window(lynx_view_);
    HWND parent = reinterpret_cast<HWND>(native_window);
    RECT rect = bounds_;
    ::ClientToScreen(parent, (LPPOINT)&rect.left);
    ::ClientToScreen(parent, (LPPOINT)&rect.right);

    win7_owned_win_ = ::CreateWindowEx(
        0, kClassName, 0,
        WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE, rect.left,
        rect.top, rect.right - rect.left, rect.bottom - rect.top,
        nullptr /*hWndParent */, nullptr, nullptr, nullptr);
    ::SetWindowLongPtr(win7_owned_win_, GWL_HWNDPARENT, LONG_PTR(parent));
    if (!gWinHook) {
      gWinHook = ::SetWindowsHookEx(WH_CALLWNDPROC, CallWndProc, nullptr,
                                    GetCurrentThreadId());
    }
    gWatchList.insert(this);
  }
  return win7_owned_win_;
}

void CEFWebviewWin::AdjustOwnedWinAndChild(HWND child) {
  if (win7_owned_win_) {
    auto* native_window = lynx_view_get_native_window(lynx_view_);
    HWND parent = reinterpret_cast<HWND>(native_window);
    RECT owned_rect = bounds_;
    RECT child_rect = {0, 0, bounds_.right - bounds_.left,
                       bounds_.bottom - bounds_.top};
    RECT client;
    ::GetClientRect(parent, &client);
    if (owned_rect.left < client.left) {
      child_rect.left -= client.left - owned_rect.left;
      owned_rect.left = client.left;
    }
    if (owned_rect.top < client.top) {
      child_rect.top -= client.top - owned_rect.top;
      owned_rect.top = client.top;
    }
    if (owned_rect.right > client.right) {
      owned_rect.right = client.right;
    }
    if (owned_rect.bottom > client.bottom) {
      owned_rect.bottom = client.bottom;
    }
    ::ClientToScreen(parent, (LPPOINT)&owned_rect.left);
    ::ClientToScreen(parent, (LPPOINT)&owned_rect.right);
    ::SetWindowPos(win7_owned_win_, nullptr, owned_rect.left, owned_rect.top,
                   owned_rect.right - owned_rect.left,
                   owned_rect.bottom - owned_rect.top,
                   SWP_NOZORDER | SWP_NOACTIVATE);
    if (child) {
      ::SetWindowPos(child, nullptr, child_rect.left, child_rect.top,
                     child_rect.right - child_rect.left,
                     child_rect.bottom - child_rect.top,
                     SWP_NOZORDER | SWP_NOACTIVATE);
    }
  }
}

void CEFWebviewWin::OnMouseWheelEvent(int x, int y, int modifiers,
                                      double delta_x, double delta_y) {
  if (!browser_) {
    return;
  }
  if (CefRefPtr<CefBrowserHost> host = browser_->GetHost()) {
    CefMouseEvent mouse_event;
    mouse_event.x = x;
    mouse_event.y = y;
    mouse_event.modifiers = 0;
    if (modifiers & 1) {
      mouse_event.modifiers |= EVENTFLAG_LEFT_MOUSE_BUTTON;
    }
    if (modifiers & 2) {
      mouse_event.modifiers |= EVENTFLAG_RIGHT_MOUSE_BUTTON;
    }
    if (modifiers & 4) {
      mouse_event.modifiers |= EVENTFLAG_MIDDLE_MOUSE_BUTTON;
    }
    if (::GetKeyState(VK_NUMLOCK) & 1) {
      mouse_event.modifiers |= EVENTFLAG_NUM_LOCK_ON;
    }
    if (::GetKeyState(VK_CAPITAL) & 1) {
      mouse_event.modifiers |= EVENTFLAG_CAPS_LOCK_ON;
    }
    host->SendMouseWheelEvent(mouse_event, delta_x, delta_y);
  }
}

void CEFWebviewWin::OnMouseMoveEvent(int x, int y, int modifiers,
                                     bool mouse_leave) {
  if (!browser_) {
    return;
  }
  if (CefRefPtr<CefBrowserHost> host = browser_->GetHost()) {
    CefMouseEvent mouse_event;
    mouse_event.x = x;
    mouse_event.y = y;
    mouse_event.modifiers = 0;
    if (modifiers & 1) {
      mouse_event.modifiers |= EVENTFLAG_LEFT_MOUSE_BUTTON;
    }
    if (modifiers & 2) {
      mouse_event.modifiers |= EVENTFLAG_RIGHT_MOUSE_BUTTON;
    }
    if (modifiers & 4) {
      mouse_event.modifiers |= EVENTFLAG_MIDDLE_MOUSE_BUTTON;
    }
    if (::GetKeyState(VK_NUMLOCK) & 1) {
      mouse_event.modifiers |= EVENTFLAG_NUM_LOCK_ON;
    }
    if (::GetKeyState(VK_CAPITAL) & 1) {
      mouse_event.modifiers |= EVENTFLAG_CAPS_LOCK_ON;
    }
    host->SendMouseMoveEvent(mouse_event, mouse_leave);
  }
}

void CEFWebviewWin::OnMouseClickEvent(int x, int y, int buttons,
                                      bool mouse_up) {
  if (!browser_) {
    return;
  }

  if (!mouse_up) {
    LONG currentTime = GetMessageTime();
    if ((abs(last_click_x_ - x) > (GetSystemMetrics(SM_CXDOUBLECLK) / 2)) ||
        (abs(last_click_y_ - y) > (GetSystemMetrics(SM_CYDOUBLECLK) / 2)) ||
        ((currentTime - last_click_time_) > GetDoubleClickTime()) ||
        buttons != last_click_button_) {
      last_click_count_ = 1;
    } else {
      ++last_click_count_;
    }
    last_click_time_ = currentTime;
    last_click_button_ = buttons;
    last_click_x_ = x;
    last_click_y_ = y;
  }

  if (CefRefPtr<CefBrowserHost> host = browser_->GetHost()) {
    CefMouseEvent mouse_event;
    mouse_event.x = x;
    mouse_event.y = y;
    host->SendMouseClickEvent(
        mouse_event,
        buttons & 1
            ? MBT_LEFT
            : (buttons & 2 ? MBT_RIGHT : (buttons & 4 ? MBT_MIDDLE : MBT_LEFT)),
        mouse_up, last_click_count_);
  }
}

void CEFWebviewWin::MoveOwnedWinOnly() {
  auto* native_window = lynx_view_get_native_window(lynx_view_);
  HWND parent = reinterpret_cast<HWND>(native_window);
  POINT owned_pt = {bounds_.left, bounds_.top};
  ::ClientToScreen(parent, &owned_pt);
  ::SetWindowPos(win7_owned_win_, nullptr, owned_pt.x, owned_pt.y, 0, 0,
                 SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSIZE);
}

}  // namespace embedder
}  // namespace plugin
}  // namespace lynx

LYNX_EXTERN_C lynx_native_view_t* cef_webview_create_view(void* opaque) {
  lynx::plugin::embedder::CEFWebviewWin* view =
      new lynx::plugin::embedder::CEFWebviewWin(
          static_cast<lynx_view_t*>(opaque));
  return view->native_view();
}
