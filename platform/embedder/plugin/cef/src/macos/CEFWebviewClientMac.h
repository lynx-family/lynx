// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_EMBEDDER_PLUGIN_CEF_SRC_MACOS_CEFWEBVIEWCLIENTMAC_H_
#define PLATFORM_EMBEDDER_PLUGIN_CEF_SRC_MACOS_CEFWEBVIEWCLIENTMAC_H_

#include "platform/embedder/plugin/cef/src/cef_webview_client.h"
#include "platform/embedder/plugin/cef/src/macos/CEFTextInputClientOSRMac.h"

namespace lynx {
namespace plugin {
namespace embedder {

class CEFWebviewMac;

class CEFWebviewClientMac : public CEFWebviewClient {
 public:
  explicit CEFWebviewClientMac(CEFWebview* webview)
      : CEFWebviewClient(webview) {}

  // CefRenderHandler
  void GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect) override;

  bool GetScreenInfo(CefRefPtr<CefBrowser> browser,
                     CefScreenInfo& screen_info) override;

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

  void OnKeyUp(NSEvent* event);
  void OnKeyDown(NSEvent* event);

 private:
  NSTextInputContext* text_input_context_osr_mac_ = nullptr;
  CefTextInputClientOSRMac* text_input_client_ = nullptr;
};

}  // namespace embedder
}  // namespace plugin
}  // namespace lynx

#endif  // PLATFORM_EMBEDDER_PLUGIN_CEF_SRC_MACOS_CEFWEBVIEWCLIENTMAC_H_
