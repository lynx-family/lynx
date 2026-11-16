// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "ExplorerModule.h"
#import <Lynx/LynxLog.h>
#import "../LynxExplorerSwiftInterop.h"
#import "LynxSettingManager.h"

@implementation ExplorerModule

- (instancetype)init {
  if (self = [super init]) {
    LLogInfo(@"ExplorerModule alloc");
  }
  return self;
}

- (void)dealloc {
  LLogInfo(@"ExplorerModule dealloc");
}

+ (NSString *)name {
  return @"ExplorerModule";
}

+ (NSDictionary<NSString *, NSString *> *)methodLookup {
  return @{
    @"openSchema" : NSStringFromSelector(@selector(openSchema:)),
    @"openRoute" : NSStringFromSelector(@selector(openRoute:container:callback:)),
    @"openScan" : NSStringFromSelector(@selector(openScan)),
    @"navigateBack" : NSStringFromSelector(@selector(navigateBack:)),
    @"setThreadMode" : NSStringFromSelector(@selector(setThreadMode:)),
    @"switchPreSize" : NSStringFromSelector(@selector(switchPreSize:)),
    @"getSettingInfo" : NSStringFromSelector(@selector(getSettingInfo)),
    @"saveThemePreferences" : NSStringFromSelector(@selector(saveThemePreferences:value:)),
    @"saveToLocalStorage" : NSStringFromSelector(@selector(saveToLocalStorage:value:)),
    @"readFromLocalStorage" : NSStringFromSelector(@selector(readFromLocalStorage:)),
  };
}

- (void)openSchema:(NSString *)url {
  LLogInfo(@"openSchema %@", url);
  LXRouteCoordinator *coordinator = [LXRouteCoordinator currentBridge];
  if (coordinator == nil) {
    LLogError(@"Unable to open route: route coordinator unavailable");
    return;
  }
  [coordinator openModuleURL:url
                   container:@"automatic"
                    callback:^(id payload) {
                      NSDictionary *result =
                          [payload isKindOfClass:NSDictionary.class] ? payload : nil;
                      if (![result[@"success"] boolValue]) {
                        LLogError(@"Unable to open route: %@", result[@"message"]);
                      }
                    }];
}

- (void)openRoute:(NSString *)url
        container:(NSString *)container
         callback:(LynxCallbackBlock)callback {
  LLogInfo(@"openRoute %@", url);
  LXRouteCoordinator *coordinator = [LXRouteCoordinator currentBridge];
  if (coordinator == nil) {
    callback(@{
      @"success" : @NO,
      @"code" : @"coordinator_unavailable",
      @"message" : @"The route coordinator is unavailable.",
    });
    return;
  }
  [coordinator openModuleURL:url container:container callback:callback];
}

- (void)openScan {
  LXRouteCoordinator *coordinator = [LXRouteCoordinator currentBridge];
  if (coordinator == nil) {
    LLogError(@"Unable to open scanner: route coordinator unavailable");
    return;
  }
  [coordinator presentScannerWithCallback:^(id payload) {
    NSDictionary *result = [payload isKindOfClass:NSDictionary.class] ? payload : nil;
    if (![result[@"success"] boolValue]) {
      LLogError(@"Unable to open scanner: %@", result[@"message"]);
    }
  }];
}

- (void)navigateBack:(LynxCallbackBlock)callback {
  LXRouteCoordinator *coordinator = [LXRouteCoordinator currentBridge];
  if (coordinator == nil) {
    callback(@{
      @"success" : @NO,
      @"code" : @"coordinator_unavailable",
      @"message" : @"The route coordinator is unavailable.",
    });
    return;
  }
  [coordinator closeAnimated:YES callback:callback];
}

- (void)setThreadMode:(LynxThreadStrategyForRender)arg {
  LynxSettingManager *manager = [LynxSettingManager sharedDataHandler];
  manager.threadStrategy = arg;
}

- (void)switchPreSize:(BOOL)arg {
  LynxSettingManager *manager = [LynxSettingManager sharedDataHandler];
  [manager setPresetWidthAndHeightStatus:arg];
}

- (NSDictionary *)getSettingInfo {
  LynxSettingManager *manager = [LynxSettingManager sharedDataHandler];
  return @{
    @"threadMode" : @(manager.threadStrategy),
    @"preSize" : @(manager.isPresetWidthAndHeightOn),
    @"enableRenderNode" : @(NO),
  };
}

- (void)saveThemePreferences:(NSString *)theme value:(NSString *)value {
  NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
  [defaults setObject:value forKey:theme];
  [defaults synchronize];
  if ([theme isEqualToString:@"preferredTheme"]) {
    dispatch_async(dispatch_get_main_queue(), ^{
      [[NSNotificationCenter defaultCenter]
          postNotificationName:@"ExplorerThemePreferenceDidChange"
                        object:nil
                      userInfo:@{@"preference" : value ?: @"Auto"}];
    });
  }
}

- (void)saveToLocalStorage:(NSString *)key value:(NSString *)value {
  NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
  [defaults setObject:value forKey:key];
  [defaults synchronize];
}

- (NSString *)readFromLocalStorage:(NSString *)key {
  NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
  return [defaults objectForKey:key];
}

@end
