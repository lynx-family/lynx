// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxDevToolSetModule.h"
#import <Lynx/LynxBaseInspectorOwner.h>
#import <Lynx/LynxBaseInspectorOwnerNG.h>
#import <Lynx/LynxContext+Internal.h>
#import <Lynx/LynxEnv.h>
#import <Lynx/LynxEnvKey.h>
#import "LynxDevtoolEnv.h"

@implementation LynxDevToolSetModule {
  __weak LynxContext *context_;
}

+ (NSString *)name {
  return @"LynxDevToolSetModule";
}

+ (NSDictionary<NSString *, NSString *> *)methodLookup {
  return @{
    @"isLynxDebugEnabled" : NSStringFromSelector(@selector(isLynxDebugEnabled)),
    @"switchLynxDebug" : NSStringFromSelector(@selector(switchLynxDebug:)),
    @"isDevToolEnabled" : NSStringFromSelector(@selector(isDevToolEnabled)),
    @"switchDevTool" : NSStringFromSelector(@selector(switchDevTool:)),
    @"isLogBoxEnabled" : NSStringFromSelector(@selector(isLogBoxEnabled)),
    @"switchLogBox" : NSStringFromSelector(@selector(switchLogBox:)),
    @"isHighlightTouchEnabled" : NSStringFromSelector(@selector(isHighlightTouchEnabled)),
    @"switchHighlightTouch" : NSStringFromSelector(@selector(switchHighlightTouch:)),
    @"switchLaunchRecord" : NSStringFromSelector(@selector(switchLaunchRecord:)),
    @"isLaunchRecord" : NSStringFromSelector(@selector(isLaunchRecord)),
    @"enableDomTree" : NSStringFromSelector(@selector(enableDomTree:)),
    @"isDomTreeEnabled" : NSStringFromSelector(@selector(isDomTreeEnabled)),
    @"isIgnorePropErrorsEnabled" : NSStringFromSelector(@selector(isIgnorePropErrorsEnabled)),
    @"switchIgnorePropErrors" : NSStringFromSelector(@selector(switchIgnorePropErrors:)),
    @"isQuickjsDebugEnabled" : NSStringFromSelector(@selector(isQuickjsDebugEnabled)),
    @"switchQuickjsDebug" : NSStringFromSelector(@selector(switchQuickjsDebug:)),
#if OS_IOS
    @"isLongPressMenuEnabled" : NSStringFromSelector(@selector(isLongPressMenuEnabled)),
    @"switchLongPressMenu" : NSStringFromSelector(@selector(switchLongPressMenu:)),
#endif
  };
}

- (instancetype)initWithLynxContext:(LynxContext *)context {
  self = [super init];
  if (self) {
    context_ = context;
  }
  return self;
}

- (BOOL)isLynxDebugEnabled {
  return [LynxEnv.sharedInstance lynxDebugEnabled];
}

- (void)switchLynxDebug:(BOOL)arg {
  LynxEnv.sharedInstance.lynxDebugEnabled = arg;
}

- (BOOL)isDevToolEnabled {
  return [LynxEnv.sharedInstance devtoolEnabled];
}

- (void)switchDevTool:(BOOL)arg {
  LynxEnv.sharedInstance.devtoolEnabled = arg;
}

- (BOOL)isLogBoxEnabled {
  return [LynxEnv.sharedInstance logBoxEnabled];
}

- (void)switchLogBox:(BOOL)arg {
  LynxEnv.sharedInstance.logBoxEnabled = arg;
}

- (BOOL)isLaunchRecord {
  return [LynxEnv.sharedInstance launchRecordEnabled];
}

- (void)switchLaunchRecord:(BOOL)arg {
  LynxEnv.sharedInstance.launchRecordEnabled = arg;
}

- (BOOL)isDomTreeEnabled {
  return [LynxDevtoolEnv.sharedInstance domTreeEnabled];
}

- (void)enableDomTree:(BOOL)arg {
  [LynxDevtoolEnv.sharedInstance setDomTreeEnabled:arg];
}

- (BOOL)isIgnorePropErrorsEnabled {
  return [LynxDevtoolEnv.sharedInstance get:SP_KEY_ENABLE_IGNORE_ERROR_CSS withDefaultValue:NO];
}

- (void)switchIgnorePropErrors:(BOOL)arg {
  [LynxDevtoolEnv.sharedInstance set:arg forKey:SP_KEY_ENABLE_IGNORE_ERROR_CSS];
}

- (BOOL)isQuickjsDebugEnabled {
  return [LynxDevtoolEnv.sharedInstance quickjsDebugEnabled];
}

- (void)switchQuickjsDebug:(BOOL)arg {
  [LynxDevtoolEnv.sharedInstance setQuickjsDebugEnabled:arg];
}

#if OS_IOS
- (BOOL)isLongPressMenuEnabled {
  return [LynxDevtoolEnv.sharedInstance longPressMenuEnabled];
}

- (void)switchLongPressMenu:(BOOL)arg {
  [LynxDevtoolEnv.sharedInstance setLongPressMenuEnabled:arg];
}

- (BOOL)isHighlightTouchEnabled {
  return [LynxEnv.sharedInstance highlightTouchEnabled];
}

- (void)switchHighlightTouch:(BOOL)arg {
  LynxEnv.sharedInstance.highlightTouchEnabled = arg;
}
#endif

@end
