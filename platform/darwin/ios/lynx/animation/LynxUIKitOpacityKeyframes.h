// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

/** @cond */
@interface LynxUIKitOpacityKeyframes : NSObject
@property(nonatomic, copy, nullable) BOOL (^isCurrent)(void);
+ (BOOL)canAnimate:(CAKeyframeAnimation*)animation;
- (instancetype)initWithAnimation:(CAKeyframeAnimation*)animation
                             view:(UIView*)view
                   activeDuration:(NSTimeInterval)activeDuration
                underlyingOpacity:(CGFloat)underlyingOpacity
                       completion:(void (^)(BOOL finished))completion;
- (void)start;
- (void)stop;
- (void)setUnderlyingOpacity:(CGFloat)opacity;
- (NSNumber*)currentOpacity;
@end
// cspell:ignore endcond
/** @endcond */

NS_ASSUME_NONNULL_END
