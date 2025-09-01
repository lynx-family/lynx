// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <Lynx/LynxBaseScrollView+Horizontal.h>
#import <Lynx/LynxBaseScrollView+Vertical.h>
#import <Lynx/LynxBaseScrollView.h>

NS_ASSUME_NONNULL_BEGIN

@protocol LynxScrollProtocol <NSObject>

@required

- (CGPoint)getScrollOffset;

- (void)setScrollContentSize:(CGSize)contentSize;

- (void)scrollByUnlimited:(CGPoint)delta;

- (void)scrollBy:(CGPoint)delta;

- (void)scrollToUnlimited:(CGPoint)offset;

- (void)scrollTo:(CGPoint)offset;

- (void)animatedScrollTo:(CGPoint)offset
                complete:(LynxBaseScrollViewScrollFinishedCallback)complete;

- (void)animatedScrollToUnlimited:(CGPoint)offset
                         complete:(LynxBaseScrollViewScrollFinishedCallback)complete;

- (void)getScrollRange:(CGFloat (*)[4])range;

- (BOOL)canScrollForwards;

- (BOOL)canScrollBackwards;

@end

@interface LynxBaseScrollView (Auto) <LynxScrollProtocol>

@end

NS_ASSUME_NONNULL_END
