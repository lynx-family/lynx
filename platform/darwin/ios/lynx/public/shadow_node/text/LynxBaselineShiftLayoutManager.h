// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <UIKit/UIKit.h>
#import "LynxShadowNodeStyle.h"

@interface LynxBaselineShiftLayoutManager : NSLayoutManager

- (instancetype)initWithVerticalAlign:(LynxVerticalAlign)verticalAlign;

@property(nonatomic, assign) LynxVerticalAlign verticalAlign;

@end
