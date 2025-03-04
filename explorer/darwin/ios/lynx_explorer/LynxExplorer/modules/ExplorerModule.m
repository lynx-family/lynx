// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "ExplorerModule.h"
#import <Lynx/LynxLog.h>
#import "AppDelegate.h"
#import "LynxSettingManager.h"
#import "ScanViewController.h"
#import "TasmDispatcher.h"
#import "UIHelper.h"

NSString *const DEVTOOL_SWITCH_URL = @"file://lynx?local://switchPage/devtoolSwitch.lynx.bundle";

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
    @"openScan" : NSStringFromSelector(@selector(openScan)),
    @"setThreadMode" : NSStringFromSelector(@selector(setThreadMode:)),
    @"switchPreSize" : NSStringFromSelector(@selector(switchPreSize:)),
    @"getSettingInfo" : NSStringFromSelector(@selector(getSettingInfo)),
    @"openDevtoolSwitchPage" : NSStringFromSelector(@selector(openDevToolSwitchPage)),
    @"navigateBack" : NSStringFromSelector(@selector(navigateBack)),
  };
}

- (void)openSchema:(NSString *)url {
  LLogInfo(@"openSchema %s", [url cStringUsingEncoding:[NSString defaultCStringEncoding]]);
  dispatch_async(dispatch_get_main_queue(), ^() {
    [[TasmDispatcher sharedInstance] openTargetUrl:url];
  });
}

- (void)openScan {
  dispatch_async(dispatch_get_main_queue(), ^() {
    UIViewController *vc = [UIHelper getTopViewController];
    if (vc != nil && [vc isKindOfClass:[UINavigationController class]]) {
      ScanViewController *scanVC = [ScanViewController new];
      [(UINavigationController *)vc pushViewController:scanVC animated:YES];
    }
  });
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

- (void)openDevToolSwitchPage {
  [[TasmDispatcher sharedInstance] openTargetUrl:DEVTOOL_SWITCH_URL];
}

- (void)navigateBack {
  dispatch_async(dispatch_get_main_queue(), ^() {
    UINavigationController *vc =
        ((AppDelegate *)([UIApplication sharedApplication].delegate)).navigationController;
    if (vc != nil && [vc isKindOfClass:[UINavigationController class]]) {
      [vc popViewControllerAnimated:YES];
    }
  });
}

@end
