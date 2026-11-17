// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxTransitionAnimationManager.h>

NS_ASSUME_NONNULL_BEGIN

/** @cond */
@interface LynxTransitionAnimationManager ()
- (void)removeLegacyTransformTransition;
- (void)cancelPlatformTransition:(uint64_t)animationID
                      generation:(uint32_t)generation __attribute__((objc_direct));
// Endpoints are numbers for opacity and arrays of LynxTransformRaw for transform.
- (void)applyPlatformTransition:(LynxAnimationInfo*)info
                           from:(id)from
                             to:(id)to
                    animationID:(uint64_t)animationID
                     generation:(uint32_t)generation __attribute__((objc_direct));
@end
/** @endcond */

NS_ASSUME_NONNULL_END
