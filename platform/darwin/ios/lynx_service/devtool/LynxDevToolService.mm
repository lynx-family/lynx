// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxService.h>
#import <LynxService/LynxDevToolService.h>
#import <objc/message.h>

@LynxServiceRegister(LynxDevToolService, LynxServiceDevToolProtocol)
    @implementation LynxDevToolService

#pragma mark - LynxServiceDevToolProtocol

- (instancetype)init {
  self = [super init];
  if (self) {
    _lynxDebugPresetValue = NO;
    _logBoxPresetValue = NO;
  }
  return self;
}

- (id<LynxBaseInspectorController>)createInspectorOwnerWithLynxView:(LynxView *)lynxView
                                                         debuggable:(BOOL)debuggable {
  Class inspectorOwnerClass = NSClassFromString(@"LynxInspectorOwner");
  if (!inspectorOwnerClass) {
    return nil;
  }

  SEL initSelector = NSSelectorFromString(@"initWithLynxView:debuggable:");
  id allocated = [inspectorOwnerClass alloc];

  if (![allocated respondsToSelector:initSelector]) {
    return nil;
  }

  id (*InitFunc)(id, SEL, id, BOOL) = (id(*)(id, SEL, id, BOOL))objc_msgSend;
  id instance = InitFunc(allocated, initSelector, lynxView, debuggable);
  return instance;
}

- (id<LynxLogBoxProtocol>)createLogBoxWithLynxView:(LynxView *)lynxView {
  Class logBoxClass = NSClassFromString(@"LynxLogBoxWrapper");
  if (!logBoxClass) {
    return nil;
  }
  SEL initSelector = NSSelectorFromString(@"initWithLynxView:");
  if (![logBoxClass instancesRespondToSelector:initSelector]) {
    return nil;
  }
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Warc-performSelector-leaks"
  return [[logBoxClass alloc] performSelector:initSelector withObject:lynxView];
#pragma clang diagnostic pop
}

- (Class<LynxContextModule>)devtoolSetModuleClass {
  return NSClassFromString(@"LynxDevToolSetModule");
}

- (Class<LynxContextModule>)lynxWebSocketModuleClass {
  return NSClassFromString(@"LynxWebSocketModule");
}

- (Class<LynxContextModule>)devtoolTrailModuleClass {
  return NSClassFromString(@"LynxTrailModule");
}

- (nullable Class<LynxBaseInspectorController>)inspectorOwnerClass {
  return NSClassFromString(@"LynxInspectorOwner");
}

- (id)debugBridgeSingleton {
  Class debugBridgeClass = NSClassFromString(@"LynxDebugBridge");
  if (!debugBridgeClass) {
    return nil;
  }

  SEL singletonSelector = NSSelectorFromString(@"singleton");
  if (![debugBridgeClass respondsToSelector:singletonSelector]) {
    return nil;
  }
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Warc-performSelector-leaks"
  return [debugBridgeClass performSelector:singletonSelector];
#pragma clang diagnostic pop
}

- (BOOL)enable:(NSURL *)url withOptions:(NSDictionary *)options {
  id debugBridgeSingleton = [self debugBridgeSingleton];
  if (!debugBridgeSingleton) {
    return NO;
  }

  SEL enableSelector = NSSelectorFromString(@"enable:withOptions:");
  if ([debugBridgeSingleton respondsToSelector:enableSelector]) {
    NSInvocation *invocation = [NSInvocation
        invocationWithMethodSignature:[[debugBridgeSingleton class]
                                          instanceMethodSignatureForSelector:enableSelector]];
    [invocation setSelector:enableSelector];
    [invocation setTarget:debugBridgeSingleton];
    [invocation setArgument:&url atIndex:2];
    [invocation setArgument:&options atIndex:3];
    [invocation invoke];

    BOOL result;
    [invocation getReturnValue:&result];
    return result;
  }
  return NO;
}

- (void)addOpenCardCallback:(LynxOpenCardCallback)callback {
  id debugBridgeSingleton = [self debugBridgeSingleton];
  if (!debugBridgeSingleton) {
    return;
  }

  SEL addOpenCardCallbackSelector = NSSelectorFromString(@"addOpenCardCallback:");
  if ([debugBridgeSingleton respondsToSelector:addOpenCardCallbackSelector]) {
    NSInvocation *invocation =
        [NSInvocation invocationWithMethodSignature:
                          [[debugBridgeSingleton class]
                              instanceMethodSignatureForSelector:addOpenCardCallbackSelector]];
    [invocation setSelector:addOpenCardCallbackSelector];
    [invocation setTarget:debugBridgeSingleton];
    [invocation setArgument:&callback atIndex:2];
    [invocation invoke];
  }
}

- (BOOL)hasSetOpenCardCallback {
  id debugBridgeSingleton = [self debugBridgeSingleton];
  if (!debugBridgeSingleton) {
    return NO;
  }

  SEL hasSetOpenCardCallbackSelector = NSSelectorFromString(@"hasSetOpenCardCallback");
  if ([debugBridgeSingleton respondsToSelector:hasSetOpenCardCallbackSelector]) {
    NSInvocation *invocation =
        [NSInvocation invocationWithMethodSignature:
                          [[debugBridgeSingleton class]
                              instanceMethodSignatureForSelector:hasSetOpenCardCallbackSelector]];
    [invocation setSelector:hasSetOpenCardCallbackSelector];
    [invocation setTarget:debugBridgeSingleton];
    [invocation invoke];

    BOOL result;
    [invocation getReturnValue:&result];
    return result;
  }
  return NO;
}

- (void)onPerfMetricsEvent:(NSString *)eventName
                  withData:(NSDictionary<NSString *, NSObject *> *)data
                instanceId:(int32_t)instanceId {
  id debugBridgeSingleton = [self debugBridgeSingleton];
  if (!debugBridgeSingleton) {
    return;
  }

  SEL onPerfMetricsEventSelector = NSSelectorFromString(@"onPerfMetricsEvent:withData:instanceId:");
  if ([debugBridgeSingleton respondsToSelector:onPerfMetricsEventSelector]) {
    NSInvocation *invocation =
        [NSInvocation invocationWithMethodSignature:
                          [[debugBridgeSingleton class]
                              instanceMethodSignatureForSelector:onPerfMetricsEventSelector]];
    [invocation setSelector:onPerfMetricsEventSelector];
    [invocation setTarget:debugBridgeSingleton];
    [invocation setArgument:&eventName atIndex:2];
    [invocation setArgument:&data atIndex:3];
    [invocation setArgument:&instanceId atIndex:4];
    [invocation invoke];
  }
}

- (void)setAppInfo:(NSDictionary *)options {
  id debugBridgeSingleton = [self debugBridgeSingleton];
  if (!debugBridgeSingleton) {
    return;
  }

  SEL setAppInfoSelector = NSSelectorFromString(@"setAppInfo:");
  if ([debugBridgeSingleton respondsToSelector:setAppInfoSelector]) {
    NSInvocation *invocation = [NSInvocation
        invocationWithMethodSignature:[[debugBridgeSingleton class]
                                          instanceMethodSignatureForSelector:setAppInfoSelector]];
    [invocation setSelector:setAppInfoSelector];
    [invocation setTarget:debugBridgeSingleton];
    [invocation setArgument:&options atIndex:2];
    [invocation invoke];
  }
}

- (id)devtoolEnvSharedInstance {
  Class envClass = NSClassFromString(@"LynxDevtoolEnv");
  if (!envClass) {
    return nil;
  }

  SEL sharedInstanceSelector = NSSelectorFromString(@"sharedInstance");
  if (![envClass respondsToSelector:sharedInstanceSelector]) {
    return nil;
  }

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Warc-performSelector-leaks"
  return [envClass performSelector:sharedInstanceSelector];
#pragma clang diagnostic pop
}

- (void)initializeDebugRouterSessions {
  Class debugRouterClass = NSClassFromString(@"DebugRouter");
  if (!debugRouterClass) {
    return;
  }

  SEL instanceSelector = NSSelectorFromString(@"instance");
  id debugRouterInstance = nil;
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Warc-performSelector-leaks"
  debugRouterInstance = [debugRouterClass performSelector:instanceSelector];
#pragma clang diagnostic pop

  if (debugRouterInstance) {
    SEL enableSessionsSelector = NSSelectorFromString(@"enableAllSessions");
    if ([debugRouterInstance respondsToSelector:enableSessionsSelector]) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Warc-performSelector-leaks"
      [debugRouterInstance performSelector:enableSessionsSelector];
#pragma clang diagnostic pop
    }
  }
}

- (void)enableAllSessions {
  [self initializeDebugRouterSessions];
}

#pragma mark - LynxServiceProtocol

@end
