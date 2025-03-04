// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "DevToolGlobalSlotIOS.h"
#import <DebugRouter/DebugRouter.h>
#import <DebugRouter/DebugRouterGlobalHandler.h>
#import <UIKit/UIKit.h>

const int GLOBAL_MESSAGE_SESSION_ID = -1;
@interface DevToolGlobalSlotIOS () <DebugRouterGlobalHandler>
@end
@implementation DevToolGlobalSlotIOS {
  std::weak_ptr<lynx::devtool::DevToolGlobalSlot> _slotPtr;
}
- (instancetype)initWithSlotPtr:(const std::shared_ptr<lynx::devtool::DevToolGlobalSlot> &)ptr {
  self = [super init];
  if (self) {
    _slotPtr = ptr;
    [[DebugRouter instance] addGlobalHandler:self];
  }
  return self;
}
- (void)sendMessage:(NSString *)message withType:(NSString *)type {
  [[DebugRouter instance] sendDataAsync:message WithType:type ForSession:GLOBAL_MESSAGE_SESSION_ID];
}

// TODO(zhoumingsong.smile) refactor, openCard should not coupled with message channel
- (void)openCard:(NSString *)url {
}
- (void)onMessage:(NSString *)message withType:(NSString *)type {
  std::shared_ptr<lynx::devtool::DevToolGlobalSlot> slotPtr = _slotPtr.lock();
  if (slotPtr && message && type) {
    slotPtr->OnMessage([type UTF8String], [message UTF8String]);
  }
}
@end
