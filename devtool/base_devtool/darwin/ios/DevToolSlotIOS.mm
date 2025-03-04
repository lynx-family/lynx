// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "DevToolSlotIOS.h"
#import <DebugRouter/DebugRouterSlot.h>
@interface DevToolSlotIOS () <DebugRouterSlotDelegate>

@end

@implementation DevToolSlotIOS {
  DebugRouterSlot *_debug_router_slot;
  std::weak_ptr<lynx::devtool::DevToolSlot> _slotPtr;
  NSString *_template_url;
}
- (instancetype)initWithSlotPtr:(const std::shared_ptr<lynx::devtool::DevToolSlot> &)ptr {
  self = [super init];
  if (self) {
    _slotPtr = ptr;
    _debug_router_slot = [[DebugRouterSlot alloc] init];
    _debug_router_slot.delegate = self;
    _template_url = @"";
  }
  return self;
}

- (NSString *)getTemplateUrl {
  return _template_url;
}
- (void)onMessage:(NSString *)message WithType:(NSString *)type {
  std::shared_ptr<lynx::devtool::DevToolSlot> slotPtr = _slotPtr.lock();
  if (slotPtr && message && type) {
    slotPtr->OnMessage([type UTF8String], [message UTF8String]);
  }
}

- (int)plug:(NSString *)url {
  _template_url = url;
  return [_debug_router_slot plug];
}
- (void)pull {
  [_debug_router_slot pull];
}
- (void)sendMessage:(NSString *)message withType:(NSString *)type {
  [_debug_router_slot sendDataAsync:message WithType:type];
}
@end
