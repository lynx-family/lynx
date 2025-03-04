// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxUIKitAPIAdapter.h"

@implementation LynxUIKitAPIAdapter

+ (NSArray<UIWindow *> *)getWindows {
  if (@available(iOS 15.0, *)) {
    for (UIScene *scene in [UIApplication sharedApplication].connectedScenes) {
      if (scene.activationState == UISceneActivationStateForegroundActive &&
          [scene isKindOfClass:[UIWindowScene class]]) {
        return ((UIWindowScene *)scene).windows;
      }
    }
  } else {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    return [UIApplication sharedApplication].windows;
#pragma clang diagnostic pop
  }
  return nil;
}

+ (UIWindow *)getKeyWindow {
  if (@available(iOS 15.0, *)) {
    for (UIScene *scene in [UIApplication sharedApplication].connectedScenes) {
      if (scene.activationState == UISceneActivationStateForegroundActive &&
          [scene isKindOfClass:[UIWindowScene class]]) {
        return ((UIWindowScene *)scene).keyWindow;
      }
    }
  } else {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    for (UIWindow *window in [UIApplication sharedApplication].windows) {
#pragma clang diagnostic pop
      if (window.isKeyWindow) {
        return window;
      }
    }
  }
  return nil;
}

+ (CGRect)getStatusBarFrame {
  if (@available(iOS 13.0, *)) {
    return [LynxUIKitAPIAdapter getKeyWindow].windowScene.statusBarManager.statusBarFrame;
  } else {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    return [[UIApplication sharedApplication] statusBarFrame];
#pragma clang diagnostic pop
  }
}

@end
