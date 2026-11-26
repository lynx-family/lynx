// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxKeyframeAnimator.h>
#import <Lynx/LynxKeyframeManager.h>

NS_ASSUME_NONNULL_BEGIN

/** @cond */
@interface LynxKeyframeAnimator ()
@property(nonatomic, assign) BOOL allowsUIKitOpacity;
- (nullable NSNumber*)currentUIKitOpacity;
- (void)cancelUIKitOpacityAnimation;
- (void)notifyTransitionUpdated;
@end

@interface LynxKeyframeManager ()
- (nullable NSNumber*)currentUIKitOpacity;
- (void)cancelUIKitOpacityAnimation;
- (void)notifyTransitionUpdated;
@end
// cspell:ignore endcond
/** @endcond */

NS_ASSUME_NONNULL_END
