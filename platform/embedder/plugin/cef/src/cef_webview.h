// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_EMBEDDER_PLUGIN_CEF_SRC_CEF_WEBVIEW_H_
#define PLATFORM_EMBEDDER_PLUGIN_CEF_SRC_CEF_WEBVIEW_H_

#include <string>
#include <vector>

#include "include/capi/cef_app_capi.h"
#include "include/cef_base.h"
#include "include/cef_browser.h"
#include "include/cef_client.h"
#include "include/wrapper/cef_library_loader.h"
#include "include/wrapper/cef_message_router.h"
#include "platform/embedder/plugin/cef/src/cef_webview_client.h"
#include "platform/embedder/public/capi/lynx_export.h"
#include "platform/embedder/public/lynx_native_view.h"
#include "platform/embedder/public/lynx_value.h"

namespace lynx {
namespace plugin {
namespace embedder {

struct SetCookieMethod {
  std::string url;
  CefCookie cookie;

  bool equals(const SetCookieMethod& other) const;
  bool valid() const;
  static SetCookieMethod build(const lynx::pub::LynxValue& attrs);
};

class CEFWebview : public lynx::pub::LynxNativeView {
 public:
  explicit CEFWebview(lynx_view_t* lynx_view) : lynx_view_(lynx_view) {}
  virtual ~CEFWebview() override {}

  bool OnCreate() override;
  void OnPropertiesChanged(const lynx::pub::LynxValue& attrs,
                           const lynx::pub::LynxValue& events) override;
  void OnMotionEvent(native_view_motion_event_t* event) override;
  void OnFocusChanged(bool focused, bool is_leaf) override;
  void OnMethodInvoked(
      const char* method, const lynx::pub::LynxValue& attrs,
      std::function<void(int, lynx::pub::LynxValue&&)> callback) override;

  bool IsScrollEnabled() override { return true; }
  bool IsSurfaceEnabled() override { return use_osr_; }

  void SetNativeView(cef_window_handle_t native_view) {
    native_view_ = native_view;
  }

  bool UseOSR() const { return use_osr_; }

  bool EnableDevtool() const { return enable_devtool_; }

  virtual float GetPixelRatio() const { return 1.0f; }

  virtual void SetupClient() {}

  virtual void OnMouseWheelEvent(int x, int y, int modifiers, double delta_x,
                                 double delta_y) {}
  virtual void OnMouseMoveEvent(int x, int y, int modifiers, bool mouse_leave) {
  }
  virtual void OnMouseClickEvent(int x, int y, int buttons, bool mouse_up) {}

  void RegisterIMEHandler(void* handler, void* opaque);

  lynx_view_t* GetLynxView() const { return lynx_view_; }

  CefRefPtr<CefBrowser> GetBrowser() { return browser_; }

  void SetClosing(bool closing) { closing_ = closing; }
  bool IsClosing() const { return closing_; }

 protected:
  friend CEFWebviewClient;
  // The client_ will be managered by CefBrowser.
  CEFWebviewClient* client_ = nullptr;
  CefRefPtr<CefBrowser> browser_;
  lynx_view_t* lynx_view_;
  std::vector<SetCookieMethod> set_cookie_methods_;
  cef_window_handle_t native_view_ = nullptr;
  bool use_osr_ = false;
  int fps_ = 30;
  bool enable_devtool_ = false;
  std::string url_;
  std::string init_js_;
  bool closing_ = false;
};

}  // namespace embedder
}  // namespace plugin
}  // namespace lynx

LYNX_EXTERN_C_BEGIN
LYNX_CAPI_EXPORT lynx_native_view_t* cef_webview_create_view(void* opaque);
LYNX_EXTERN_C_END

#endif  // PLATFORM_EMBEDDER_PLUGIN_CEF_SRC_CEF_WEBVIEW_H_
