// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxThreadSafeDictionary.h>
#import <Lynx/LynxUI.h>

NS_ASSUME_NONNULL_BEGIN

@class LynxUISVG;

@interface LynxSVGView : UIView

@property(nonatomic, weak) LynxUISVG* ui;
@property(nonatomic, strong) UIImage* image;

- (void)invalidate;
- (instancetype)initWithFrame:(CGRect)frame ui:(LynxUISVG*)ui;

@end
NS_ASSUME_NONNULL_END
