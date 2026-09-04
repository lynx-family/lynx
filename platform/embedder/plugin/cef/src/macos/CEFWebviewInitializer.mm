// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Cocoa/Cocoa.h>
#include <dlfcn.h>
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

namespace {

struct CEFBundlePaths {
  NSString *framework_path;
  NSString *framework_executable_path;
  NSString *helper_bundle_path;
  NSString *helper_executable_path;
};

NSString *GetModuleDirectory() {
  Dl_info info = {};
  if (dladdr(reinterpret_cast<const void *>(&cef_extension_module_initialize), &info) == 0 ||
      info.dli_fname == nullptr) {
    return nil;
  }
  NSString *module_path = [NSString stringWithUTF8String:info.dli_fname];
  return [module_path stringByDeletingLastPathComponent];
}

bool ResolveCEFBundlePaths(CEFBundlePaths *paths) {
  NSString *module_directory = GetModuleDirectory();
  NSString *main_bundle_path = NSBundle.mainBundle.bundlePath;
  NSMutableArray<NSString *> *framework_roots = [NSMutableArray arrayWithCapacity:2];
  if (module_directory != nil) {
    [framework_roots addObject:[module_directory stringByAppendingPathComponent:@"frameworks"]];
  }
  if (main_bundle_path != nil) {
    [framework_roots
        addObject:[main_bundle_path stringByAppendingPathComponent:@"Contents/Frameworks"]];
  }

  NSFileManager *file_manager = NSFileManager.defaultManager;
  NSString *helper_name = [NSString stringWithUTF8String:CEF_WEBVIEW_HELPER_OUTPUT_NAME];
  for (NSString *framework_root in framework_roots) {
    NSString *framework_path =
        [framework_root stringByAppendingPathComponent:@"Chromium Embedded Framework.framework"];
    NSString *framework_executable_path =
        [framework_path stringByAppendingPathComponent:@"Chromium Embedded Framework"];
    NSString *helper_bundle_path = [framework_root
        stringByAppendingPathComponent:[helper_name stringByAppendingPathExtension:@"app"]];
    NSString *helper_executable_path =
        [[helper_bundle_path stringByAppendingPathComponent:@"Contents/MacOS"]
            stringByAppendingPathComponent:helper_name];
    if ([file_manager isExecutableFileAtPath:framework_executable_path] &&
        [file_manager isExecutableFileAtPath:helper_executable_path]) {
      paths->framework_path = framework_path;
      paths->framework_executable_path = framework_executable_path;
      paths->helper_bundle_path = helper_bundle_path;
      paths->helper_executable_path = helper_executable_path;
      return true;
    }
  }
  return false;
}

}  // namespace

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
  CEFBundlePaths paths = {};
  if (!ResolveCEFBundlePaths(&paths)) {
    fprintf(stderr, "Failed to locate the CEF framework and helper app.\n");
    return false;
  }
  if (!cef_load_library(paths.framework_executable_path.fileSystemRepresentation)) {
    fprintf(stderr, "Failed to load the CEF framework from %s.\n",
            paths.framework_executable_path.fileSystemRepresentation);
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
  settings.windowless_rendering_enabled = true;
  CefString(&settings.framework_dir_path) = paths.framework_path.fileSystemRepresentation;
  CefString(&settings.main_bundle_path) = paths.helper_bundle_path.fileSystemRepresentation;
  CefString(&settings.browser_subprocess_path) =
      paths.helper_executable_path.fileSystemRepresentation;

  CefRefPtr<CEFWebviewApp> app(new CEFWebviewApp);

  if (!CefInitialize(main_args, settings, app.get(), nullptr)) {
    cef_unload_library();
    return false;
  }

  return true;
}
