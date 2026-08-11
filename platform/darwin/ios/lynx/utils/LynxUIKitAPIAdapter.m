// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxUIKitAPIAdapter.h>

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

+ (NSArray<UIWindow *> *)getAllWindows {
  if (@available(iOS 13.0, *)) {
    NSMutableArray<UIWindow *> *windows = [NSMutableArray array];
    for (UIScene *scene in [UIApplication sharedApplication].connectedScenes) {
      if ([scene isKindOfClass:[UIWindowScene class]]) {
        [windows addObjectsFromArray:((UIWindowScene *)scene).windows];
      }
    }
    return windows;
  }
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
  return [UIApplication sharedApplication].windows;
#pragma clang diagnostic pop
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

+ (UIWindow *)getForegroundKeyWindow {
  if (@available(iOS 15.0, *)) {
    for (UIScene *scene in [UIApplication sharedApplication].connectedScenes) {
      if ((scene.activationState == UISceneActivationStateForegroundActive ||
           scene.activationState == UISceneActivationStateForegroundInactive) &&
          [scene isKindOfClass:[UIWindowScene class]]) {
        return ((UIWindowScene *)scene).keyWindow;
      }
    }
  }
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
  for (UIWindow *window in [UIApplication sharedApplication].windows) {
#pragma clang diagnostic pop
    if (window.isKeyWindow) {
      return window;
    }
  }
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
  UIWindow *window = [UIApplication sharedApplication].keyWindow;
#pragma clang diagnostic pop
  return window;
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

+ (void)hideMenuController:(UIMenuController *)menu fromView:(UIView *)view {
#if TARGET_OS_MACCATALYST
  [menu hideMenuFromView:view];
#else
  if (@available(iOS 13.0, *)) {
    [menu hideMenuFromView:view];
  } else {
    [menu setMenuVisible:NO animated:YES];
  }
#endif
}

+ (void)showMenuController:(UIMenuController *)menu fromView:(UIView *)view rect:(CGRect)rect {
#if TARGET_OS_MACCATALYST
  [menu showMenuFromView:view rect:rect];
#else
  if (@available(iOS 13.0, *)) {
    [menu showMenuFromView:view rect:rect];
  } else {
    [menu setTargetRect:rect inView:view];
    [menu setMenuVisible:YES animated:YES];
  }
#endif
}

@end
