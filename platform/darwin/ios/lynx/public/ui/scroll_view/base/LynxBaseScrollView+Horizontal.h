// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <Lynx/LynxBaseScrollView.h>

NS_ASSUME_NONNULL_BEGIN

@protocol LynxHorizontalScrollProtocol <NSObject>

@required

- (CGFloat)getScrollOffsetHorizontally;

- (void)setScrollContentSizeHorizontally:(CGFloat)contentSize;

- (void)scrollByUnlimitedHorizontally:(CGFloat)delta;

- (void)scrollByHorizontally:(CGFloat)delta;

- (void)scrollToUnlimitedHorizontally:(CGFloat)offset;

- (void)scrollToHorizontally:(CGFloat)offset;

- (void)animatedScrollToHorizontally:(CGFloat)offset
                            complete:(LynxBaseScrollViewScrollFinishedCallback)complete;

- (void)animatedScrollToUnlimitedHorizontally:(CGFloat)offset
                                     complete:(LynxBaseScrollViewScrollFinishedCallback)complete;

- (void)getScrollRangeHorizontally:(CGFloat (*)[2])range;

- (BOOL)canScrollForwardsHorizontally;

- (BOOL)canScrollBackwardsHorizontally;

@end

@interface LynxBaseScrollView (Horizontal) <LynxHorizontalScrollProtocol>

@end

NS_ASSUME_NONNULL_END
