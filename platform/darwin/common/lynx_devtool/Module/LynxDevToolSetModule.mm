// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/DevToolSettings.h>
#import <Lynx/LynxBaseInspectorOwner.h>
#import <Lynx/LynxContext+Internal.h>
#import <Lynx/LynxEnv.h>
#import <LynxDevtool/LynxDevToolSetModule.h>
#import <LynxDevtool/LynxDevtoolEnv.h>

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
    @"invokeCdp" : NSStringFromSelector(@selector(invokeCdp:callback:)),
    @"isFspScreenshotEnabled" : NSStringFromSelector(@selector(isFspScreenshotEnabled)),
    @"switchFspScreenshot" : NSStringFromSelector(@selector(switchFspScreenshot:)),
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
  return [[DevToolSettings sharedInstance] isCSSErrorIgnored];
}

- (void)switchIgnorePropErrors:(BOOL)arg {
  [[DevToolSettings sharedInstance] setCSSErrorIgnored:arg];
}

- (BOOL)isQuickjsDebugEnabled {
  return [LynxDevtoolEnv.sharedInstance quickjsDebugEnabled];
}

- (void)switchQuickjsDebug:(BOOL)arg {
  [LynxDevtoolEnv.sharedInstance setQuickjsDebugEnabled:arg];
}

- (BOOL)isFspScreenshotEnabled {
  return [DevToolSettings sharedInstance].fspScreenshotEnabled;
}

- (void)switchFspScreenshot:(BOOL)arg {
  [DevToolSettings sharedInstance].fspScreenshotEnabled = arg;
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

- (void)invokeCdp:(NSString *)message callback:(LynxCallbackBlock)callback {
  if (context_ == nil || [context_ getLynxView] == nil) {
    return;
  }
  id<LynxBaseInspectorOwner> owner = [context_ getLynxView].baseInspectorOwner;
  if (owner == nil) {
    return;
  }
  [owner invokeCDPFromSDK:message
             withCallback:^(NSString *response) {
               __strong typeof(context_) strongContext = context_;
               if (strongContext == nil || callback == nil) {
                 return;
               }
               [strongContext runOnJSThread:^() {
                 callback(response);
               }];
             }];
}

@end
