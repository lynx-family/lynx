// Copyright 2025 The Lynx Authors
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

@interface LynxGestureExtraBundle : NSObject

// 1 —— DIRECTION_HORIZONTAL -1 —— DIRECTION_VERTICAL 0 —— DIRECTION_UNDETERMINED
@property(nonatomic, assign) NSInteger gestureDirection;
// The offset x that needs to be consumed by the simultaneous gesture
@property(nonatomic, assign) CGFloat simultaneousDeltaX;
// The offset y that needs to be consumed by the simultaneous gesture
@property(nonatomic, assign) CGFloat simultaneousDeltaY;
@property(nonatomic, assign) BOOL isNeedConsumedSimultaneousGesture;
// need consume simultaneous gesture or not
- (void)resetSimultaneousDelta;

@end

NS_ASSUME_NONNULL_END
