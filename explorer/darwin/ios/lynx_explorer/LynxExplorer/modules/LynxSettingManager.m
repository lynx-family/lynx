// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxSettingManager.h"

@implementation LynxSettingManager
+ (instancetype)sharedDataHandler {
  static dispatch_once_t onceToken;
  static LynxSettingManager *_instance;

  dispatch_once(&onceToken, ^{
    _instance = [[LynxSettingManager alloc] init];
    _instance.isPresetWidthAndHeightOn = NO;
    _instance.threadStrategy = LynxThreadStrategyForRenderAllOnUI;
    [_instance initFrame];
  });

  return _instance;
}

- (CGRect)generateFrame:(CGFloat)width height:(CGFloat)height {
  CGRect frame = CGRectZero;
  CGRect screenFrame = [UIScreen mainScreen].bounds;
  frame.origin.x = screenFrame.origin.x;
  frame.origin.y = screenFrame.origin.y;
  frame.size.width = width;
  frame.size.height = height;

  return frame;
};

- (void)initFrame {
  CGRect screenFrame = [UIScreen mainScreen].bounds;
  CGRect frame = CGRectZero;
  CGRect statusRect = [[UIApplication sharedApplication] statusBarFrame];
  CGRect navRect = _navigationController.navigationBar.frame;
  frame.origin.x = screenFrame.origin.x;
  frame.origin.y = screenFrame.origin.y;
  frame.size.width = screenFrame.size.width;
  frame.size.height = screenFrame.size.height - statusRect.size.height - navRect.size.height;
  self.frame = frame;
};

- (void)setPresetWidthAndHeightStatus:(BOOL)isEnablePresetWidthAndHeight {
  _isPresetWidthAndHeightOn = isEnablePresetWidthAndHeight;
};

@end
