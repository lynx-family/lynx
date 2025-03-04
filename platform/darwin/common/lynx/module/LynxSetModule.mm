// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxSetModule.h"
#import "LynxBaseInspectorOwner.h"
#import "LynxBaseInspectorOwnerNG.h"
#import "LynxContext+Internal.h"
#import "LynxEnv+Internal.h"
#import "LynxEnv.h"
#import "LynxEnvKey.h"

#if OS_IOS
#import "LynxUIOwner.h"
#endif

@implementation LynxSetModule {
  __weak LynxContext *context_;
}

+ (NSString *)name {
  return @"LynxSetModule";
}

+ (NSDictionary<NSString *, NSString *> *)methodLookup {
  return @{
    @"switchKeyBoardDetect" : NSStringFromSelector(@selector(switchKeyBoardDetect:)),
    @"getLogToSystemStatus" : NSStringFromSelector(@selector(getLogToSystemStatus)),
    @"switchLogToSystem" : NSStringFromSelector(@selector(switchLogToSystem:)),
    @"isAutomationEnabled" : NSStringFromSelector(@selector(isAutomationEnabled)),
    @"switchAutomation" : NSStringFromSelector(@selector(switchAutomation:)),
    @"getEnableLayoutOnly" : NSStringFromSelector(@selector(getEnableLayoutOnly)),
    @"switchEnableLayoutOnly" : NSStringFromSelector(@selector(switchEnableLayoutOnly:)),
    @"getAutoResumeAnimation" : NSStringFromSelector(@selector(getAutoResumeAnimation)),
    @"setAutoResumeAnimation" : NSStringFromSelector(@selector(setAutoResumeAnimation:)),
    @"getEnableNewTransformOrigin" : NSStringFromSelector(@selector(getEnableNewTransformOrigin)),
    @"setEnableNewTransformOrigin" : NSStringFromSelector(@selector(setEnableNewTransformOrigin:)),
    @"getAllSettings" : NSStringFromSelector(@selector(getAllSettings:callback:)),
  };
}

- (instancetype)initWithLynxContext:(LynxContext *)context {
  self = [super init];
  if (self) {
    context_ = context;
  }
  return self;
}

// Do nothing to align with Android
- (void)switchKeyBoardDetect:(BOOL)arg {
  return;
}

- (NSNumber *)getLogToSystemStatus {
  return @(NO);
}

- (void)switchLogToSystem:(BOOL)arg {
}

- (BOOL)isAutomationEnabled {
  return [LynxEnv.sharedInstance automationEnabled];
}

- (void)switchAutomation:(BOOL)arg {
  [LynxEnv.sharedInstance setAutomationEnabled:arg];
}

- (BOOL)getEnableLayoutOnly {
  return [LynxEnv.sharedInstance getEnableLayoutOnly];
}

- (void)switchEnableLayoutOnly:(BOOL)arg {
  [LynxEnv.sharedInstance setEnableLayoutOnly:arg];
}

- (BOOL)getAutoResumeAnimation {
  return [LynxEnv.sharedInstance getAutoResumeAnimation];
}

- (void)setAutoResumeAnimation:(BOOL)arg {
  [LynxEnv.sharedInstance setAutoResumeAnimation:arg];
}

- (BOOL)getEnableNewTransformOrigin {
  return [LynxEnv.sharedInstance getEnableNewTransformOrigin];
}

- (void)setEnableNewTransformOrigin:(BOOL)arg {
  [LynxEnv.sharedInstance setEnableNewTransformOrigin:arg];
}

- (void)getAllSettings:(NSDictionary *)params callback:(LynxCallbackBlock)callback {
  callback([self setUpSettingsDict]);
}

- (NSMutableDictionary *)setUpSettingsDict {
  static NSArray<NSNumber *> *array;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    NSMutableArray<NSNumber *> *mutableArray = [NSMutableArray array];

    for (int key = (int)LynxEnvSwitchRunloopThread; key < (int)LynxEnvKeyEndMark; key++) {
      [mutableArray addObject:@(key)];
    }
    array = [mutableArray copy];
  });

  NSMutableDictionary *dict = [NSMutableDictionary new];

  [array enumerateObjectsUsingBlock:^(NSNumber *_Nonnull obj, NSUInteger idx, BOOL *_Nonnull stop) {
    NSString *key = [LynxEnv _keyStringFromType:(LynxEnvKey)[obj intValue]];
    NSString *value = [[LynxEnv sharedInstance] _stringFromExternalEnv:key];
    if (!value) {
      return;
    }
    [dict setObject:value forKey:key];
  }];

  return dict;
}

@end
