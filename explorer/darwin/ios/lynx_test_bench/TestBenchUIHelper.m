// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "TestBenchUIHelper.h"

@implementation TestBenchUIHelper
+ (UIViewController *)getTopViewController {
  //  getting rootViewController
  UIViewController *topController = [UIApplication sharedApplication].keyWindow.rootViewController;
  //  Getting topMost ViewController
  while (topController != nil) {
    if ([topController isKindOfClass:[UINavigationController class]]) {
      return topController;
    }
    topController = [topController presentedViewController];
  }
  return nil;
}

+ (UINavigationController *)getTopNavigationController {
  UIViewController *topVC = [TestBenchUIHelper getTopViewController];

  if ([topVC isKindOfClass:[UINavigationController class]]) {
    return (UINavigationController *)topVC;
  } else if (topVC.navigationController) {
    return topVC.navigationController;
  }

  return nil;
}

@end
