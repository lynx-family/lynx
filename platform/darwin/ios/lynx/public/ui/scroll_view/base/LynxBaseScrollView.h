// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

typedef NS_ENUM(NSInteger, LynxBaseScrollViewScrollState) {
  LynxBaseScrollViewScrollStateIdle,       // Idling
  LynxBaseScrollViewScrollStateDragging,   // dragging
  LynxBaseScrollViewScrollStateAnimating,  // programmatically scroll
  LynxBaseScrollViewScrollStateFling,      // fling
};

@class LynxBaseScrollView;

typedef void (^LynxBaseScrollViewScrollFinishedCallback)(BOOL completed);

@protocol LynxBaseScrollViewDelegate <UIScrollViewDelegate>

@required

- (NSArray<UIScrollView *> *)getHitTestChainForNestedScrollViews;

- (void)scrollStateChangedFrom:(LynxBaseScrollViewScrollState)from
                            to:(LynxBaseScrollViewScrollState)to;

@end

@interface LynxBaseScrollView : UIScrollView

@property(nonatomic, weak) id<LynxBaseScrollViewDelegate> scrollDelegate;
@property(nonatomic, assign) BOOL vertical;
@property(nonatomic, assign, readonly) BOOL layoutFromEnd;
@property(nonatomic, assign) BOOL forbidReEntrant;

- (instancetype)initWithVertical:(BOOL)vertical layoutFromEnd:(BOOL)layoutFromEnd;

- (void)tryToUpdateScrollState:(LynxBaseScrollViewScrollState)newState;

- (void)updateProgrammaticallyScrollFinishedCallback:
    (LynxBaseScrollViewScrollFinishedCallback)callback;

- (void)autoScrollWithRate:(CGFloat)rate
                  interval:(NSTimeInterval)interval
                  complete:(LynxBaseScrollViewScrollFinishedCallback)complete;

@end
