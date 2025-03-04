// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <Lynx/LynxView.h>
#import <UIKit/UIKit.h>

@interface LynxSettingManager : NSObject
@property(nonatomic, assign) LynxThreadStrategyForRender threadStrategy;
@property(nonatomic, assign) BOOL isPresetWidthAndHeightOn;
@property(nonatomic, retain) UINavigationController* navigationController;
@property(nonatomic, assign) CGRect frame;

+ (instancetype)sharedDataHandler;

- (CGRect)generateFrame:(CGFloat)width height:(CGFloat)height;
- (void)initFrame;
- (void)setPresetWidthAndHeightStatus:(BOOL)isEnablePresetWidthAndHeight;
@end
