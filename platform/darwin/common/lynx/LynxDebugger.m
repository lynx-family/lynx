// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxDebugger.h"

#import <objc/message.h>

#import "LynxService.h"
#import "LynxServiceDevToolProtocol.h"

@implementation LynxDebugger

+ (Class<LynxDebuggerProtocol>)bridgeClass {
#if OS_OSX
  return NSClassFromString(@"LynxDebugBridge");
#else
  return [LynxService(LynxServiceDevToolProtocol) debuggerBridgeClass];
#endif
}

+ (BOOL)enable:(NSURL *)schema withOptions:(NSDictionary *)options {
  if ([[LynxDebugger bridgeClass] respondsToSelector:@selector(singleton)]) {
    if ([[[LynxDebugger bridgeClass] singleton] respondsToSelector:@selector(enable:
                                                                        withOptions:)]) {
      return [[[LynxDebugger bridgeClass] singleton] enable:schema withOptions:options];
    }
  }
  return NO;
}

+ (void)setOpenCardCallback:(LynxOpenCardCallback)callback {
  [LynxDebugger addOpenCardCallback:callback];
}

+ (void)addOpenCardCallback:(LynxOpenCardCallback)callback {
  if ([[LynxDebugger bridgeClass] respondsToSelector:@selector(singleton)]) {
    if ([[[LynxDebugger bridgeClass] singleton]
            respondsToSelector:@selector(setOpenCardCallback:)]) {
      [[[LynxDebugger bridgeClass] singleton] setOpenCardCallback:callback];
    }
  }
}

+ (BOOL)hasSetOpenCardCallback {
  if ([[LynxDebugger bridgeClass] respondsToSelector:@selector(singleton)]) {
    if ([[[LynxDebugger bridgeClass] singleton]
            respondsToSelector:@selector(hasSetOpenCardCallback)]) {
      return [[[LynxDebugger bridgeClass] singleton] hasSetOpenCardCallback];
    }
  }
  return NO;
}

+ (BOOL)openDebugSettingPanel {
  /*
   * This method used to get the `DebugSettingPanelManager` instance and call `openSettingPanel` on
   * OSX platform.
   * Since `DebugSettingPanelManager` has been removed, this method is no longer needed.
   */
  return NO;
}

+ (void)onPerfMetricsEvent:(NSString *)eventName
                  withData:(NSDictionary<NSString *, NSObject *> *)data
                instanceId:(int32_t)instanceId {
  static id singleton = nil;
  static BOOL hasPerfMetricsEvent = NO;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    if ([[LynxDebugger bridgeClass] respondsToSelector:@selector(singleton)]) {
      singleton = [[LynxDebugger bridgeClass] singleton];
      hasPerfMetricsEvent =
          [singleton respondsToSelector:@selector(onPerfMetricsEvent:withData:instanceId:)];
    }
  });
  if (hasPerfMetricsEvent) {
    [singleton onPerfMetricsEvent:eventName withData:data instanceId:instanceId];
  }
}

@end
