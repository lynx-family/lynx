// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxDevToolService.h"
#import <Lynx/LynxService.h>

@LynxServiceRegister(LynxDevToolService) @implementation LynxDevToolService

#pragma mark - LynxServiceDevToolProtocol

- (id<LynxBaseInspectorOwner>)createInspectorOwnerWithLynxView:(LynxView *)lynxView {
  Class inspectorOwnerClass = NSClassFromString(@"LynxInspectorOwner");
  if (!inspectorOwnerClass) {
    return nil;
  }

  SEL initSelector = NSSelectorFromString(@"initWithLynxView:");
  if (![inspectorOwnerClass instancesRespondToSelector:initSelector]) {
    return nil;
  }

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Warc-performSelector-leaks"
  return [[inspectorOwnerClass alloc] performSelector:initSelector withObject:lynxView];
#pragma clang diagnostic pop
}

- (id<LynxBaseLogBoxProxy>)createLogBoxProxyWithLynxView:(LynxView *)lynxView {
  Class logBoxProxyClass = NSClassFromString(@"LynxLogBoxProxy");
  if (!logBoxProxyClass) {
    return nil;
  }

  SEL initSelector = NSSelectorFromString(@"initWithLynxView:");
  if (![logBoxProxyClass instancesRespondToSelector:initSelector]) {
    return nil;
  }

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Warc-performSelector-leaks"
  return [[logBoxProxyClass alloc] performSelector:initSelector withObject:lynxView];
#pragma clang diagnostic pop
}

- (Class<LynxContextModule>)devtoolSetModuleClass {
  return NSClassFromString(@"LynxDevToolSetModule");
}

- (Class<LynxContextModule>)devtoolWebSocketModuleClass {
  return NSClassFromString(@"LynxWebSocketModule");
}

- (Class<LynxContextModule>)devtoolTrailModuleClass {
  return NSClassFromString(@"LynxTrailModule");
}

- (nullable Class<LynxBaseInspectorOwner>)inspectorOwnerClass {
  return NSClassFromString(@"LynxInspectorOwner");
}

- (Class<LynxDebuggerProtocol>)debuggerBridgeClass {
  return NSClassFromString(@"LynxDebugBridge");
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

- (void)devtoolEnvPrepareWithConfig:(LynxConfig *)lynxConfig {
  id sharedInstance = [self devtoolEnvSharedInstance];
  if (!sharedInstance) {
    return;
  }

  SEL prepareSelector = NSSelectorFromString(@"prepareConfig:");
  if ([sharedInstance respondsToSelector:prepareSelector]) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Warc-performSelector-leaks"
    [sharedInstance performSelector:prepareSelector withObject:lynxConfig];
#pragma clang diagnostic pop
  }
}

- (void)devtoolEnvSetValue:(BOOL)value forKey:(NSString *)key {
  id sharedInstance = [self devtoolEnvSharedInstance];
  if (!sharedInstance) {
    return;
  }

  SEL setSelector = NSSelectorFromString(@"set:forKey:");
  if ([sharedInstance respondsToSelector:setSelector]) {
    NSInvocation *invocation = [NSInvocation
        invocationWithMethodSignature:[[sharedInstance class]
                                          instanceMethodSignatureForSelector:setSelector]];
    [invocation setSelector:setSelector];
    [invocation setTarget:sharedInstance];
    [invocation setArgument:&value atIndex:2];
    [invocation setArgument:&key atIndex:3];
    [invocation invoke];
  }
}

- (BOOL)devtoolEnvGetValue:(NSString *)key withDefaultValue:(BOOL)value {
  id sharedInstance = [self devtoolEnvSharedInstance];
  if (!sharedInstance) {
    return value;
  }

  SEL getSelector = NSSelectorFromString(@"get:withDefaultValue:");
  if ([sharedInstance respondsToSelector:getSelector]) {
    NSInvocation *invocation = [NSInvocation
        invocationWithMethodSignature:[[sharedInstance class]
                                          instanceMethodSignatureForSelector:getSelector]];
    [invocation setSelector:getSelector];
    [invocation setTarget:sharedInstance];
    [invocation setArgument:&key atIndex:2];
    [invocation setArgument:&value atIndex:3];
    [invocation invoke];

    BOOL result;
    [invocation getReturnValue:&result];
    return result;
  }
  return value;
}

- (void)devtoolEnvSet:(NSSet *)newGroupValues forGroup:(NSString *)groupKey {
  id sharedInstance = [self devtoolEnvSharedInstance];
  if (!sharedInstance) {
    return;
  }

  SEL setSelector = NSSelectorFromString(@"set:forGroup:");

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Warc-performSelector-leaks"
  if ([sharedInstance respondsToSelector:setSelector]) {
    [sharedInstance performSelector:setSelector withObject:newGroupValues withObject:groupKey];
  }
#pragma clang diagnostic pop
}

- (NSSet *)devtoolEnvGetGroup:(NSString *)groupKey {
  id sharedInstance = [self devtoolEnvSharedInstance];
  if (!sharedInstance) {
    return nil;
  }

  SEL getGroupSelector = NSSelectorFromString(@"getGroup:");

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Warc-performSelector-leaks"
  if ([sharedInstance respondsToSelector:getGroupSelector]) {
    return [sharedInstance performSelector:getGroupSelector withObject:groupKey];
  }
#pragma clang diagnostic pop
  return nil;
}

#pragma mark - LynxServiceProtocol

+ (LynxServiceScope)serviceScope {
  return LynxServiceScopeDefault;
}

+ (NSUInteger)serviceType {
  return kLynxServiceDevTool;
}

+ (NSString *)serviceBizID {
  return DEFAULT_LYNX_SERVICE;
}

@end
