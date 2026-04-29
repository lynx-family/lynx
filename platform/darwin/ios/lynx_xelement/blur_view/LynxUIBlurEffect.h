// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

@interface LynxUIBlurEffect : UIBlurEffect

+ (UIBlurEffect* _Nullable)effectWithStyle:(UIBlurEffectStyle)style blurRadius:(CGFloat)radius;

@end

NS_ASSUME_NONNULL_END
