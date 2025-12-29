// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Cocoa/Cocoa.h>
#include "include/capi/cef_app_capi.h"
#include "include/cef_app.h"
#include "include/cef_application_mac.h"
#include "include/cef_client.h"
#include "include/cef_command_line.h"
#include "include/wrapper/cef_helpers.h"
#include "include/wrapper/cef_library_loader.h"
#include "include/wrapper/cef_message_router.h"
#include "platform/embedder/plugin/cef/include/cef_extension_module_creator.h"

constexpr int64_t max_delay_ms = 10;

class CEFWebviewApp : public CefApp,
                      public CefRenderProcessHandler,
                      public CefBrowserProcessHandler {
 public:
  // CefApp
  CefRefPtr<CefRenderProcessHandler> GetRenderProcessHandler() override { return this; }
  CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() override { return this; }

  // CefRenderProcessHandler
  void OnWebKitInitialized() override {
    CefMessageRouterConfig config;
    message_router_ = CefMessageRouterRendererSide::Create(config);
  }
  void OnContextCreated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
                        CefRefPtr<CefV8Context> context) override {
    message_router_->OnContextCreated(browser, frame, context);
  }
  void OnContextReleased(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
                         CefRefPtr<CefV8Context> context) override {
    message_router_->OnContextReleased(browser, frame, context);
  }
  bool OnProcessMessageReceived(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
                                CefProcessId source_process,
                                CefRefPtr<CefProcessMessage> message) override {
    return message_router_->OnProcessMessageReceived(browser, frame, source_process, message);
  }

  // CefBrowserProcessHandler
  void OnScheduleMessagePumpWork(int64_t delay_ms) override {
    dispatch_async(dispatch_get_main_queue(), ^{
      OnScheduleWorkMainThread(delay_ms);
    });
  }

 private:
  void OnScheduleWorkMainThread(int64_t delay_ms) {
    if (IsWorkPending()) {
      Stop();
    }
    if (delay_ms <= 0) {
      DoWork();
      return;
    }
    if (delay_ms > max_delay_ms) {
      delay_ms = max_delay_ms;
    }
    Schedule(delay_ms);
  }

  bool IsWorkPending() { return _timer != nil; }

  void Stop() {
    if (_timer != nil) {
      [_timer invalidate];
      _timer = nil;
    }
  }

  void DoWork() {
    CefDoMessageLoopWork();
    Schedule(max_delay_ms);
  }

  void Schedule(int64_t delay) {
    auto max_delay = (double)delay;
    _timer = [NSTimer scheduledTimerWithTimeInterval:max_delay / 1000
                                             repeats:YES
                                               block:^(NSTimer *t) {
                                                 Stop();
                                                 DoWork();
                                               }];
  }

  NSTimer *_timer = nil;
  CefRefPtr<CefMessageRouterRendererSide> message_router_;
  IMPLEMENT_REFCOUNTING(CEFWebviewApp);
};

LYNX_EXTERN_C bool cef_extension_module_initialize() {
  CefScopedLibraryLoader library_loader;
  if (!library_loader.LoadInMain()) {
    return false;
  }

  int argc = 1;
  const char *argv[] = {"", NULL};
  CefMainArgs main_args(argc, const_cast<char **>(argv));

  CefRefPtr<CefCommandLine> command_line = CefCommandLine::CreateCommandLine();
  command_line->InitFromArgv(argc, argv);

  CefSettings settings;
  settings.external_message_pump = true;
  settings.no_sandbox = true;

  CefRefPtr<CEFWebviewApp> app(new CEFWebviewApp);

  if (!CefInitialize(main_args, settings, app.get(), nullptr)) {
    return false;
  }

  return true;
}
