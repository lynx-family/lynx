// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <Lynx/LynxBaseScrollView.h>
NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSInteger, NestedScrollMode) {
  NestedScrollModeSelfOnly = 0,
  NestedScrollModeSelfFirst,
  NestedScrollModeParentFirst,
  NestedScrollModeParallel,
};

@protocol LynxNestedScrollProtocol <NSObject>

@required
- (NestedScrollMode)forwardsNestedScrollMode;
- (NestedScrollMode)backwardsNestedScrollMode;
- (CGPoint)previousScrollOffset;
- (void)setPreviousScrollOffset:(CGPoint)offset;
- (NSArray<UIScrollView<LynxNestedScrollProtocol> *> *)getHitTestChainForNestedScrollViews;
- (BOOL)triggerLayoutForNestedScrollViews;
- (void)tryToHandleNestedScroll;
- (void)scrollOffsetUpdated:(UIScrollView *)scrollView;
@end

@interface LynxBaseScrollView (Nested) <LynxNestedScrollProtocol>

+ (NSArray<UIScrollView<LynxNestedScrollProtocol> *> *)generateNestedScrollChainWithHitTestTarget:
    (UIView *)view;

@end

NS_ASSUME_NONNULL_END
