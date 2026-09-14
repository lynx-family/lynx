// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxBaseScrollView.h>
#import <UIKit/UIKit.h>

#import "LynxUIListContainer+Internal.h"

NS_ASSUME_NONNULL_BEGIN

@class LynxGestureConsumer;
@class LynxGestureDetectorDarwin;

@protocol LynxListScrollViewOwner <LynxBaseScrollViewDelegate>
- (void)listScrollView:(UIScrollView *)scrollView
    contentSizeChangedFrom:(CGSize)oldSize
                        to:(CGSize)newSize;
- (void)listScrollViewWillEndDragging:(UIScrollView *)scrollView
                         withVelocity:(CGPoint)velocity
                  targetContentOffset:(inout CGPoint *)targetContentOffset;
- (BOOL)listScrollViewShouldScrollToTop:(UIScrollView *)scrollView;
- (void)listScrollViewAutoScrollDidStop;
- (void)detachedFromWindow;
- (void)updateContentSize;
@end

// Private list backend. UIKit's delegate remains the BaseScrollView itself;
// the list UI receives callbacks through ui/scrollDelegate.
@interface LynxListScrollView
    : LynxBaseScrollView <LynxListContainerScrollView, UIGestureRecognizerDelegate>
@property(nonatomic, assign) BOOL scrollToLower;
@property(nonatomic, assign) BOOL verticalOrientation;
@property(nonatomic, assign) CGFloat scrollEstimatedOffset;
@property(nonatomic, weak, nullable) id<LynxListScrollViewOwner> ui;
@property(nonatomic, assign, setter=setLynxListAdjustingContentOffset:,
          getter=isLynxListAdjustingContentOffset) BOOL adjustingContentOffsetInternally;
@property(nonatomic, assign) BOOL forceCanScroll;
@property(nonatomic, assign, nullable) Class blockGestureClass;
@property(nonatomic, assign) NSInteger recognizedViewTag;
@property(nonatomic, assign) BOOL duringGestureScroll;
@property(nonatomic, assign) BOOL gestureEnabled;
@property(nonatomic, assign) BOOL increaseFrequencyWithGesture;
@property(nonatomic, strong, readonly, nullable) UIPanGestureRecognizer *nativeGesturePanRecognizer;
@property(nonatomic, strong, nullable) LynxGestureConsumer *gestureConsumer;

- (void)setupNativeGestureRecognizerIfNeeded:
    (NSDictionary<NSNumber *, LynxGestureDetectorDarwin *> *)gestureMap;
- (void)updateContentSize;
@end

NS_ASSUME_NONNULL_END
