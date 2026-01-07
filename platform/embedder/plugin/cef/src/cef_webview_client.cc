// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/plugin/cef/src/cef_webview_client.h"

#include <utility>

#include "platform/embedder/plugin/cef/src/cef_webview.h"

static const int kOpenDevtoolCommandID = 1001;
static const char kOpenDevToolsText[] = "Open DevTools";
namespace lynx {
namespace plugin {
namespace embedder {

// CefWebviewClient
bool CEFWebviewClient::OnProcessMessageReceived(
    CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
    CefProcessId source_process, CefRefPtr<CefProcessMessage> message) {
  return message_router_->OnProcessMessageReceived(browser, frame,
                                                   source_process, message);
}

void CEFWebviewClient::OnFrameCreated(CefRefPtr<CefBrowser> browser,
                                      CefRefPtr<CefFrame> frame) {
  if (!webview_ || !webview_->browser_) {
    return;
  }
  std::string js =
      "window.addEventListener('message', "
      "e => "
      "window.cefQuery({request: 'LyNxSig_' + e.data}));" +
      webview_->init_js_;
  frame->ExecuteJavaScript(js, "<host>", 1);
}

void CEFWebviewClient::OnAfterCreated(CefRefPtr<CefBrowser> browser) {
  if (webview_->browser_) {
    return;
  }
  webview_->browser_ = browser;
  if (!webview_->use_osr_) {
    AttachToView(browser);
  }

  if (!webview_->set_cookie_methods_.empty()) {
    auto cookie_manager = CefCookieManager::GetGlobalManager(nullptr);
    for (const auto& m : webview_->set_cookie_methods_) {
      cookie_manager->SetCookie(m.url, m.cookie, nullptr);
    }
    webview_->set_cookie_methods_.clear();
  }

  CefMessageRouterConfig config;
  message_router_ = CefMessageRouterBrowserSide::Create(config);
  message_router_->AddHandler(this, false);
}

bool CEFWebviewClient::OnBeforePopup(
    CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int popup_id,
    const CefString& target_url, const CefString& target_frame_name,
    CefLifeSpanHandler::WindowOpenDisposition target_disposition,
    bool user_gesture, const CefPopupFeatures& popupFeatures,
    CefWindowInfo& windowInfo, CefRefPtr<CefClient>& client,
    CefBrowserSettings& settings, CefRefPtr<CefDictionaryValue>& extra_info,
    bool* no_javascript_access) {
  if (target_disposition == CEF_WOD_NEW_PICTURE_IN_PICTURE) {
    client = nullptr;
    return false;
  }
  if (!webview_) {
    return true;
  }
  lynx::pub::LynxValue detail(lynx::pub::LynxValue::kCreateAsMapTag);
  detail.SetProperty("url", lynx::pub::LynxValue(target_url.ToString()));
  webview_->TriggerEvent("openwindow", std::move(detail));
  return true;
}

void CEFWebviewClient::OnBeforeClose(CefRefPtr<CefBrowser> browser) {
  if (webview_ && browser.get() != webview_->browser_.get()) {
    return;
  }
  if (message_router_) {
    message_router_->RemoveHandler(this);
    message_router_ = nullptr;
  }
  // TODO(chenyouhui): sink ref.
}

bool CEFWebviewClient::OnBeforeBrowse(CefRefPtr<CefBrowser> browser,
                                      CefRefPtr<CefFrame> frame,
                                      CefRefPtr<CefRequest> request,
                                      bool user_gesture, bool is_redirect) {
  message_router_->OnBeforeBrowse(browser, frame);
  return false;
}

bool CEFWebviewClient::DoClose(CefRefPtr<CefBrowser> browser) {
  if (webview_) {
    webview_->SetClosing(true);
  }
  return false;
}

void CEFWebviewClient::OnAddressChange(CefRefPtr<CefBrowser> browser,
                                       CefRefPtr<CefFrame> frame,
                                       const CefString& url) {
  std::string location = url.ToString();
  if (frame->IsMain() && loaded_) {
    if (location != location_ && location.find("devtools://") != 0) {
      location_ = location;

      lynx::pub::LynxValue detail(lynx::pub::LynxValue::kCreateAsMapTag);
      detail.SetProperty("url", lynx::pub::LynxValue(location));
      webview_->TriggerEvent("locationchange", std::move(detail));
    }
  }
}

void CEFWebviewClient::OnLoadEnd(CefRefPtr<CefBrowser> browser,
                                 CefRefPtr<CefFrame> frame,
                                 int httpStatusCode) {
  if (!webview_) {
    return;
  }
  if (httpStatusCode == 200 && frame->IsMain() && !loaded_) {
    loaded_ = true;
    webview_->TriggerEvent(
        "load", lynx::pub::LynxValue(lynx::pub::LynxValue::kCreateAsNullTag));
  }
}

void CEFWebviewClient::OnLoadError(CefRefPtr<CefBrowser> browser,
                                   CefRefPtr<CefFrame> frame,
                                   ErrorCode errorCode,
                                   const CefString& errorText,
                                   const CefString& failedUrl) {
  if (!webview_) {
    return;
  }
  lynx::pub::LynxValue detail(lynx::pub::LynxValue::kCreateAsMapTag);
  detail.SetProperty("errorCode", lynx::pub::LynxValue(int(errorCode)));
  detail.SetProperty("errorMsg", lynx::pub::LynxValue(errorText.ToString()));
  detail.SetProperty("failedUrl", lynx::pub::LynxValue(failedUrl.ToString()));
  webview_->TriggerEvent("error", std::move(detail));
}

void CEFWebviewClient::OnRenderProcessTerminated(
    CefRefPtr<CefBrowser> browser, CefRequestHandler::TerminationStatus status,
    int error_code, const CefString& error_string) {
  message_router_->OnRenderProcessTerminated(browser);
}

bool CEFWebviewClient::OnQuery(CefRefPtr<CefBrowser> browser,
                               CefRefPtr<CefFrame> frame, int64_t query_id,
                               const CefString& request, bool persistent,
                               CefRefPtr<Callback> callback) {
  if (!webview_) {
    return false;
  }
  const std::string& message = request;
  if (message.size() > 8 && memcmp(message.c_str(), "LyNxSig_", 8) == 0) {
    lynx::pub::LynxValue detail(lynx::pub::LynxValue::kCreateAsMapTag);
    detail.SetProperty("msg", lynx::pub::LynxValue(message.c_str() + 8));
    webview_->TriggerEvent("message", std::move(detail));
  }
  return false;
}

bool CEFWebviewClient::GetScreenInfo(CefRefPtr<CefBrowser> browser,
                                     CefScreenInfo& screen_info) {
  float ratio = webview_->GetPixelRatio();
  CefRect view_rect;
  GetViewRect(browser, view_rect);
  screen_info.device_scale_factor = ratio;
  screen_info.rect = view_rect;
  screen_info.available_rect = view_rect;
  return true;
}

void CEFWebviewClient::OnBeforeContextMenu(
    CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
    CefRefPtr<CefContextMenuParams> params, CefRefPtr<CefMenuModel> model) {
  if (!webview_->EnableDevtool()) {
    model->Clear();
    // model->Remove(IDC_VIEW_SOURCE);
    // model->Remove(IDC_CONTENT_CONTEXT_INSPECTELEMENT);
  } else {
    model->AddSeparator();
    model->AddItem(kOpenDevtoolCommandID, kOpenDevToolsText);
    model->SetEnabled(kOpenDevtoolCommandID, true);
  }
}

bool CEFWebviewClient::OnContextMenuCommand(
    CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
    CefRefPtr<CefContextMenuParams> params, int command_id,
    EventFlags event_flags) {
  if (command_id == kOpenDevtoolCommandID) {
    // TODO(chenyouhui): Add implementation.
    // OpenDevTools(browser);
    return true;
  }
  return false;
}

bool CEFWebviewClient::OnDragEnter(CefRefPtr<CefBrowser> browser,
                                   CefRefPtr<CefDragData> dragData,
                                   CefDragHandler::DragOperationsMask mask) {
  return true;
}

void CEFWebviewClient::OnDraggableRegionsChanged(
    CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
    const std::vector<CefDraggableRegion>& regions) {
  // TODO(chenyouhui): Add implementation.
}

}  // namespace embedder
}  // namespace plugin
}  // namespace lynx
