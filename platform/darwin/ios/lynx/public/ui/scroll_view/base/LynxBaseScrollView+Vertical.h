// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <Lynx/LynxBaseScrollView+Auto.h>
NS_ASSUME_NONNULL_BEGIN

@protocol LynxVerticalScrollProtocol <NSObject>

@required

- (CGFloat)getScrollOffsetVertically;

- (void)setScrollContentSizeVertically:(CGFloat)contentSize;

- (void)scrollByUnlimitedVertically:(CGFloat)delta;

- (void)scrollByVertically:(CGFloat)delta;

- (void)scrollToUnlimitedVertically:(CGFloat)offset;

- (void)scrollToVertically:(CGFloat)offset;

- (void)animatedScrollToVertically:(CGFloat)offset
                          complete:(LynxBaseScrollViewScrollFinishedCallback)complete;

- (void)animatedScrollToUnlimitedVertically:(CGFloat)offset
                                   complete:(LynxBaseScrollViewScrollFinishedCallback)complete;

- (void)getScrollRangeVertically:(CGFloat (*)[2])range;

- (BOOL)canScrollForwardsVertically;

- (BOOL)canScrollBackwardsVertically;

@end

@interface LynxBaseScrollView (Vertical) <LynxVerticalScrollProtocol>

@end

NS_ASSUME_NONNULL_END
