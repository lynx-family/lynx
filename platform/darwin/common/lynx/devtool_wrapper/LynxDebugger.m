// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxDebugger+Internal.h>

#import <Lynx/LynxEnv.h>
#import <Lynx/LynxService.h>
#import <Lynx/LynxServiceDevToolProtocol.h>

@implementation LynxDebugger

+ (BOOL)enable:(NSURL *)schema withOptions:(NSDictionary *)options {
  if (![[LynxEnv sharedInstance] lynxDebugEnabled]) {
    return NO;
  }
  return [LynxService(LynxServiceDevToolProtocol) enable:schema withOptions:options];
}

+ (void)setOpenCardCallback:(LynxOpenCardCallback)callback {
  [LynxDebugger addOpenCardCallback:callback];
}

+ (void)addOpenCardCallback:(LynxOpenCardCallback)callback {
  if (![[LynxEnv sharedInstance] lynxDebugEnabled]) {
    return;
  }
  [LynxService(LynxServiceDevToolProtocol) addOpenCardCallback:callback];
}

+ (BOOL)hasSetOpenCardCallback {
  if (![[LynxEnv sharedInstance] lynxDebugEnabled]) {
    return NO;
  }
  return [LynxService(LynxServiceDevToolProtocol) hasSetOpenCardCallback];
}

+ (void)onPerfMetricsEvent:(NSString *)eventName
                  withData:(NSDictionary<NSString *, NSObject *> *)data
                instanceId:(int32_t)instanceId {
  if (![[LynxEnv sharedInstance] lynxDebugEnabled]) {
    return;
  }
  [LynxService(LynxServiceDevToolProtocol) onPerfMetricsEvent:eventName
                                                     withData:data
                                                   instanceId:instanceId];
}

+ (void)setAppInfo:(NSDictionary *)options {
  if (![[LynxEnv sharedInstance] lynxDebugEnabled]) {
    return;
  }
  [LynxService(LynxServiceDevToolProtocol) setAppInfo:options];
}

@end
