// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "ConsoleDelegateManager.h"
#import <Foundation/Foundation.h>
#import "LynxInspectorConsoleDelegate.h"
#include "devtool/lynx_devtool/js_debug/js/inspector_java_script_debugger_impl.h"

typedef void (^ConsoleObjectHandler)(NSString* detail);

@implementation ConsoleDelegateManager {
  id<LynxInspectorConsoleDelegate> _consoleDelegate;
  NSMutableDictionary* _consoleObjectHandler;

  std::weak_ptr<lynx::devtool::DevToolPlatformFacade> devtool_platform_facade_;
}

- (instancetype)initWithDevToolPlatformFacade:
    (const std::shared_ptr<lynx::devtool::DevToolPlatformFacade>&)facade {
  if (self = [super init]) {
    devtool_platform_facade_ = facade;
  }
  return self;
}

- (void)setLynxInspectorConsoleDelegate:(id)delegate {
  if ([delegate respondsToSelector:NSSelectorFromString(@"onConsoleMessage:")]) {
    _consoleDelegate = delegate;
    auto facade_sp = devtool_platform_facade_.lock();
    if (facade_sp != nullptr) {
      auto debugger = facade_sp->GetJSDebugger().lock();
      if (debugger) {
        debugger->FlushConsoleMessages();
      }
    }
  }
}

- (void)getConsoleObject:(NSString*)objectId
           needStringify:(BOOL)stringify
           resultHandler:(void (^)(NSString* _Nonnull))handler {
  if (!objectId) {
    return;
  }
  auto facade_sp = devtool_platform_facade_.lock();
  if (facade_sp) {
    auto debugger = facade_sp->GetJSDebugger().lock();
    if (!debugger) {
      return;
    }
    if (_consoleObjectHandler == nil) {
      _consoleObjectHandler = [NSMutableDictionary dictionary];
    }
    static int handlerId = 0;
    handlerId--;
    [_consoleObjectHandler setObject:handler forKey:[NSNumber numberWithInt:handlerId]];
    debugger->GetConsoleObject([objectId UTF8String], stringify, handlerId);
  }
}

- (void)onConsoleMessage:(const std::string&)message {
  [_consoleDelegate onConsoleMessage:[NSString stringWithUTF8String:message.c_str()]];
}

- (void)onConsoleObject:(const std::string&)detail callbackId:(int)callbackId {
  if (_consoleDelegate) {
    NSNumber* cid = [NSNumber numberWithInt:callbackId];
    ConsoleObjectHandler handler = [_consoleObjectHandler objectForKey:cid];
    if (handler) {
      handler([NSString stringWithUTF8String:detail.c_str()]);
      [_consoleObjectHandler removeObjectForKey:cid];
    }
  }
}

@end
