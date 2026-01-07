// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/plugin/cef/src/win/cef_webview_client_win.h"

#include <VersionHelpers.h>

#include <algorithm>
#include <limits>
#include <vector>

#include "platform/embedder/plugin/cef/src/win/cef_webview_win.h"
#include "platform/embedder/public/lynx_native_view.h"

namespace lynx {
namespace plugin {
namespace embedder {

CEFWebviewClientWin::CEFWebviewClientWin(CEFWebviewWin* webview)
    : CEFWebviewClient(webview) {
  parent_ = reinterpret_cast<HWND>(
      lynx_view_get_native_window(webview->GetLynxView()));
  ime_handler_ = std::make_unique<OsrImeHandlerWin>(parent_);
}

void CEFWebviewClientWin::GetViewRect(CefRefPtr<CefBrowser> browser,
                                      CefRect& rect) {
  if (!webview_) {
    return;
  }
  auto& bounds = static_cast<CEFWebviewWin*>(webview_)->bounds_;
  rect.Set(0, 0, std::max(1L, bounds.right - bounds.left),
           std::max(1L, bounds.bottom - bounds.top));
}

bool CEFWebviewClientWin::GetScreenPoint(CefRefPtr<CefBrowser> browser,
                                         int viewX, int viewY, int& screenX,
                                         int& screenY) {
  float ratio = webview_->GetPixelRatio();
  POINT screen_pt = {LONG(viewX * ratio), LONG(viewY * ratio)};
  ::ClientToScreen(parent_, &screen_pt);
  screenX = screen_pt.x;
  screenY = screen_pt.y;
  return true;
}

void CEFWebviewClientWin::OnAcceleratedPaint(
    CefRefPtr<CefBrowser> browser, PaintElementType type,
    const RectList& dirtyRects, const CefAcceleratedPaintInfo& info) {
  // TODO: check image sink
  static const float transform[3 * 3] = {1, 0, 0, 0, -1, 1, 0, 0, 1};
  webview_->PresentSurface(
      info.extra.source_size.width, info.extra.source_size.height, transform,
      reinterpret_cast<lynx_surface_handle_t*>(info.shared_texture_handle));
}

void CEFWebviewClientWin::OnImeCompositionRangeChanged(
    CefRefPtr<CefBrowser> browser, const CefRange& selection_range,
    const CefRenderHandler::RectList& character_bounds) {
  if (ime_handler_) {
    float ratio = webview_->GetPixelRatio();
    auto& bounds = static_cast<CEFWebviewWin*>(webview_)->bounds_;
    CefRenderHandler::RectList device_bounds;
    CefRenderHandler::RectList::const_iterator it = character_bounds.begin();
    for (; it != character_bounds.end(); ++it) {
      CefRect rect((bounds.left + it->x) * ratio, (bounds.top + it->y) * ratio,
                   it->width * ratio, it->height * ratio);
      device_bounds.push_back(rect);
    }

    ime_handler_->ChangeCompositionRange(selection_range, device_bounds);
  }
}

void CEFWebviewClientWin::OnIMESetContext(UINT message, WPARAM wParam,
                                          LPARAM lParam) {
  if (ime_handler_) {
    ime_handler_->CreateImeWindow();
    ime_handler_->MoveImeWindow();
  }
}

void CEFWebviewClientWin::OnIMEStartComposition() {
  if (ime_handler_) {
    ime_handler_->CreateImeWindow();
    ime_handler_->MoveImeWindow();
    ime_handler_->ResetComposition();
  }
}

namespace {
static CefRange InvalidRange() {
  return CefRange((std::numeric_limits<uint32_t>::max)(),
                  (std::numeric_limits<uint32_t>::max)());
}

static bool IsKeyDown(WPARAM wparam) {
  return (::GetKeyState(wparam) & 0x8000) != 0;
}

static int GetCefKeyboardModifiers(WPARAM wparam, LPARAM lparam) {
  int modifiers = 0;
  if (IsKeyDown(VK_SHIFT)) {
    modifiers |= EVENTFLAG_SHIFT_DOWN;
  }
  if (IsKeyDown(VK_CONTROL)) {
    modifiers |= EVENTFLAG_CONTROL_DOWN;
  }
  if (IsKeyDown(VK_MENU)) {
    modifiers |= EVENTFLAG_ALT_DOWN;
  }

  // Low bit set from GetKeyState indicates "toggled".
  if (::GetKeyState(VK_NUMLOCK) & 1) {
    modifiers |= EVENTFLAG_NUM_LOCK_ON;
  }
  if (::GetKeyState(VK_CAPITAL) & 1) {
    modifiers |= EVENTFLAG_CAPS_LOCK_ON;
  }

  switch (wparam) {
    case VK_RETURN:
      if ((lparam >> 16) & KF_EXTENDED) {
        modifiers |= EVENTFLAG_IS_KEY_PAD;
      }
      break;
    case VK_INSERT:
    case VK_DELETE:
    case VK_HOME:
    case VK_END:
    case VK_PRIOR:
    case VK_NEXT:
    case VK_UP:
    case VK_DOWN:
    case VK_LEFT:
    case VK_RIGHT:
      if (!((lparam >> 16) & KF_EXTENDED)) {
        modifiers |= EVENTFLAG_IS_KEY_PAD;
      }
      break;
    case VK_NUMLOCK:
    case VK_NUMPAD0:
    case VK_NUMPAD1:
    case VK_NUMPAD2:
    case VK_NUMPAD3:
    case VK_NUMPAD4:
    case VK_NUMPAD5:
    case VK_NUMPAD6:
    case VK_NUMPAD7:
    case VK_NUMPAD8:
    case VK_NUMPAD9:
    case VK_DIVIDE:
    case VK_MULTIPLY:
    case VK_SUBTRACT:
    case VK_ADD:
    case VK_DECIMAL:
    case VK_CLEAR:
      modifiers |= EVENTFLAG_IS_KEY_PAD;
      break;
    case VK_SHIFT:
      if (IsKeyDown(VK_LSHIFT)) {
        modifiers |= EVENTFLAG_IS_LEFT;
      } else if (IsKeyDown(VK_RSHIFT)) {
        modifiers |= EVENTFLAG_IS_RIGHT;
      }
      break;
    case VK_CONTROL:
      if (IsKeyDown(VK_LCONTROL)) {
        modifiers |= EVENTFLAG_IS_LEFT;
      } else if (IsKeyDown(VK_RCONTROL)) {
        modifiers |= EVENTFLAG_IS_RIGHT;
      }
      break;
    case VK_MENU:
      if (IsKeyDown(VK_LMENU)) {
        modifiers |= EVENTFLAG_IS_LEFT;
      } else if (IsKeyDown(VK_RMENU)) {
        modifiers |= EVENTFLAG_IS_RIGHT;
      }
      break;
    case VK_LWIN:
      modifiers |= EVENTFLAG_IS_LEFT;
      break;
    case VK_RWIN:
      modifiers |= EVENTFLAG_IS_RIGHT;
      break;
  }
  return modifiers;
}
}  // namespace

void CEFWebviewClientWin::OnIMEComposition(UINT message, WPARAM wParam,
                                           LPARAM lParam) {
  if (webview_->GetBrowser() && ime_handler_) {
    CefString cTextStr;
    if (ime_handler_->GetResult(lParam, cTextStr)) {
      webview_->GetBrowser()->GetHost()->ImeCommitText(cTextStr, InvalidRange(),
                                                       0);
      ime_handler_->ResetComposition();
    }

    std::vector<CefCompositionUnderline> underlines;
    int composition_start = 0;

    if (ime_handler_->GetComposition(lParam, cTextStr, underlines,
                                     composition_start)) {
      webview_->GetBrowser()->GetHost()->ImeSetComposition(
          cTextStr, underlines, InvalidRange(),
          CefRange(composition_start,
                   static_cast<int>(composition_start + cTextStr.length())));
      ime_handler_->UpdateCaretPosition(composition_start - 1);
    } else {
      OnIMECancelCompositionEvent();
    }
  }
}

void CEFWebviewClientWin::OnIMECancelCompositionEvent() {
  if (webview_->GetBrowser() && ime_handler_) {
    webview_->GetBrowser()->GetHost()->ImeCancelComposition();
    ime_handler_->ResetComposition();
    ime_handler_->DestroyImeWindow();
  }
}

void CEFWebviewClientWin::OnKeyEvent(UINT message, WPARAM wParam,
                                     LPARAM lParam) {
  if (!webview_->GetBrowser()) {
    return;
  }

  CefKeyEvent event;
  event.windows_key_code = wParam;
  event.native_key_code = lParam;
  event.is_system_key = message == WM_SYSCHAR || message == WM_SYSKEYDOWN ||
                        message == WM_SYSKEYUP;
  if (message == WM_KEYDOWN || message == WM_SYSKEYDOWN) {
    event.type = KEYEVENT_RAWKEYDOWN;
  } else if (message == WM_KEYUP || message == WM_SYSKEYUP) {
    event.type = KEYEVENT_KEYUP;
  } else {
    event.type = KEYEVENT_CHAR;
  }
  event.modifiers = GetCefKeyboardModifiers(wParam, lParam);
  if ((event.type == KEYEVENT_CHAR) && IsKeyDown(VK_RMENU)) {
    HKL current_layout = ::GetKeyboardLayout(0);
    SHORT scan_res = ::VkKeyScanExW(wParam, current_layout);
    constexpr auto ctrlAlt = (2 | 4);
    if (((scan_res >> 8) & ctrlAlt) == ctrlAlt) {  // ctrl-alt pressed
      event.modifiers &= ~(EVENTFLAG_CONTROL_DOWN | EVENTFLAG_ALT_DOWN);
      event.modifiers |= EVENTFLAG_ALTGR_DOWN;
    }
  }
  webview_->GetBrowser()->GetHost()->SendKeyEvent(event);
}

namespace {

static LRESULT ime_callback(HWND hWnd, UINT message, WPARAM wparam,
                            LPARAM lparam, CEFWebviewClientWin* client) {
  switch (message) {
    case WM_IME_SETCONTEXT:
      lparam &= ~ISC_SHOWUICOMPOSITIONWINDOW;
      ::DefWindowProc(hWnd, message, wparam, lparam);
      client->OnIMESetContext(message, wparam, lparam);
      return 0;
    case WM_IME_STARTCOMPOSITION:
      client->OnIMEStartComposition();
      return 0;
    case WM_IME_COMPOSITION:
      client->OnIMEComposition(message, wparam, lparam);
      return 0;
    case WM_IME_ENDCOMPOSITION:
      client->OnIMECancelCompositionEvent();
      break;
    case WM_SYSCHAR:
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_CHAR:
      client->OnKeyEvent(message, wparam, lparam);
      break;
  }
  return ::DefWindowProc(hWnd, message, wparam, lparam);
}
}  // namespace

void CEFWebviewClientWin::OnVirtualKeyboardRequested(
    CefRefPtr<CefBrowser> browser, TextInputMode input_mode) {
  switch (input_mode) {
    case CEF_TEXT_INPUT_MODE_NONE:
      if (!ime_shown_) {
        return;
      }
      ime_shown_ = false;
      webview_->RegisterIMEHandler(nullptr, nullptr);
      break;
    default:
      if (ime_shown_) {
        return;
      }
      ime_shown_ = true;
      webview_->RegisterIMEHandler(reinterpret_cast<void*>(ime_callback), this);
      webview_->RequestFocus();
      break;
  }
}

bool CEFWebviewClientWin::OnCursorChange(
    CefRefPtr<CefBrowser> browser, CefCursorHandle cursor,
    cef_cursor_type_t type, const CefCursorInfo& custom_cursor_info) {
  if (webview_ && webview_->UseOSR()) {
    // Change the window's cursor.
    SetClassLongPtr(parent_, GCLP_HCURSOR,
                    static_cast<LONG>(reinterpret_cast<LONG_PTR>(cursor)));
    SetCursor(cursor);
    return true;
  }
  return false;
}

void CEFWebviewClientWin::AttachToView(CefRefPtr<CefBrowser> browser) {
  CefRefPtr<CefBrowserHost> host = browser->GetHost();
  HWND hwnd = host->GetWindowHandle();
  static_cast<CEFWebviewWin*>(webview_)->hwnd_ = hwnd;

  if (!IsWindows8OrGreater()) {
    static_cast<CEFWebviewWin*>(webview_)->AdjustOwnedWinAndChild(hwnd);
  } else {
    auto& bounds = static_cast<CEFWebviewWin*>(webview_)->bounds_;
    ::SetWindowPos(hwnd, nullptr, bounds.left, bounds.top,
                   bounds.right - bounds.left, bounds.bottom - bounds.top,
                   SWP_NOZORDER);
  }
}

}  // namespace embedder
}  // namespace plugin
}  // namespace lynx
