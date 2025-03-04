// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxListViewLight.h>
#import <Lynx/LynxPropsProcessor.h>
#import <Lynx/LynxUI.h>
#import <Lynx/LynxUIListInvalidationContext.h>
#import <Lynx/LynxUIListLight+PropSetter.h>
#import <Lynx/LynxUIListLight.h>

@implementation LynxUIListLight (propSetter)
LYNX_PROPS_GROUP_DECLARE(
    LYNX_PROP_DECLARE("preload-buffer-count", setPreloadBuffer, NSInteger),
    LYNX_PROP_DECLARE("list-cross-axis-gap", setListCrossAxisGap, CGFloat),
    LYNX_PROP_DECLARE("list-main-axis-gap", setListMainAxisGap, CGFloat),
    LYNX_PROP_DECLARE("update-list-info", updateListActionInfo, NSDictionary *),
    LYNX_PROP_DECLARE("list-type", setListType, NSString *),
    LYNX_PROP_DECLARE("initial-scroll-index", setInitialScrollIndex, NSNumber *),
    LYNX_PROP_DECLARE("data-update-type", setDataUpdateType, NSInteger),
    LYNX_PROP_DECLARE("insert-anchor-mode", setInsertAnchorModeInside, NSString *),
    LYNX_PROP_DECLARE("needs-visible-cells", setNeedsVisibleCells, NSNumber *),
    LYNX_PROP_DECLARE("delete-regress-policy", setDeleteRegressPolicyToTop, NSString *),
    LYNX_PROP_DECLARE("lower-threshold", setLowerThreshold, NSNumber *),
    LYNX_PROP_DECLARE("upper-threshold", setUpperThreshold, NSNumber *),
    LYNX_PROP_DECLARE("upper-threshold-item-count", setUpperThresholdItemCount, NSNumber *),
    LYNX_PROP_DECLARE("lower-threshold-item-count", setLowerThresholdItemCount, NSNumber *),
    LYNX_PROP_DECLARE("scroll-event-throttle", setScrollEventThrottle, NSNumber *),
    LYNX_PROP_DECLARE("list-platform-info", setCurComponents, NSDictionary *),
    LYNX_PROP_DECLARE("anchor-priority", setAnchorPriorityFromBegin, NSString *),
    LYNX_PROP_DECLARE("delete-regress-policy", setDeleteRegressPolicyToTop, NSString *),
    LYNX_PROP_DECLARE("insert-anchor-mode", setInsertAnchorModeInside, NSString *),
    LYNX_PROP_DECLARE("anchor-visibility", setAnchorVisibility, NSString *),
    LYNX_PROP_DECLARE("anchor-align", setAnchorAlignToBottom, NSString *),
    LYNX_PROP_DECLARE("enable-fade-in-animation", setEnableFadeInAnimation, BOOL),
    LYNX_PROP_DECLARE("update-animation-fade-in-duration", setUpdateAnimationFadeInDuration,
                      NSInteger));

#pragma mark general layout related props update
LYNX_PROP_DEFINE("list-type", setListType, NSString *) {
  if (requestReset) {
    value = @"flow";
  }

  LynxListLayoutType type = LynxListLayoutFlow;

  if ([value isEqualToString:@"waterfall"]) {
    type = LynxListLayoutWaterfall;
  } else if ([value isEqualToString:@"flow"]) {
    type = LynxListLayoutFlow;
  }

  self.generalPropsInfo.layoutType = type;
}

LYNX_PROP_SETTER("column-count", setColumnCount, NSInteger) {
  if (requestReset || value <= 0) {
    value = 1;
  }
  ((LynxListViewLight *)self.view).numberOfColumns = value;
  self.generalPropsInfo.numberOfColumns = value;
}

LYNX_PROP_DEFINE("needs-visible-cells", setNeedsVisibleCells, NSNumber *) {
  self.generalPropsInfo.needsVisibleCells = requestReset ? @(NO) : value;
}

LYNX_PROP_DEFINE("preload-buffer-count", setPreloadBuffer, NSInteger) {
  if (requestReset) {
    value = 0;
  }
  ((LynxListViewLight *)self.view).preloadBufferCount = value;
}

LYNX_PROP_DEFINE("list-cross-axis-gap", setListCrossAxisGap, CGFloat) {
  if (requestReset || value < 0) {
    value = 0;
  }
  self.generalPropsInfo.crossAxisGap = value;
}

LYNX_PROP_DEFINE("list-main-axis-gap", setListMainAxisGap, CGFloat) {
  if (requestReset || value < 0) {
    value = 0;
  }
  self.generalPropsInfo.mainAxisGap = value;
}

// Don't support dynamically change
LYNX_PROP_SETTER("vertical-orientation", setVerticalOrientation, BOOL) {
  [self setVerticalOrientation:value];
  ((LynxListViewLight *)self.view).verticalOrientation = value;
}

LYNX_PROP_DEFINE("internal-cell-prepare-for-reuse-notification",
                 setInternalCellPrepareForReuseNotification, BOOL) {
  self.generalPropsInfo.needsInternalCellPrepareForReuseNotification = requestReset ? NO : value;
}

LYNX_PROP_DEFINE("list-platform-info", setCurComponents, NSDictionary *) {
  self.diffResultFromTasm = value[@"diffResult"];
  self.curComponents = value;
  self.listNoDiffInfo = nil;
}

LYNX_PROP_DEFINE("update-list-info", updateListActionInfo, NSDictionary *) {
  self.listNoDiffInfo = value;
  self.diffResultFromTasm = nil;
}

LYNX_PROP_DEFINE("initial-scroll-index", setInitialScrollIndex, NSNumber *) {
  self.initialScrollIndex = value;
}

LYNX_PROP_DEFINE("upper-threshold", setUpperThreshold, NSNumber *) {
  self.scrollThreSholds.scrollToStartOffset = value;
}

LYNX_PROP_DEFINE("lower-threshold", setLowerThreshold, NSNumber *) {
  self.scrollThreSholds.scrollToEndOffset = value;
}

LYNX_PROP_DEFINE("upper-threshold-item-count", setUpperThresholdItemCount, NSNumber *) {
  self.scrollThreSholds.scrollToStartItemCount = value;
}

LYNX_PROP_DEFINE("lower-threshold-item-count", setLowerThresholdItemCount, NSNumber *) {
  self.scrollThreSholds.scrollToEndItemCount = value;
}

LYNX_PROP_DEFINE("scroll-event-throttle", setScrollEventThrottle, NSNumber *) {
  self.scrollThreSholds.throttle = value;
}

#pragma mark anchor policies
LYNX_PROP_DEFINE("anchor-priority", setAnchorPriorityFromBegin, NSString *) {
  if ([value isEqualToString:@"fromBegin"]) {
    ((LynxListViewLight *)self.view).anchorPriorityFromBegin = YES;
  } else {
    ((LynxListViewLight *)self.view).anchorPriorityFromBegin = NO;
  }
}

LYNX_PROP_DEFINE("delete-regress-policy", setDeleteRegressPolicyToTop, NSString *) {
  if ([value isEqualToString:@"toTop"]) {
    ((LynxListViewLight *)self.view).deleteRegressPolicyToTop = YES;
  } else {
    ((LynxListViewLight *)self.view).deleteRegressPolicyToTop = NO;
  }
}

LYNX_PROP_DEFINE("insert-anchor-mode", setInsertAnchorModeInside, NSString *) {
  if ([value isEqualToString:@"inside"]) {
    ((LynxListViewLight *)self.view).insertAnchorModeInside = YES;
  } else {
    ((LynxListViewLight *)self.view).insertAnchorModeInside = NO;
  }
}

LYNX_PROP_DEFINE("anchor-visibility", setAnchorVisibility, NSString *) {
  if ([value isEqualToString:@"show"]) {
    ((LynxListViewLight *)self.view).anchorVisibility = LynxAnchorVisibilityShow;
  } else if ([value isEqualToString:@"hide"]) {
    ((LynxListViewLight *)self.view).anchorVisibility = LynxAnchorVisibilityHide;
  } else {
    ((LynxListViewLight *)self.view).anchorVisibility = LynxAnchorVisibilityNoAdjustment;
  }
}

LYNX_PROP_DEFINE("anchor-align", setAnchorAlignToBottom, NSString *) {
  if ([value isEqualToString:@"toBottom"]) {
    ((LynxListViewLight *)self.view).anchorAlignToBottom = YES;
  } else {
    ((LynxListViewLight *)self.view).anchorAlignToBottom = NO;
  }
}

LYNX_PROP_DEFINE("enable-fade-in-animation", setEnableFadeInAnimation, BOOL) {
  ((LynxListViewLight *)self.view).enableFadeInAnimation = value;
}
LYNX_PROP_DEFINE("update-animation-fade-in-duration", setUpdateAnimationFadeInDuration, NSInteger) {
  ((LynxListViewLight *)self.view).updateAnimationFadeInDuration = value / 1000.;
}

@end
