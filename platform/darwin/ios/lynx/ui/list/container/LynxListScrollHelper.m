// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxListScrollHelper.h"

#import "LynxListItemHelper.h"

#include <math.h>

const CGFloat LynxListScrollInvalidEstimatedOffset = -1.0;

static const int64_t kLynxListProgrammaticScrollTimeoutInMilliseconds = 600;

@interface LynxListScrollHelper ()

@property(nonatomic, weak) id<LynxListScrollHelperOwner> owner;
@property(nonatomic, assign) NSInteger scrollRequestId;

- (void)finishProgrammaticScrollRequestWithReason:
    (LynxListProgrammaticScrollCompletionReason)reason;

@end

@implementation LynxListScrollHelper

+ (CGPoint)contentOffset:(CGPoint)contentOffset
    constrainedToEstimatedOffset:(CGFloat)estimatedOffset
                        vertical:(BOOL)vertical
                   scrollToLower:(BOOL)scrollToLower {
  if (estimatedOffset == LynxListScrollInvalidEstimatedOffset) {
    return contentOffset;
  }

  CGFloat proposedOffset = vertical ? contentOffset.y : contentOffset.x;
  if ((scrollToLower && proposedOffset > estimatedOffset) ||
      (!scrollToLower && proposedOffset < estimatedOffset)) {
    if (vertical) {
      contentOffset.y = estimatedOffset;
    } else {
      contentOffset.x = estimatedOffset;
    }
  }
  return contentOffset;
}

- (instancetype)initWithOwner:(id<LynxListScrollHelperOwner>)owner {
  self = [super init];
  if (self) {
    _owner = owner;
  }
  return self;
}

- (CGPoint)normalizedContentOffset {
  UIScrollView *scrollView = [self.owner scrollViewForListScrollHelper];
  return CGPointMake([self.owner isRTLForListScrollHelper]
                         ? [self contentOffsetXForRTL:scrollView.contentOffset.x]
                         : scrollView.contentOffset.x,
                     scrollView.contentOffset.y);
}

- (CGFloat)clampedContentOffsetForVerticalAxis:(BOOL)isVertical {
  UIScrollView *scrollView = [self.owner scrollViewForListScrollHelper];
  CGFloat validOffset = isVertical ? scrollView.contentOffset.y : scrollView.contentOffset.x;
  if (!isVertical && [self.owner isRTLForListScrollHelper]) {
    validOffset = [self contentOffsetXForRTL:validOffset];
  }
  validOffset = MAX(0, validOffset);
  return MIN(validOffset, self.orientationMaxScrollableDistance);
}

- (CGFloat)orientationMaxScrollableDistance {
  UIScrollView *scrollView = [self.owner scrollViewForListScrollHelper];
  CGRect listFrame = [self.owner listFrameForListScrollHelper];
  return MAX(0, [self.owner isVerticalForListScrollHelper]
                    ? scrollView.contentSize.height - listFrame.size.height
                    : scrollView.contentSize.width - listFrame.size.width);
}

- (CGFloat)orientationSize {
  CGRect scrollViewFrame = [self.owner scrollViewForListScrollHelper].frame;
  return [self.owner isVerticalForListScrollHelper]
             ? scrollViewFrame.origin.y + scrollViewFrame.size.height
             : scrollViewFrame.origin.x + scrollViewFrame.size.width;
}

- (CGFloat)orientationContentSize {
  CGSize contentSize = [self.owner scrollViewForListScrollHelper].contentSize;
  return [self.owner isVerticalForListScrollHelper] ? contentSize.height : contentSize.width;
}

- (CGFloat)clampContentOffset:(CGFloat)offset
                        lower:(CGFloat)lower
                         size:(CGFloat)size
                 viewportSize:(CGFloat)viewportSize {
  offset = MIN(offset, size - viewportSize);
  return MAX(offset, lower);
}

- (CGFloat)availableScrollOffsetFromSubviewsForward:(BOOL)forward
                                      currentOffset:(CGFloat)currentOffset {
  UIScrollView *scrollView = [self.owner scrollViewForListScrollHelper];
  if (scrollView.subviews.count == 0) {
    return currentOffset;
  }

  __block CGFloat max = CGFLOAT_MIN;
  __block CGFloat min = CGFLOAT_MAX;
  BOOL vertical = [self.owner isVerticalForListScrollHelper];
  UIEdgeInsets padding = [self.owner listPaddingForListScrollHelper];
  if (forward) {
    CGFloat contentSize = vertical ? scrollView.contentSize.height : scrollView.contentSize.width;
    CGFloat paddingEnd = vertical ? padding.bottom : padding.right;
    [scrollView.subviews enumerateObjectsUsingBlock:^(__kindof UIView *_Nonnull subview,
                                                      NSUInteger index, BOOL *_Nonnull stop) {
      if ([subview conformsToProtocol:@protocol(LynxListItemWrapper)]) {
        max = MAX(max, vertical ? CGRectGetMaxY(subview.frame) : CGRectGetMaxX(subview.frame));
      }
    }];
    if (fabs(max + paddingEnd - contentSize) < CGFLOAT_EPSILON) {
      max = contentSize;
    }
    max -= vertical ? scrollView.frame.size.height : scrollView.frame.size.width;
  } else {
    CGFloat paddingStart = vertical ? padding.top : padding.left;
    [scrollView.subviews enumerateObjectsUsingBlock:^(__kindof UIView *_Nonnull subview,
                                                      NSUInteger index, BOOL *_Nonnull stop) {
      if ([subview conformsToProtocol:@protocol(LynxListItemWrapper)]) {
        min = MIN(min, vertical ? CGRectGetMinY(subview.frame) : CGRectGetMinX(subview.frame));
      }
    }];
    if (fabs(min - paddingStart) < CGFLOAT_EPSILON) {
      min = 0.f;
    }
  }
  return forward ? max : min;
}

- (CGFloat)contentOffsetXForRTL:(CGFloat)contentOffsetX {
  UIScrollView *scrollView = [self.owner scrollViewForListScrollHelper];
  return MAX(scrollView.contentSize.width - contentOffsetX - scrollView.frame.size.width, 0.f);
}

- (CGPoint)applyContentSize:(CGFloat)targetContentSize
                offsetDelta:(CGPoint)targetDelta
      previousContentOffset:(CGPoint)previousContentOffset
     disableScrollFiltering:(BOOL)disableScrollFiltering {
  UIScrollView<LynxListScrollHelperView> *scrollView = [self.owner scrollViewForListScrollHelper];
  BOOL vertical = [self.owner isVerticalForListScrollHelper];
  CGRect listFrame = [self.owner listFrameForListScrollHelper];
  UIEdgeInsets padding = [self.owner listPaddingForListScrollHelper];
  BOOL contentSizeChanged = vertical ? targetContentSize != scrollView.contentSize.height
                                     : targetContentSize != scrollView.contentSize.width;
  BOOL deltaChanged = vertical ? targetDelta.y != 0 : targetDelta.x != 0;

  if (vertical) {
    CGFloat contentWidth = listFrame.size.width - padding.left - padding.right;
    scrollView.contentSize = CGSizeMake(contentWidth, targetContentSize);
  } else {
    CGFloat contentHeight = listFrame.size.height - padding.top - padding.bottom;
    scrollView.contentSize = CGSizeMake(targetContentSize, contentHeight);
  }

  // Changing contentSize may cause UIKit to adjust contentOffset synchronously. Always derive the
  // ListElement result from the offset captured before the size change.
  CGPoint adjustedContentOffset =
      CGPointMake(previousContentOffset.x + targetDelta.x, previousContentOffset.y + targetDelta.y);

  // Keep list-container compatibility: filtering avoids an incorrect offset update during refresh,
  // while disableScrollFiltering preserves externally modified offsets from KVO integrations.
  if (disableScrollFiltering || contentSizeChanged || deltaChanged) {
    scrollView.adjustingContentOffsetInternally = YES;
    scrollView.contentOffset = CGPointMake([self.owner isRTLForListScrollHelper]
                                               ? [self contentOffsetXForRTL:adjustedContentOffset.x]
                                               : adjustedContentOffset.x,
                                           adjustedContentOffset.y);
    scrollView.adjustingContentOffsetInternally = NO;
  }
  return adjustedContentOffset;
}

- (void)updateScrollInfoWithEstimatedOffset:(CGFloat)estimatedOffset
                                     smooth:(BOOL)smooth
                                  scrolling:(BOOL)scrolling {
  UIScrollView<LynxListScrollHelperView> *scrollView = [self.owner scrollViewForListScrollHelper];
  scrollView.scrollEstimatedOffset = estimatedOffset;
  if (scrolling) {
    return;
  }

  NSInteger scrollRequestId = ++self.scrollRequestId;
  BOOL vertical = [self.owner isVerticalForListScrollHelper];
  scrollView.scrollToLower = vertical ? estimatedOffset > scrollView.contentOffset.y
                                      : estimatedOffset > scrollView.contentOffset.x;
  CGPoint target = CGPointMake(vertical ? scrollView.contentOffset.x : estimatedOffset,
                               vertical ? estimatedOffset : scrollView.contentOffset.y);
  BOOL needsAnimation = smooth && !CGPointEqualToPoint(scrollView.contentOffset, target);
  if (needsAnimation) {
    [self.owner listScrollHelperWillStartScrollAnimation];
  }

  [scrollView setContentOffset:target animated:smooth];

  if (![self.owner listScrollHelperShouldDeferCompletionForSmoothScroll:smooth
                                                         needsAnimation:needsAnimation]) {
    scrollView.scrollEstimatedOffset = LynxListScrollInvalidEstimatedOffset;
    [self.owner listScrollHelperDidFinishProgrammaticScrollWithReason:
                    LynxListProgrammaticScrollCompletionReasonImmediate];
    return;
  }

  id completionToken = [self.owner listScrollHelperCompletionToken];
  __weak __typeof(self) weakSelf = self;
  dispatch_after(dispatch_time(DISPATCH_TIME_NOW,
                               (kLynxListProgrammaticScrollTimeoutInMilliseconds * NSEC_PER_MSEC)),
                 dispatch_get_main_queue(), ^{
                   __strong __typeof(weakSelf) strongSelf = weakSelf;
                   if (!strongSelf || scrollRequestId != strongSelf.scrollRequestId) {
                     return;
                   }
                   UIScrollView<LynxListScrollHelperView> *strongScrollView =
                       [strongSelf.owner scrollViewForListScrollHelper];
                   BOOL hasPendingScroll = strongScrollView.scrollEstimatedOffset !=
                                           LynxListScrollInvalidEstimatedOffset;
                   id currentCompletionToken = [strongSelf.owner listScrollHelperCompletionToken];
                   BOOL hasMatchingCompletion =
                       completionToken != nil && completionToken == currentCompletionToken;
                   if (!hasPendingScroll && !hasMatchingCompletion) {
                     return;
                   }
                   if (hasPendingScroll) {
                     [strongSelf stopProgrammaticScroll];
                   }
                   if (hasMatchingCompletion ||
                       [strongSelf.owner listScrollHelperShouldFinishTimeoutForPendingScroll]) {
                     [strongSelf.owner listScrollHelperDidFinishProgrammaticScrollWithReason:
                                           LynxListProgrammaticScrollCompletionReasonTimeout];
                   }
                 });
}

- (void)finishProgrammaticScrollRequest {
  [self finishProgrammaticScrollRequestWithReason:
            LynxListProgrammaticScrollCompletionReasonAnimationEnded];
}

- (void)invalidateProgrammaticScrollRequest {
  ++self.scrollRequestId;
}

- (void)finishProgrammaticScrollRequestWithReason:
    (LynxListProgrammaticScrollCompletionReason)reason {
  [self stopProgrammaticScroll];
  [self.owner listScrollHelperDidFinishProgrammaticScrollWithReason:reason];
}

- (BOOL)stopProgrammaticScroll {
  UIScrollView<LynxListScrollHelperView> *scrollView = [self.owner scrollViewForListScrollHelper];
  if (scrollView.scrollEstimatedOffset == LynxListScrollInvalidEstimatedOffset) {
    return NO;
  }
  scrollView.scrollEstimatedOffset = LynxListScrollInvalidEstimatedOffset;
  [self.owner listScrollHelperDidStopProgrammaticScroll];
  return YES;
}

@end
