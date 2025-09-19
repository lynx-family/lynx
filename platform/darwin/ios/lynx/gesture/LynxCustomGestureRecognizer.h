// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

@interface LynxCustomGestureRecognizer : UIGestureRecognizer
- (void)possibleGesture;
- (void)beginGesture;
- (void)changeGesture;
- (void)endGesture;
- (void)failGesture;
@end

NS_ASSUME_NONNULL_END
