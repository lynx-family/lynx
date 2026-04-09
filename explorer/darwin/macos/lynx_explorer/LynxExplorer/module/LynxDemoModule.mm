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

namespace {

constexpr CGFloat kDefaultWindowWidth = 800.0;
constexpr CGFloat kDefaultWindowHeight = 600.0;
constexpr CGFloat kMinWindowSize = 200.0;

NSSize GetInitialWindowSize(NSString *url) {
  NSSize size = NSMakeSize(kDefaultWindowWidth, kDefaultWindowHeight);
  NSRange queryRange = [url rangeOfString:@".lynx.bundle?"];
  if (queryRange.location == NSNotFound) {
    return size;
  }

  NSUInteger queryStart = queryRange.location + queryRange.length;
  if (queryStart >= url.length) {
    return size;
  }

  NSString *query = [url substringFromIndex:queryStart];
  NSURLComponents *components = [NSURLComponents
      componentsWithString:[@"https://lynx.local/?" stringByAppendingString:query]];
  for (NSURLQueryItem *item in components.queryItems) {
    if ([item.name isEqualToString:@"width"]) {
      CGFloat width = item.value.doubleValue;
      if (width > 0) {
        size.width = MAX(kMinWindowSize, width);
      }
    } else if ([item.name isEqualToString:@"height"]) {
      CGFloat height = item.value.doubleValue;
      if (height > 0) {
        size.height = MAX(kMinWindowSize, height);
      }
    }
  }
  return size;
}

}  // namespace

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
    NSSize size = GetInitialWindowSize(url);
    lynxWindow.contentViewController = viewController;
    lynxWindow.accessibilityLabel = @"openWindow";
    [lynxWindow setContentSize:size];
    [lynxWindow center];
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
