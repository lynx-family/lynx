// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

FOUNDATION_EXPORT const CGFloat LynxListScrollInvalidEstimatedOffset;

typedef NS_ENUM(NSInteger, LynxListProgrammaticScrollCompletionReason) {
  LynxListProgrammaticScrollCompletionReasonImmediate,
  LynxListProgrammaticScrollCompletionReasonAnimationEnded,
  LynxListProgrammaticScrollCompletionReasonTimeout,
};

@protocol LynxListScrollHelperView <NSObject>
@property(nonatomic, assign) BOOL scrollToLower;
@property(nonatomic, assign) CGFloat scrollEstimatedOffset;
@property(nonatomic, assign, setter=setLynxListAdjustingContentOffset:,
          getter=isLynxListAdjustingContentOffset) BOOL adjustingContentOffsetInternally;
@end

@protocol LynxListScrollHelperOwner <NSObject>
- (UIScrollView<LynxListScrollHelperView> *)scrollViewForListScrollHelper;
- (BOOL)isVerticalForListScrollHelper;
- (BOOL)isRTLForListScrollHelper;
- (CGRect)listFrameForListScrollHelper;
- (UIEdgeInsets)listPaddingForListScrollHelper;

// Allows each list implementation to preserve its completion and event semantics while sharing
// request generation, target calculation, and timeout handling.
- (BOOL)listScrollHelperShouldDeferCompletionForSmoothScroll:(BOOL)smooth
                                              needsAnimation:(BOOL)needsAnimation;
- (nullable id)listScrollHelperCompletionToken;
- (BOOL)listScrollHelperShouldFinishTimeoutForPendingScroll;
- (void)listScrollHelperWillStartScrollAnimation;
- (void)listScrollHelperDidStopProgrammaticScroll;
- (void)listScrollHelperDidFinishProgrammaticScrollWithReason:
    (LynxListProgrammaticScrollCompletionReason)reason;
@end

// Owns reusable list scroll geometry and programmatic scroll request coordination independently
// from a concrete list UI or scroll-view subclass.
@interface LynxListScrollHelper : NSObject

- (instancetype)initWithOwner:(id<LynxListScrollHelperOwner>)owner NS_DESIGNATED_INITIALIZER;
- (instancetype)init NS_UNAVAILABLE;

+ (CGPoint)contentOffset:(CGPoint)contentOffset
    constrainedToEstimatedOffset:(CGFloat)estimatedOffset
                        vertical:(BOOL)vertical
                   scrollToLower:(BOOL)scrollToLower;

- (CGPoint)normalizedContentOffset;
- (CGFloat)clampedContentOffsetForVerticalAxis:(BOOL)isVertical;
- (CGFloat)orientationMaxScrollableDistance;
- (CGFloat)orientationSize;
- (CGFloat)orientationContentSize;
- (CGFloat)clampContentOffset:(CGFloat)offset
                        lower:(CGFloat)lower
                         size:(CGFloat)size
                 viewportSize:(CGFloat)viewportSize;
- (CGFloat)availableScrollOffsetFromSubviewsForward:(BOOL)forward
                                      currentOffset:(CGFloat)currentOffset;
- (CGFloat)contentOffsetXForRTL:(CGFloat)contentOffsetX;

// Applies the content-size and offset reconciliation produced by ListElement during node ready.
// The caller continues to own the Lynx lifecycle and recursion-blocking state.
- (CGPoint)applyContentSize:(CGFloat)targetContentSize
                offsetDelta:(CGPoint)targetDelta
      previousContentOffset:(CGPoint)previousContentOffset
     disableScrollFiltering:(BOOL)disableScrollFiltering;

- (void)updateScrollInfoWithEstimatedOffset:(CGFloat)estimatedOffset
                                     smooth:(BOOL)smooth
                                  scrolling:(BOOL)scrolling;
- (void)invalidateProgrammaticScrollRequest;
- (void)finishProgrammaticScrollRequest;
- (BOOL)stopProgrammaticScroll;

@end

NS_ASSUME_NONNULL_END
