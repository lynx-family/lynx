// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "explorer/darwin/macos/lynx_explorer/LynxExplorer/module/LynxDemoModule.h"
#import "../LynxWindow.h"
#import "../LynxWindowController.h"
#import "../ViewController.h"
#ifdef USE_WEAK_SUFFIX_NAPI
#include "third_party/weak-node-api/headers/weak_napi_defines.h"
#endif

// LynxTestModule, prefer to use C++ Wrapper.
napi_value openSchema(napi_env env, napi_callback_info info) {
  // unwrap the url param.
  size_t argc = 1;
  napi_value argv[1];
  napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
  if (argc < 1) {
    return 0;
  }
  char c_url[512] = {0};
  napi_get_value_string_utf8(env, argv[0], c_url, sizeof(c_url), nullptr);

  NSString *url = [NSString stringWithUTF8String:c_url];
  dispatch_sync(dispatch_get_main_queue(), ^{
    ViewController *viewController = [[ViewController alloc] initWithUrl:url];
    LynxWindow *lynxWindow = [[LynxWindow alloc] init];
    lynxWindow.contentViewController = viewController;
    lynxWindow.accessibilityLabel = @"openWindow";
    LynxWindowController *windowController =
        [[LynxWindowController alloc] initWithWindow:lynxWindow];
    [windowController showWindow:nil];
    [lynxWindow makeKeyAndOrderFront:nil];
  });
  return 0;
}

napi_value ExplorerModuleCreator(napi_env env, napi_value exports, const char *module_name,
                                 void *opaque) {
  napi_value func;
  napi_create_function(env, "openSchema", 1, &openSchema, 0, &func);
  napi_set_named_property(env, exports, "openSchema", func);
  return exports;
}
