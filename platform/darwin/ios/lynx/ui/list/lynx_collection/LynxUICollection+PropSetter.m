// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxCollectionDataSource.h"
#import "LynxCollectionInvalidationContext.h"
#import "LynxCollectionViewLayout.h"
#import "LynxFeatureCounter.h"
#import "LynxPropsProcessor.h"
#import "LynxSubErrorCode.h"
#import "LynxUI+Internal.h"
#import "LynxUICollection+Delegate.h"
#import "LynxUICollection+Internal.h"
#import "LynxUICollection+PropSetter.h"
#import "LynxUIContext+Internal.h"

static const CGFloat DEFAULT_SCROLL_EVENT_THROTTLE = 200;
static const CGFloat DEFAULT_SCROLL_UPPER_THRESHOLD = 50;
static const CGFloat DEFAULT_SCROLL_LOWER_THRESHOLD = 50;
static const NSInteger DEFAULT_SCROLL_UPPER_THRESHOLD_ITEM_COUNT = 0;
static const NSInteger DEFAULT_SCROLL_LOWER_THRESHOLD_ITEM_COUNT = 0;

@implementation LynxUICollection (PropSetter)

LYNX_PROPS_GROUP_DECLARE(
    LYNX_PROP_DECLARE("enable-async-list", enableAsyncList, BOOL),
    LYNX_PROP_DECLARE("scroll-event-throttle", setScrollEventThrottle, CGFloat),
    LYNX_PROP_DECLARE("upper-threshold", setUpperThreshold, CGFloat),
    LYNX_PROP_DECLARE("lower-threshold", setLowerThreshold, CGFloat),
    LYNX_PROP_DECLARE("upper-threshold-item-count", setUpperThresholdItemCount, NSInteger),
    LYNX_PROP_DECLARE("lower-threshold-item-count", setLowerThresholdItemCount, NSInteger),
    LYNX_PROP_DECLARE("scroll-bar-enable", setScrollBarEnable, BOOL),
    LYNX_PROP_DECLARE("bounces", setBounces, BOOL),
    LYNX_PROP_DECLARE("needs-visible-cells", setNeedsVisibleCells, BOOL),
    LYNX_PROP_DECLARE("column-count", setColumnCount, NSInteger),
    LYNX_PROP_DECLARE("list-type", setListType, NSString *),
    LYNX_PROP_DECLARE("update-animation", setUpdateAnimation, NSString *),
    LYNX_PROP_DECLARE("enable-scroll", setScrollEnabled, BOOL),
    LYNX_PROP_DECLARE("touch-scroll", setTouchEnabled, BOOL),
    LYNX_PROP_DECLARE("sticky", setEnableSticky, BOOL),
    LYNX_PROP_DECLARE("sticky-offset", setStickyOffset, CGFloat),
    LYNX_PROP_DECLARE("initial-scroll-index", setInitialScrollIndex, NSInteger),
    LYNX_PROP_DECLARE("scroll-index", setUpdatedScrollIndex, NSInteger),
    LYNX_PROP_DECLARE("paging-enabled", setPageEnabled, BOOL),
    LYNX_PROP_DECLARE("item-snap", setPagingAlignment, NSDictionary *),
    LYNX_PROP_DECLARE("list-main-axis-gap", setListMainAxisGap, CGFloat),
    LYNX_PROP_DECLARE("list-cross-axis-gap", setListCrossAxisGap, CGFloat),
    LYNX_PROP_DECLARE("ios-index-as-z-index", setIndexAsZIndex, BOOL),
    LYNX_PROP_DECLARE("internal-cell-appear-notification", setInternalCellAppearNotification, BOOL),
    LYNX_PROP_DECLARE("internal-cell-disappear-notification", setInternalCellDisappearNotification,
                      BOOL),
    LYNX_PROP_DECLARE("internal-cell-prepare-for-reuse-notification",
                      setInternalCellPrepareForReuseNotification, BOOL),
    LYNX_PROP_DECLARE("ios-scrolls-to-top", iosScrollsToTop, BOOL),
    LYNX_PROP_DECLARE("experimental-ios-gesture-frequency-increasing", exprIOSIncreaseFrequency,
                      BOOL),
    LYNX_PROP_DECLARE("enable-rtl", setEnableRtl, BOOL));

LYNX_PROP_DEFINE("scroll-event-throttle", setScrollEventThrottle, CGFloat) {
  if (requestReset) {
    self.scrollEventEmitter.scrollEventThrottle = DEFAULT_SCROLL_EVENT_THROTTLE;
  } else {
    self.scrollEventEmitter.scrollEventThrottle = value;
  }
}

LYNX_PROP_DEFINE("ios-scrolls-to-top", iosScrollsToTop, BOOL) { self.view.scrollsToTop = value; }

LYNX_PROP_DEFINE("upper-threshold", setUpperThreshold, CGFloat) {
  self.scrollEventEmitter.scrollUpperThreshold =
      requestReset ? DEFAULT_SCROLL_UPPER_THRESHOLD : value;
}

LYNX_PROP_DEFINE("lower-threshold", setLowerThreshold, CGFloat) {
  self.scrollEventEmitter.scrollLowerThreshold =
      requestReset ? DEFAULT_SCROLL_LOWER_THRESHOLD : value;
}

LYNX_PROP_DEFINE("experimental-ios-gesture-frequency-increasing", exprIOSIncreaseFrequency, BOOL) {
  // On iOS, if the default scrolling capability of UIScrollView in UIKit is not used, the CPU
  // frequency may be reduced, resulting in slower rendering of <list-item>. To increase the CPU
  // main frequency, in the gesture scenario, we attempt to turn on the scrolling of UIScrollView,
  // but we do not consume the scrolling dispatched from the system to be compatible with this
  // scenario.
  [self setIncreaseFrequencyWithGesture:value];
}

LYNX_PROP_DEFINE("upper-threshold-item-count", setUpperThresholdItemCount, NSInteger) {
  self.scrollUpperThresholdItemCount =
      requestReset ? DEFAULT_SCROLL_UPPER_THRESHOLD_ITEM_COUNT : value;
}

LYNX_PROP_DEFINE("lower-threshold-item-count", setLowerThresholdItemCount, NSInteger) {
  self.scrollLowerThresholdItemCount =
      requestReset ? DEFAULT_SCROLL_LOWER_THRESHOLD_ITEM_COUNT : value;
}

LYNX_PROP_DEFINE("scroll-bar-enable", setScrollBarEnable, BOOL) {
  BOOL val = requestReset ? YES : value;
  UIScrollView *view = self.view;
  view.showsVerticalScrollIndicator = val;
  view.showsHorizontalScrollIndicator = val;
}

LYNX_PROP_DEFINE("bounces", setBounces, BOOL) {
  BOOL val = requestReset ? YES : value;
  UIScrollView *view = self.view;
  view.bounces = val;
}

LYNX_PROP_DEFINE("needs-visible-cells", setNeedsVisibleCells, BOOL) {
  self.needsVisibleCells = requestReset ? NO : value;
}

LYNX_PROP_DEFINE("column-count", setColumnCount, NSInteger) {
  if (requestReset) {
    value = 1;
  }
  if (value <= 0) {
    value = 1;
  }
  self.numberOfColumns = value;
  LynxCollectionInvalidationContext *context =
      [[LynxCollectionInvalidationContext alloc] initWithNumberOfColumnsChanging:value];
  [self.view.collectionViewLayout invalidateLayoutWithContext:context];
}

LYNX_PROP_DEFINE("list-main-axis-gap", setListMainAxisGap, CGFloat) {
  if (requestReset) {
    value = 0;
  }
  if (value < 0) {
    value = 0;
  }
  self.mainAxisGap = value;
  LynxCollectionInvalidationContext *context =
      [[LynxCollectionInvalidationContext alloc] initWithMainAxisGapChanging:value];
  [self.view.collectionViewLayout invalidateLayoutWithContext:context];
}

LYNX_PROP_DEFINE("list-cross-axis-gap", setListCrossAxisGap, CGFloat) {
  if (requestReset) {
    value = 0;
  }
  if (value < 0) {
    value = 0;
  }
  self.crossAxisGap = value;
  LynxCollectionInvalidationContext *context =
      [[LynxCollectionInvalidationContext alloc] initWithCrossAxisGapChanging:value];
  [self.view.collectionViewLayout invalidateLayoutWithContext:context];
}

LYNX_PROP_DEFINE("list-type", setListType, NSString *) {
  if (requestReset) {
    value = @"flow";
  }

  LynxCollectionViewLayoutType type = LynxCollectionViewLayoutFlow;

  if ([value isEqualToString:@"waterfall"]) {
    type = LynxCollectionViewLayoutWaterfall;
  } else if ([value isEqualToString:@"flow"]) {
    type = LynxCollectionViewLayoutFlow;
  }

  LynxCollectionInvalidationContext *context =
      [[LynxCollectionInvalidationContext alloc] initWithLayoutTypeSwitching:type];
  [self.view.collectionViewLayout invalidateLayoutWithContext:context];
}

LYNX_PROP_DEFINE("update-animation", setUpdateAnimation, NSString *) {
  NSSet<NSString *> *optionSet = [NSSet setWithArray:[value componentsSeparatedByString:@" "]];
  self.enableUpdateAnimation = [optionSet containsObject:@"default"];
  if (self.enableUpdateAnimation) {
    if ([optionSet containsObject:@"cell-fade-in"]) {
      self.cellUpdateAnimationType = LynxCollectionCellUpdateAnimationTypeFadeIn;
    } else if ([optionSet containsObject:@"cell-animation-disable"]) {
      self.cellUpdateAnimationType = LynxCollectionCellUpdateAnimationTypeDisable;
    } else {
      self.cellUpdateAnimationType = LynxCollectionCellUpdateAnimationTypeNone;
    }
  }
  LynxCollectionInvalidationContext *context = [[LynxCollectionInvalidationContext alloc]
      initWithResetAnimationTo:self.enableUpdateAnimation];
  [self.view.collectionViewLayout invalidateLayoutWithContext:context];
}

LYNX_PROP_DEFINE("enable-scroll", setScrollEnabled, BOOL) { self.view.scrollEnabled = value; }
LYNX_PROP_DEFINE("touch-scroll", setTouchEnabled, BOOL) { self.view.scrollEnabled = value; }

LYNX_PROP_DEFINE("sticky", setEnableSticky, BOOL) { [self.layout setEnableSticky:value]; }

LYNX_PROP_DEFINE("sticky-offset", setStickyOffset, CGFloat) { [self.layout setStickyOffset:value]; }

LYNX_PROP_DEFINE("initial-scroll-index", setInitialScrollIndex, NSInteger) {
  self.initialScrollIndex = value;
}

/**
 * @name: scroll-index
 * @description: Every time it is set, it will take effect, while "initial-scroll-index" will only
 *take effect when it is set for the first time
 * @standardAction: keep
 * @supportVersion: 2.14
 **/
LYNX_PROP_DEFINE("scroll-index", setUpdatedScrollIndex, NSInteger) {
  self.updatedScrollIndex = value;
}

LYNX_PROP_DEFINE("paging-enabled", setPageEnabled, BOOL) {
  if (self.pagingAlignFactor == kInvalidSnapFactor) {
    self.view.pagingEnabled = value;
  }
}

LYNX_PROP_DEFINE("item-snap", setPagingAlignment, NSDictionary *) {
  if (value.count) {
    CGFloat factor = [value[@"factor"] doubleValue];
    CGFloat offset = [value[@"offset"] doubleValue];
    if (factor < 0 || factor > 1) {
      [NSException raise:@"item-snap arguments invalid!"
                  format:@"The factor should be constrained to the range of [0,1]."];
      [self.context
          reportLynxError:
              [LynxError
                  lynxErrorWithCode:ECLynxComponentListInvalidPropsArg
                            message:@"item-snap invalid!"
                      fixSuggestion:@"The factor should be constrained to the range of [0,1]."
                              level:LynxErrorLevelWarn]];
      factor = 0;
    }
    self.view.pagingEnabled = NO;
    self.pagingAlignFactor = factor;
    self.pagingAlignOffset = offset;
    self.view.decelerationRate = UIScrollViewDecelerationRateFast;
  } else {
    self.pagingAlignFactor = kInvalidSnapFactor;
    self.view.decelerationRate = UIScrollViewDecelerationRateNormal;
  }
}

LYNX_PROP_DEFINE("ios-index-as-z-index", setIndexAsZIndex, BOOL) {
  [self.layout setIndexAsZIndex:value];
}

/**
 * @name: enable-rtl
 * @description: If set to false, layout would not use rtl mode
 * @category: experimental
 * @standardAction: keep
 * @supportVersion: 2.10
 **/
LYNX_PROP_DEFINE("enable-rtl", setEnableRtl, BOOL) { self.enableRtl = value; }

LYNX_PROP_SETTER("ios-enable-align-height", setEnableAlignHeight, BOOL) {
  [self.layout setEnableAlignHeight:value];
}

LYNX_PROP_SETTER("ios-fix-offset-from-start", setFixOffsetFromStart, BOOL) {
  [self.layout setFixSelfSizingOffsetFromStart:value];
}

/**
 * @name: ios-forbid-single-sided-bounce
 * @description: 'upperBounce' can only forbid bounces effect at top or left (right for
 *RTL); 'lowerBounce' can only forbid bounces effect at bottom or right (left for RTL). iOS uses
 *bounces to implement refresh-view. This prop can be used when you need a refresh-view without
 *bounces on the other side.
 * @category: different
 * @standardAction: keep
 * @supportVersion: 2.11
 **/
LYNX_PROP_SETTER("ios-forbid-single-sided-bounce", setIosForbiddenSingleSidedBounce, NSString *) {
  if ([value isEqualToString:@"upperBounce"]) {
    self.bounceForbiddenDirection = LynxForbiddenUpper;
  } else if ([value isEqualToString:@"lowerBounce"]) {
    self.bounceForbiddenDirection = LynxForbiddenLower;
  } else if ([value isEqualToString:@"none"]) {
    self.bounceForbiddenDirection = LynxForbiddenNone;
  }
}

LYNX_PROP_SETTER("use-old-sticky", setUseOldSticky, BOOL) { [self.layout setUseOldSticky:value]; }

LYNX_PROP_SETTER("vertical-orientation", setHorizontalLayout, BOOL) {
  [self.layout setHorizontalLayout:!value];
  if (value) {
    self.layoutOrientation = LynxListLayoutOrientationVertical;
  } else {
    self.layoutOrientation = LynxListLayoutOrientationHorizontal;
  }
  self.view.scrollY = value;
  self.scroll.horizontalLayout = !value;
  self.view.alwaysBounceVertical = value;
  self.scrollEventEmitter.horizontalLayout = !value;
  self.scrollEventEmitter.helper.horizontalLayout = !value;
}

LYNX_PROP_SETTER("ios-fixed-content-offset", setFixedContentOffset, BOOL) {
  self.fixedContentOffset = value;
  self.layout.needsAdjustContentOffsetForSelfSizingCells = value;
}

LYNX_PROP_SETTER("ios-enable-adjust-offset-for-selfsizing",
                 setEnableAdjustContentOfssetForSelfSizing, BOOL) {
  self.layout.needsAdjustContentOffsetForSelfSizingCells = value;
}

LYNX_PROP_SETTER("ios-update-valid-layout", setUpdateValidLayout, BOOL) {
  self.layout.needUpdateValidLayoutAttributesAfterDiff = value;
}

LYNX_PROP_SETTER("ios-scroll-emitter-helper", setScrollEmitterHelper, BOOL) {
  if (value) {
    self.scrollEventEmitter.helper =
        [[LynxListScrollEventEmitterHelper alloc] initWithEmitter:self.scrollEventEmitter];
  } else {
    self.scrollEventEmitter.helper = nil;
  }
}

LYNX_PROP_SETTER("sticky-with-bounces", setStickyWithBounces, BOOL) {
  [self.layout setStickyWithBounces:value];
}

LYNX_PROP_SETTER("ios-no-recursive-layout", setNoRecursiveLayout, BOOL) {
  self.noRecursiveLayout = value;
  if (!value) {
    [LynxFeatureCounter count:LynxFeatureObjcListIosRecursiveLayout
                   instanceId:[self.context instanceId]];
  }
}

LYNX_PROP_SETTER("ios-force-reload-data", setForceReloadData, BOOL) {
  self.forceReloadData = value;
}

LYNX_PROP_DEFINE("internal-cell-appear-notification", setInternalCellAppearNotification, BOOL) {
  if (requestReset) {
    value = false;
  }
  self.needsInternalCellAppearNotification = value;
}

LYNX_PROP_DEFINE("internal-cell-disappear-notification", setInternalCellDisappearNotification,
                 BOOL) {
  if (requestReset) {
    value = false;
  }
  self.needsInternalCellDisappearNotification = value;
}

LYNX_PROP_DEFINE("internal-cell-prepare-for-reuse-notification",
                 setInternalCellPrepareForReuseNotification, BOOL) {
  if (requestReset) {
    value = false;
  }
  self.needsInternalCellPrepareForReuseNotification = value;
}

LYNX_PROP_DEFINE("should-request-state-restore", setShouldRequestStateRestore, BOOL) {
  if (requestReset) {
    value = false;
  }
  self.needsInternalCellPrepareForReuseNotification = value;
  self.needsInternalCellDisappearNotification = value;
  self.needsInternalCellAppearNotification = value;
}

LYNX_PROP_SETTER("list-platform-info", setCurComponents, NSDictionary *) {
  self.diffResultFromTasm = value[@"diffResult"];
  self.curComponents = value;
  self.listNoDiffInfo = nil;
  [self markIsNewArch];
}

LYNX_PROP_SETTER("update-list-info", updateListActionInfo, NSDictionary *) {
  self.listNoDiffInfo = value;
  self.diffResultFromTasm = nil;
  [self markIsNewArch];
}

/**
 * @name: enable-async-list
 * @description: enable create node async on List
 * @category: temporary
 * @standardAction: offline
 * @supportVersion: 2.10
 **/
LYNX_PROP_DEFINE("enable-async-list", enableAsyncList, BOOL) { self.enableAsyncList = value; }

/**
 * @name: list-debug-info-level
 * @description: enable list to send extra debug event with key debug info
 * @category: standardized
 * @supportVersion: 2.18
 **/
LYNX_PROP_SETTER("list-debug-info-level", debugInfoLevel, NSInteger) {
  if (requestReset) {
    value = 0;
  }
  self.debugInfoLevel = value;
}

@end
