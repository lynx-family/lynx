// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/DevToolSettings.h>
#import <Lynx/LynxEnv.h>
#import <Lynx/LynxLog.h>
#import <Lynx/LynxUIKitAPIAdapter.h>
#import <LynxDevtool/LynxDevtoolEnv.h>

#include <LynxDevtool/LynxDebugBridge.h>
#include "core/renderer/utils/devtool_lifecycle.h"
#include "core/renderer/utils/lynx_env.h"

@implementation LynxDevtoolEnv

+ (instancetype)sharedInstance {
  static LynxDevtoolEnv *_instance = nil;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    _instance = [[LynxDevtoolEnv alloc] init];
  });

  return _instance;
}

- (instancetype)init {
  self = [super init];
  if (self) {
    /**
     * [self setDefaultAppInfo]
     *   -> [LynxDebugBridge singleton]
     *   -> [LynxInspectorOwner init]
     *   -> [DevtoolRuntimeManagerDarwinDelegate init]
     *   -> [LynxDevtoolEnv sharedInstance]
     *
     * NSOperationQueue is used to avoid above recursive call
     */
    [[NSOperationQueue mainQueue] addOperationWithBlock:^{
      [self setDefaultAppInfo];
    }];
    lynx::tasm::DevToolLifecycle::GetInstance().OnInitialized();
  }
  return self;
}

- (void)setDefaultAppInfo {
  NSDictionary *info = [[NSBundle mainBundle] infoDictionary];
  NSString *appName =
      info[@"CFBundleDisplayName"] ? info[@"CFBundleDisplayName"] : info[@"CFBundleName"];
  [[LynxDebugBridge singleton] setAppInfo:appName ? @{@"App" : appName} : @{}];
}

- (BOOL)isErrorTypeIgnored:(NSInteger)errCode {
  return [[DevToolSettings sharedInstance] isErrorTypeIgnored:errCode];
}

- (void)setShowDevtoolBadge:(BOOL)show __attribute__((deprecated("Deprecated after Lynx2.9"))) {
}

- (BOOL)showDevtoolBadge __attribute__((deprecated("Deprecated after Lynx2.9"))) {
  return NO;
}

- (void)setV8Enabled:(BOOL)enableV8 __attribute__((deprecated("Deprecated after Lynx3.1"))) {
}

- (BOOL)v8Enabled __attribute__((deprecated("Deprecated after Lynx3.1"))) {
  return NO;
}

- (void)setLongPressMenuEnabled:(BOOL)enableLongPressMenu {
#if OS_IOS
  [DevToolSettings sharedInstance].longPressMenuEnabled = enableLongPressMenu;
#endif
}

- (BOOL)longPressMenuEnabled {
#if OS_IOS
  return [DevToolSettings sharedInstance].longPressMenuEnabled;
#else
  return NO;
#endif
}

- (void)setDomTreeEnabled:(BOOL)enableDomTree {
  [DevToolSettings sharedInstance].domTreeEnabled = enableDomTree;
}

- (BOOL)domTreeEnabled {
  return [DevToolSettings sharedInstance].domTreeEnabled;
}

- (BOOL)previewScreenshotEnabled {
#if OS_IOS
  return [DevToolSettings sharedInstance].previewScreenshotEnabled;
#else
  return NO;
#endif
}

- (void)setQuickjsDebugEnabled:(BOOL)quickjsDebugEnabled {
  [DevToolSettings sharedInstance].quickjsDebugEnabled = quickjsDebugEnabled;
}

- (BOOL)quickjsDebugEnabled {
  return [DevToolSettings sharedInstance].quickjsDebugEnabled;
}

- (void)setPerfMetricsEnabled:(BOOL)enable {
  [DevToolSettings sharedInstance].perfMetricsEnabled = enable;
}

- (BOOL)perfMetricsEnabled {
  return [DevToolSettings sharedInstance].perfMetricsEnabled;
}

@end
