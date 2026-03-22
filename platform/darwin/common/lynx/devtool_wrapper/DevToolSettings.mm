// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/DevToolSettings.h>
#include "core/renderer/utils/lynx_env.h"

@interface DevToolSettings ()

@property(nonatomic, strong) NSUserDefaults *defaults;

@end

@implementation DevToolSettings {
  // Member variables for non-persisted settings
  BOOL _highlightTouchEnabled;
  BOOL _perfMetricsEnabled;
  BOOL _previewScreenshotEnabled;
}

+ (instancetype)sharedInstance {
  static DevToolSettings *_instance = nil;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    _instance = [[DevToolSettings alloc] init];
  });
  return _instance;
}

- (instancetype)init {
  self = [super init];
  if (self) {
    _defaults = [NSUserDefaults standardUserDefaults];

    // Initialize default values for non-persisted settings
    _highlightTouchEnabled = NO;
    _perfMetricsEnabled = NO;
    _previewScreenshotEnabled = YES;

    [self syncToNative];
  }
  return self;
}

#pragma mark - Properties

- (BOOL)devToolEnabled {
  /*!
   devToolEnabled
   @note Persistence: YES
   @note Sync to native: YES
   @note Default: NO
   */
  return [self boolForKey:SP_KEY_ENABLE_DEVTOOL defaultValue:NO];
}

- (void)setDevToolEnabled:(BOOL)devToolEnabled {
  [self setBool:devToolEnabled forKey:SP_KEY_ENABLE_DEVTOOL];
  [self syncToNative:SP_KEY_ENABLE_DEVTOOL value:devToolEnabled];
}

- (BOOL)logBoxEnabled {
  /*!
   logBoxEnabled
   @note Persistence: YES
   @note Sync to native: YES
   @note Default: YES
   */
  return [self boolForKey:SP_KEY_ENABLE_LOGBOX defaultValue:YES];
}

- (void)setLogBoxEnabled:(BOOL)logBoxEnabled {
  [self setBool:logBoxEnabled forKey:SP_KEY_ENABLE_LOGBOX];
  [self syncToNative:SP_KEY_ENABLE_LOGBOX value:logBoxEnabled];
}

- (BOOL)highlightTouchEnabled {
  /*!
   highlightTouchEnabled
   @note Persistence: NO
   @note Sync to native: NO
   @note Default: NO
   */
  return _highlightTouchEnabled;
}

- (void)setHighlightTouchEnabled:(BOOL)highlightTouchEnabled {
  _highlightTouchEnabled = highlightTouchEnabled;
}

- (BOOL)launchRecordEnabled {
  /*!
   launchRecordEnabled
   @note Persistence: YES
   @note Sync to native: NO
   @note Default: NO
   */
  return [self boolForKey:SP_KEY_ENABLE_LAUNCH_RECORD defaultValue:NO];
}

- (void)setLaunchRecordEnabled:(BOOL)launchRecordEnabled {
  [self setBool:launchRecordEnabled forKey:SP_KEY_ENABLE_LAUNCH_RECORD];
}

- (BOOL)quickjsDebugEnabled {
  /*!
   quickjsDebugEnabled
   @note Persistence: YES
   @note Sync to native: YES
   @note Default: YES
   */
  return [self boolForKey:SP_KEY_ENABLE_QUICKJS_DEBUG defaultValue:YES];
}

- (void)setQuickjsDebugEnabled:(BOOL)quickjsDebugEnabled {
  [self setBool:quickjsDebugEnabled forKey:SP_KEY_ENABLE_QUICKJS_DEBUG];
  [self syncToNative:SP_KEY_ENABLE_QUICKJS_DEBUG value:quickjsDebugEnabled];
}

- (BOOL)domTreeEnabled {
  /*!
   domTreeEnabled
   @note Persistence: YES
   @note Sync to native: YES
   @note Default: YES
   */
  return [self boolForKey:SP_KEY_ENABLE_DOM_TREE defaultValue:YES];
}

- (void)setDomTreeEnabled:(BOOL)domTreeEnabled {
  [self setBool:domTreeEnabled forKey:SP_KEY_ENABLE_DOM_TREE];
  [self syncToNative:SP_KEY_ENABLE_DOM_TREE value:domTreeEnabled];
}

- (BOOL)perfMetricsEnabled {
  /*!
   perfMetricsEnabled
   @note Persistence: NO
   @note Sync to native: NO
   @note Default: NO
   */
  return _perfMetricsEnabled;
}

- (void)setPerfMetricsEnabled:(BOOL)perfMetricsEnabled {
  _perfMetricsEnabled = perfMetricsEnabled;
}

- (BOOL)fspScreenshotEnabled {
  /*!
   fspScreenshotEnabled
   @note Persistence: YES
   @note Sync to native: NO
   @note Default: NO
   */
  return [self boolForKey:SP_KEY_ENABLE_FSP_SCREENSHOT defaultValue:NO];
}

- (void)setFspScreenshotEnabled:(BOOL)fspScreenshotEnabled {
  [self setBool:fspScreenshotEnabled forKey:SP_KEY_ENABLE_FSP_SCREENSHOT];
}

- (BOOL)longPressMenuEnabled {
  /*!
   longPressMenuEnabled
   @note Persistence: YES
   @note Sync to native: NO
   @note Default: YES
   */
  return [self boolForKey:SP_KEY_ENABLE_LONG_PRESS_MENU defaultValue:YES];
}

- (void)setLongPressMenuEnabled:(BOOL)longPressMenuEnabled {
  [self setBool:longPressMenuEnabled forKey:SP_KEY_ENABLE_LONG_PRESS_MENU];
}

- (BOOL)previewScreenshotEnabled {
  /*!
   previewScreenshotEnabled
   @note Persistence: NO
   @note Sync to native: NO
   @note Default: YES
   */
  return _previewScreenshotEnabled;
}

- (void)setPreviewScreenshotEnabled:(BOOL)previewScreenshotEnabled {
  _previewScreenshotEnabled = previewScreenshotEnabled;
}

#pragma mark - Helper Methods

- (BOOL)boolForKey:(NSString *)key defaultValue:(BOOL)defaultValue {
  if ([_defaults objectForKey:key]) {
    return [_defaults boolForKey:key];
  }
  return defaultValue;
}

- (void)setBool:(BOOL)value forKey:(NSString *)key {
  [_defaults setBool:value forKey:key];
}

- (void)syncToNative:(NSString *)key value:(BOOL)value {
  lynx::tasm::LynxEnv::GetInstance().SetBoolLocalEnv([key UTF8String], value ? true : false);
}

- (void)syncToNative {
  [self syncToNative:SP_KEY_ENABLE_DEVTOOL value:self.devToolEnabled];
  [self syncToNative:SP_KEY_ENABLE_LOGBOX value:self.logBoxEnabled];
  [self syncToNative:SP_KEY_ENABLE_QUICKJS_DEBUG value:self.quickjsDebugEnabled];
  [self syncToNative:SP_KEY_ENABLE_DOM_TREE value:self.domTreeEnabled];
}

@end
