// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_EMBEDDER_PLUGIN_CEF_SRC_CEF_WEBVIEW_CLIENT_H_
#define PLATFORM_EMBEDDER_PLUGIN_CEF_SRC_CEF_WEBVIEW_CLIENT_H_

#include <string>
#include <vector>

#include "include/capi/cef_app_capi.h"
#include "include/cef_base.h"
#include "include/cef_browser.h"
#include "include/cef_client.h"
#include "include/wrapper/cef_library_loader.h"
#include "include/wrapper/cef_message_router.h"

namespace lynx {
namespace plugin {
namespace embedder {

class CEFWebview;

class CEFWebviewClient : public CefClient,
                         public CefLifeSpanHandler,
                         public CefDragHandler,
                         public CefDisplayHandler,
                         public CefRenderHandler,
                         public CefFrameHandler,
                         public CefLoadHandler,
                         public CefRequestHandler,
                         public CefContextMenuHandler,
                         public CefMessageRouterBrowserSide::Handler {
 public:
  explicit CEFWebviewClient(CEFWebview* webview) : webview_(webview) {}

  // CefClient
  CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override { return this; }
  CefRefPtr<CefDisplayHandler> GetDisplayHandler() override { return this; }
  CefRefPtr<CefLoadHandler> GetLoadHandler() override { return this; }
  CefRefPtr<CefRequestHandler> GetRequestHandler() override { return this; }
  CefRefPtr<CefRenderHandler> GetRenderHandler() override { return this; }
  CefRefPtr<CefFrameHandler> GetFrameHandler() override { return this; }
  CefRefPtr<CefContextMenuHandler> GetContextMenuHandler() override {
    return this;
  }

  bool OnProcessMessageReceived(CefRefPtr<CefBrowser> browser,
                                CefRefPtr<CefFrame> frame,
                                CefProcessId source_process,
                                CefRefPtr<CefProcessMessage> message) override;

  // CefFrameHandler
  void OnFrameCreated(CefRefPtr<CefBrowser> browser,
                      CefRefPtr<CefFrame> frame) override;

  // CefLifeSpanHandler
  void OnAfterCreated(CefRefPtr<CefBrowser> browser) override;

  bool OnBeforePopup(
      CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int popup_id,
      const CefString& target_url, const CefString& target_frame_name,
      CefLifeSpanHandler::WindowOpenDisposition target_disposition,
      bool user_gesture, const CefPopupFeatures& popupFeatures,
      CefWindowInfo& windowInfo, CefRefPtr<CefClient>& client,
      CefBrowserSettings& settings, CefRefPtr<CefDictionaryValue>& extra_info,
      bool* no_javascript_access) override;

  void OnBeforeClose(CefRefPtr<CefBrowser> browser) override;

  bool OnBeforeBrowse(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
                      CefRefPtr<CefRequest> request, bool user_gesture,
                      bool is_redirect) override;

  bool DoClose(CefRefPtr<CefBrowser> browser) override;

  // CefDisplayHandler
  void OnAddressChange(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
                       const CefString& url) override;

  // CefLoadHandler
  void OnLoadEnd(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
                 int httpStatusCode) override;

  void OnLoadError(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
                   ErrorCode errorCode, const CefString& errorText,
                   const CefString& failedUrl) override;

  // CefRequestHandler
  void OnRenderProcessTerminated(CefRefPtr<CefBrowser> browser,
                                 CefRequestHandler::TerminationStatus status,
                                 int error_code,
                                 const CefString& error_string) override;

  // CefMessageRouterBrowserSide::Handler
  bool OnQuery(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
               int64_t query_id, const CefString& request, bool persistent,
               CefRefPtr<Callback> callback) override;

  // CefRenderHandler
  bool GetScreenInfo(CefRefPtr<CefBrowser> browser,
                     CefScreenInfo& screen_info) override;

  void OnPaint(CefRefPtr<CefBrowser> browser, PaintElementType type,
               const RectList& dirtyRects, const void* buffer, int width,
               int height) override {}

  // CefContextMenuHandler
  void OnBeforeContextMenu(CefRefPtr<CefBrowser> browser,
                           CefRefPtr<CefFrame> frame,
                           CefRefPtr<CefContextMenuParams> params,
                           CefRefPtr<CefMenuModel> model) override;
  bool OnContextMenuCommand(CefRefPtr<CefBrowser> browser,
                            CefRefPtr<CefFrame> frame,
                            CefRefPtr<CefContextMenuParams> params,
                            int command_id, EventFlags event_flags) override;

  // CefDragHandler
  bool OnDragEnter(CefRefPtr<CefBrowser> browser,
                   CefRefPtr<CefDragData> dragData,
                   CefDragHandler::DragOperationsMask mask) override;

  void OnDraggableRegionsChanged(
      CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
      const std::vector<CefDraggableRegion>& regions) override;

  void Reset() { webview_ = nullptr; }
  bool IsImeShown() const { return ime_shown_; }
  void SetLoaded(bool loaded) { loaded_ = loaded; }

 protected:
  virtual void AttachToView(CefRefPtr<CefBrowser> browser) {}

  CEFWebview* webview_;
  CefRefPtr<CefMessageRouterBrowserSide> message_router_;
  std::string location_;
  bool loaded_ = false;
  bool ime_shown_ = false;
  IMPLEMENT_REFCOUNTING(CEFWebviewClient);
};

}  // namespace embedder
}  // namespace plugin
}  // namespace lynx

#endif  // PLATFORM_EMBEDDER_PLUGIN_CEF_SRC_CEF_WEBVIEW_CLIENT_H_
