// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "include/cef_app.h"
#include "include/cef_render_process_handler.h"
#include "include/wrapper/cef_message_router.h"
#include "platform/embedder/plugin/cef/include/cef_extension_module_creator.h"

namespace {
class CEFWebviewApp : public CefApp,
                      public CefBrowserProcessHandler,
                      public CefRenderProcessHandler {
  // CefApp methods:
  CefRefPtr<CefRenderProcessHandler> GetRenderProcessHandler() override {
    return this;
  }
  CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() override {
    return this;
  }
  // CefRenderProcessHandler methods:
  void OnBrowserCreated(CefRefPtr<CefBrowser> browser,
                        CefRefPtr<CefDictionaryValue> extra_info) override {
    if (extra_info) {
      auto init_js_str = extra_info->GetString("initjs");
      if (!init_js_str.empty()) {
        auto id = browser->GetIdentifier();
        init_js_map_[id] = init_js_str.ToString();
      }
    }
  }

  void OnBrowserDestroyed(CefRefPtr<CefBrowser> browser) override {
    init_js_map_.erase(browser->GetIdentifier());
  }

  void OnWebKitInitialized() override {
    CefMessageRouterConfig config;
    message_router_ = CefMessageRouterRendererSide::Create(config);
  }

  void OnContextCreated(CefRefPtr<CefBrowser> browser,
                        CefRefPtr<CefFrame> frame,
                        CefRefPtr<CefV8Context> context) override {
    // refer to https://github.com/chromiumembedded/cef/issues/3867
    bool is_same_context = frame->GetV8Context()->IsSame(context);
    if (is_same_context) {
      std::string init_js;
      auto id = browser->GetIdentifier();
      if (init_js_map_.find(id) != init_js_map_.end()) {
        init_js = init_js_map_[id];
      }
      std::string js =
          "window.addEventListener('message', "
          "e => "
          "window.cefQuery({request: 'LyNxSig_' + e.data}));" +
          init_js;
      frame->ExecuteJavaScript(js, "<host>", 1);
    }
    message_router_->OnContextCreated(browser, frame, context);
  }

  void OnContextReleased(CefRefPtr<CefBrowser> browser,
                         CefRefPtr<CefFrame> frame,
                         CefRefPtr<CefV8Context> context) override {
    message_router_->OnContextReleased(browser, frame, context);
  }

  bool OnProcessMessageReceived(CefRefPtr<CefBrowser> browser,
                                CefRefPtr<CefFrame> frame,
                                CefProcessId source_process,
                                CefRefPtr<CefProcessMessage> message) override {
    return message_router_->OnProcessMessageReceived(browser, frame,
                                                     source_process, message);
  }

  void OnBeforeCommandLineProcessing(
      const CefString& process_type,
      CefRefPtr<CefCommandLine> command_line) override {}

 private:
  void OnRegisterCustomSchemes(
      CefRawPtr<CefSchemeRegistrar> registrar) override {}

 private:
  CefRefPtr<CefMessageRouterRendererSide> message_router_;
  std::map<int, std::string> init_js_map_;
  IMPLEMENT_REFCOUNTING(CEFWebviewApp);
};

}  // namespace

LYNX_EXTERN_C bool cef_extension_module_initialize() {
  CefMainArgs main_args(::GetModuleHandle(nullptr));
  int exit_code = CefExecuteProcess(main_args, new CEFWebviewApp, nullptr);
  if (exit_code >= 0) {
    return false;
  }
  CefSettings settings;

  settings.no_sandbox = 1;
  settings.multi_threaded_message_loop = 1;
  settings.windowless_rendering_enabled = 1;

  wchar_t locale[64];
  ::GetUserDefaultLocaleName(locale, sizeof(locale) / sizeof(locale[0]));
  CefString(&settings.locale) = locale;
  CefRefPtr<CEFWebviewApp> app(new CEFWebviewApp);
  if (!CefInitialize(main_args, settings, app.get(), nullptr)) {
    return false;
  }

  return true;
}
