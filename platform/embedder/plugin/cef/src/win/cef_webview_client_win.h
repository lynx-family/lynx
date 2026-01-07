// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_EMBEDDER_PLUGIN_CEF_SRC_WIN_CEF_WEBVIEW_CLIENT_WIN_H_
#define PLATFORM_EMBEDDER_PLUGIN_CEF_SRC_WIN_CEF_WEBVIEW_CLIENT_WIN_H_

#include <Windows.h>

#include <memory>

#include "platform/embedder/plugin/cef/src/cef_webview_client.h"
#include "platform/embedder/plugin/cef/src/win/osr_ime_handler_win.h"

namespace lynx {
namespace plugin {
namespace embedder {

class CEFWebviewWin;

class CEFWebviewClientWin : public CEFWebviewClient {
 public:
  explicit CEFWebviewClientWin(CEFWebviewWin* webview);

  // CefRenderHandler
  void GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect) override;

  bool GetScreenPoint(CefRefPtr<CefBrowser> browser, int viewX, int viewY,
                      int& screenX, int& screenY) override;

  void OnAcceleratedPaint(CefRefPtr<CefBrowser> browser, PaintElementType type,
                          const RectList& dirtyRects,
                          const CefAcceleratedPaintInfo& info) override;

  void OnImeCompositionRangeChanged(
      CefRefPtr<CefBrowser> browser, const CefRange& selection_range,
      const CefRenderHandler::RectList& character_bounds) override;

  void OnVirtualKeyboardRequested(CefRefPtr<CefBrowser> browser,
                                  TextInputMode input_mode) override;

  // CefDisplayHandler methods.
  bool OnCursorChange(CefRefPtr<CefBrowser> browser, CefCursorHandle cursor,
                      cef_cursor_type_t type,
                      const CefCursorInfo& custom_cursor_info) override;

  void AttachToView(CefRefPtr<CefBrowser> browser) override;

  void OnIMESetContext(UINT message, WPARAM wParam, LPARAM lParam);
  void OnIMEStartComposition();
  void OnIMEComposition(UINT message, WPARAM wParam, LPARAM lParam);
  void OnIMECancelCompositionEvent();
  void OnKeyEvent(UINT message, WPARAM wParam, LPARAM lParam);

 private:
  HWND parent_;
  std::unique_ptr<OsrImeHandlerWin> ime_handler_;
};

}  // namespace embedder
}  // namespace plugin
}  // namespace lynx

#endif  // PLATFORM_EMBEDDER_PLUGIN_CEF_SRC_WIN_CEF_WEBVIEW_CLIENT_WIN_H_
