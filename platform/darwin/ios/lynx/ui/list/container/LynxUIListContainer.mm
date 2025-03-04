// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxUIListContainer.h"
#import "LynxComponentRegistry.h"
#import "LynxPropsProcessor.h"
#import "LynxScrollEventManager.h"
#import "LynxScrollView.h"
#import "LynxSubErrorCode.h"
#import "LynxUI+Fluency.h"
#import "LynxUI+Internal.h"
#import "LynxUIContext+Internal.h"
#import "LynxUIMethodProcessor.h"
#import "UIScrollView+Lynx.h"
#include "core/shell/lynx_shell.h"

static const CGFloat kLynxListContainerInvalidScrollEstimatedOffset = -1.0;
static const CGFloat kInvalidSnapFactor = -1;
static const CGFloat kFadeInAnimationDefaultDuration = 0.1;
static const CGFloat kLynxListAutomaticMaxFlingRatio = CGFLOAT_MAX;
typedef NS_ENUM(NSInteger, LynxListScrollState) {
  LynxListScrollStateIdle = 1,
  LynxListScrollStateDragging = 2,
  LynxListScrollStateFling = 3,
  LynxListScrollStateScrollAnimation = 4,
};

@interface LynxListContainerComponentWrapper : UIView
@property(nonatomic, weak) LynxUIComponent *holdingUI;
@end

@implementation LynxListContainerComponentWrapper

- (void)addListItemView:(UIView *)listItemView withFrame:(CGRect)frame {
  self.frame = frame;
  listItemView.frame = [LynxListContainerComponentWrapper getAlignedFrame:frame];
  [self addSubview:listItemView];
}

+ (CGRect)getAlignedFrame:(CGRect)frame {
  return CGRectMake(0, 0, frame.size.width, frame.size.height);
}

@end

@interface LynxListContainerView : LynxScrollView
@property(nonatomic, assign) BOOL scrollToLower;
@property(nonatomic, assign) BOOL verticalOrientation;
@property(nonatomic, assign) CGFloat scrollEstimatedOffset;
@property(nonatomic, weak) LynxUIListContainer *ui;
@property(nonatomic, assign, setter=setLynxListAdjustingContentOffset:,
          getter=isLynxListAdjustingContentOffset) BOOL adjustingContentOffsetInternally;
@end

@implementation LynxListContainerView

- (void)willMoveToWindow:(UIWindow *)newWindow {
  [super willMoveToWindow:newWindow];
  if (!newWindow) {
    [self.ui detachedFromWindow];
  }
}

- (void)setContentOffset:(CGPoint)contentOffset {
  if (_scrollEstimatedOffset != kLynxListContainerInvalidScrollEstimatedOffset) {
    // Ensure that our offset does not exceed the estimated value.

    if (_verticalOrientation) {
      if (_scrollToLower && contentOffset.y > _scrollEstimatedOffset) {
        contentOffset.y = _scrollEstimatedOffset;
      }
      if (!_scrollToLower && contentOffset.y < _scrollEstimatedOffset) {
        contentOffset.y = _scrollEstimatedOffset;
      }
    } else {
      if (_scrollToLower && contentOffset.x > _scrollEstimatedOffset) {
        contentOffset.x = _scrollEstimatedOffset;
      }
      if (!_scrollToLower && contentOffset.x < _scrollEstimatedOffset) {
        contentOffset.x = _scrollEstimatedOffset;
      }
    }
  }

  if (self.contentOffset.y != contentOffset.y || self.contentOffset.x != contentOffset.x) {
    [super setContentOffset:contentOffset];
  }
}

@end

@interface LynxUIListContainer ()
@property(nonatomic, assign) BOOL verticalOrientation;
@property(nonatomic, strong) NSArray<NSString *> *itemKeys;
@property(nonatomic, copy) LynxUIMethodCallbackBlock scrollToCallback;
@property(nonatomic, assign) NSInteger scrollRequestId;
// True if this setContentOffset is triggered by onNodeReady
@property(nonatomic, assign) BOOL shouldBlockScrollByListContainer;
@property(nonatomic, assign) CGPoint previousContentOffset;
// Sticky properties
@property(nonatomic, assign) BOOL enableListSticky;
@property(nonatomic, assign) CGFloat stickyOffset;
@property(nonatomic, strong) NSArray<NSNumber *> *stickyTopIndexes;
@property(nonatomic, strong) NSMutableDictionary<NSNumber *, LynxUIComponent *> *stickyTopItems;
@property(nonatomic, strong) NSArray<NSNumber *> *stickyBottomIndexes;
@property(nonatomic, strong) NSMutableDictionary<NSNumber *, LynxUIComponent *> *stickyBottomItems;
@property(nonatomic, weak) LynxUI *prevStickyTopItem;
@property(nonatomic, weak) LynxUI *prevStickyBottomItem;
@property(nonatomic, assign) CGFloat pagingAlignFactor;
@property(nonatomic, assign) CGFloat pagingAlignOffset;
@property(nonatomic, assign) BOOL enableFadeInAnimation;
@property(nonatomic, assign) BOOL enableBatchRender;
@property(nonatomic, assign) CGFloat updateAnimationFadeInDuration;
@property(nonatomic, assign) CGFloat maxFlingDistanceRatio;
@property(nonatomic, assign) BOOL isInScrollToPosition;
@property(nonatomic, assign) BOOL isInAutoScroll;
@property(nonatomic, assign) LynxListScrollState currentScrollState;
// Experimental
@property(nonatomic, assign) BOOL disableFilterScroll;

@end

@implementation LynxUIListContainer

#if LYNX_LAZY_LOAD
LYNX_LAZY_REGISTER_UI("list-container")
#else
LYNX_REGISTER_UI("list-container")
#endif

#pragma mark base
- (instancetype)init {
  self = [super init];
  if (self) {
    _stickyTopItems = [NSMutableDictionary dictionary];
    _stickyBottomItems = [NSMutableDictionary dictionary];
    _verticalOrientation = YES;  // Default Vertical
    self.enableScrollY = YES;
    _pagingAlignFactor = kInvalidSnapFactor;
    _updateAnimationFadeInDuration = kFadeInAnimationDefaultDuration;
    _maxFlingDistanceRatio = -1;
    _listNativeStateCache = [NSMutableDictionary dictionary];
    _initialFlushPropCache = [NSMutableDictionary dictionary];
    _enableBatchRender = NO;
    _currentScrollState = LynxListScrollStateIdle;
  }
  return self;
}

- (UIView *)createView {
  LynxListContainerView *scrollView = [LynxListContainerView new];
  scrollView.autoresizesSubviews = NO;
  scrollView.clipsToBounds = YES;
  scrollView.showsVerticalScrollIndicator = NO;
  scrollView.showsHorizontalScrollIndicator = NO;
  scrollView.scrollEnabled = YES;
  scrollView.delegate = self;
  scrollView.bounces = YES;
  scrollView.enableNested = NO;
  scrollView.verticalOrientation = YES;
  scrollView.scrollEstimatedOffset = kLynxListContainerInvalidScrollEstimatedOffset;
  scrollView.ui = self;
  if (@available(iOS 11.0, *)) {
    scrollView.contentInsetAdjustmentBehavior = UIScrollViewContentInsetAdjustmentNever;
  }

  return scrollView;
}

- (BOOL)isScrollContainer {
  return YES;
}

- (void)onNodeReady {
  [super onNodeReady];
  if (_needAdjustContentOffset) {
    _needAdjustContentOffset = NO;
    // Avoid adjustContentOffsetIfnecessary called from system
    _shouldBlockScrollByListContainer = YES;
    CGPoint contentOffsetBeforeSizeChange = _previousContentOffset;

    BOOL contentSizeChanged = NO;
    BOOL deltaChanged = NO;

    if (_verticalOrientation) {
      contentSizeChanged = _targetContentSize != self.view.contentSize.height;
      deltaChanged = _targetDelta.y != 0;
      CGFloat contentWidth = self.frame.size.width - self.padding.left - self.padding.right;
      self.view.contentSize = CGSizeMake(contentWidth, _targetContentSize);
    } else {
      contentSizeChanged = _targetContentSize != self.view.contentSize.width;
      deltaChanged = _targetDelta.x != 0;
      CGFloat contentHeight = self.frame.size.height - self.padding.top - self.padding.bottom;
      self.view.contentSize = CGSizeMake(_targetContentSize, contentHeight);
    }
    // contentSize change may cause contentOffset adjustment by system call.
    _previousContentOffset = CGPointMake(contentOffsetBeforeSizeChange.x + _targetDelta.x,
                                         contentOffsetBeforeSizeChange.y + _targetDelta.y);

    // The filtering logic here has a relatively big risk because the contentOffset might be
    // modified externally through KVO. However, if there is no filtering, incorrect behavior will
    // occur in the refresh scenario.
    // TODO(xiamengfei.moonface): Use a way similar to Android to replace MJRefresh to implement the
    // pull-down refreshing.
    if (self.disableFilterScroll || (contentSizeChanged || deltaChanged)) {
      [self.view setLynxListAdjustingContentOffset:YES];
      self.view.contentOffset = CGPointMake(
          self.isRtl ? [self contentOffsetXRTL:_previousContentOffset.x] : _previousContentOffset.x,
          _previousContentOffset.y);
      [self.view setLynxListAdjustingContentOffset:NO];
    }
    _targetDelta = CGPointZero;
  }
  _shouldBlockScrollByListContainer = NO;
  [self updateStickyTops];
  [self updateStickyBottoms];
}

- (void)detachedFromWindow {
  [self setScrollState:LynxListScrollStateIdle];
  // TODO(fangzhou.fz) should we stop autoscroll here?
  self.isInScrollToPosition = NO;
}

- (void)updateContentSize {
  // Override the old updateContentSize and do nothing. Use contentSize from c++.
}

- (void)insertChild:(LynxUI *)child atIndex:(NSInteger)index {
  if (child != nil) {
    child.parent = self;
    if ((NSUInteger)index > self.children.count) {
      [self.children addObject:child];
    } else {
      [self.children insertObject:child atIndex:index];
    }
  }
  LynxUIComponent *componentChild = (LynxUIComponent *)child;
  componentChild.layoutObserver = self;

  if (self.enableListSticky) {
    NSInteger indexFromItemKey = [self getIndexFromItemKey:componentChild.itemKey];

    [self updateStickyInfoForInsertedChild:componentChild
                               stickyItems:self.stickyTopItems
                             stickyIndexes:self.stickyTopIndexes
                                     index:indexFromItemKey];

    [self updateStickyInfoForInsertedChild:componentChild
                               stickyItems:self.stickyBottomItems
                             stickyIndexes:self.stickyBottomIndexes
                                     index:indexFromItemKey];
  }
}

- (void)removeChild:(id)child atIndex:(NSInteger)index {
  [super removeChild:child atIndex:index];

  [self updateStickyInfoForDeletedChild:child stickyItems:self.stickyTopItems];
  [self updateStickyInfoForDeletedChild:child stickyItems:self.stickyBottomItems];
}

#pragma mark component update
- (void)onComponentLayoutUpdated:(LynxUIComponent *)component {
  LynxListContainerComponentWrapper *wrapper =
      (LynxListContainerComponentWrapper *)component.view.superview;
  if ([wrapper isKindOfClass:LynxListContainerComponentWrapper.class]) {
    [wrapper addListItemView:component.view withFrame:component.frame];
    wrapper.layer.zPosition = component.zIndex;
  }
}

- (void)onAsyncComponentLayoutUpdated:(LynxUIComponent *)component
                          operationID:(int64_t)operationID {
  // If enable batch render, no need to insert platform view in finishLayoutOperation()
  if (!self.enableBatchRender) {
    [self insertListComponent:component];
  }
}

- (void)insertListComponent:(LynxUIComponent *)component {
  if (![component.view.superview isKindOfClass:LynxListContainerComponentWrapper.class]) {
    // Insert platform view.
    LynxListContainerComponentWrapper *wrapper = [[LynxListContainerComponentWrapper alloc] init];
    wrapper.holdingUI = component;
    [component.view removeFromSuperview];
    [wrapper addListItemView:component.view withFrame:component.frame];
    [self.view addSubview:wrapper];
    wrapper.layer.zPosition = component.zIndex;
    // Invoke fade-in animation.
    if (self.enableFadeInAnimation) {
      component.view.alpha = 0;
      [UIView animateWithDuration:_updateAnimationFadeInDuration
                            delay:0
                          options:UIViewAnimationOptionAllowUserInteraction
                       animations:^{
                         component.view.alpha = 1;
                       }
                       completion:^(BOOL finished){

                       }];
    }
  }
  if (self.enableListSticky) {
    NSInteger indexFromItemKey = [self getIndexFromItemKey:component.itemKey];
    [self updateStickyInfoForUpdatedChild:component
                              stickyItems:self.stickyTopItems
                            stickyIndexes:self.stickyTopIndexes
                                    index:indexFromItemKey];

    [self updateStickyInfoForUpdatedChild:component
                              stickyItems:self.stickyBottomItems
                            stickyIndexes:self.stickyBottomIndexes
                                    index:indexFromItemKey];
  }
}

#pragma mark prop setters

LYNX_PROP_SETTER("experimental-disable-filter-scroll", setFilterScroll, BOOL) {
  self.disableFilterScroll = value;
}

LYNX_PROP_SETTER("experimental-max-fling-distance-ratio", setMaxFlingDistanceRatio, NSObject *) {
  if ([value isKindOfClass:NSString.class] && [@"auto" isEqualToString:(NSString *)value]) {
    self.maxFlingDistanceRatio = kLynxListAutomaticMaxFlingRatio;
  } else if ([value isKindOfClass:NSNumber.class]) {
    self.maxFlingDistanceRatio = ((NSNumber *)value).floatValue;
  }
}

LYNX_PROP_SETTER("item-snap", setPagingAlignment, NSDictionary *) {
  if (value.count) {
    CGFloat factor = [value[@"factor"] doubleValue];
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
    CGFloat offset = [value[@"offset"] doubleValue];
    self.view.pagingEnabled = NO;
    self.pagingAlignFactor = factor;
    self.pagingAlignOffset = offset;
    self.view.decelerationRate = UIScrollViewDecelerationRateFast;
  } else {
    self.pagingAlignFactor = kInvalidSnapFactor;
    self.view.decelerationRate = UIScrollViewDecelerationRateNormal;
  }
}

LYNX_PROP_SETTER("list-container-info", setStickyInfo, NSDictionary *) {
  _stickyTopIndexes = value[@"stickyTop"];
  _stickyBottomIndexes = value[@"stickyBottom"];
  _itemKeys = value[@"itemkeys"];
}

LYNX_PROP_SETTER("experimental-batch-render-strategy", setBatchRenderStrategy, NSInteger) {
  self.enableBatchRender = value > 0;
}

LYNX_PROP_SETTER("vertical-orientation", setVerticalOrientation, BOOL) {
  _verticalOrientation = value;
  self.enableScrollY = value;
  ((LynxListContainerView *)self.view).verticalOrientation = value;
}

LYNX_PROP_SETTER("scroll-orientation", setScrollOrientation, NSString *) {
  if ([value isKindOfClass:NSString.class] && [@"vertical" isEqualToString:(NSString *)value]) {
    _verticalOrientation = YES;
  } else if ([value isKindOfClass:NSString.class] &&
             [@"horizontal" isEqualToString:(NSString *)value]) {
    _verticalOrientation = NO;
  } else {
    _verticalOrientation = YES;
  }
  self.enableScrollY = _verticalOrientation;
  ((LynxListContainerView *)self.view).verticalOrientation = _verticalOrientation;
}

LYNX_PROP_SETTER("ios-scrolls-to-top", iosScrollsToTop, BOOL) {
  ((LynxListContainerView *)self.view).scrollsToTop = value;
}

// Sticky for horizontal layout is not supported.
LYNX_PROP_SETTER("sticky", setEnableSticky, BOOL) { self.enableListSticky = value; }
LYNX_PROP_SETTER("sticky-offset", setStickyOffset, CGFloat) { self.stickyOffset = value; }

LYNX_PROP_SETTER("enable-fade-in-animation", setEnableFadeInAnimation, BOOL) {
  self.enableFadeInAnimation = value;
}
LYNX_PROP_SETTER("update-animation-fade-in-duration", setUpdateAnimationFadeInDuration, NSInteger) {
  self.updateAnimationFadeInDuration = value / 1000.;
}

- (void)setEnableScroll:(BOOL)value requestReset:(BOOL)requestReset {
  if (requestReset) {
    value = YES;
  }
  ((LynxListContainerView *)self.view).scrollEnabled = value;
}

- (void)setScrollBarEnable:(BOOL)value requestReset:(BOOL)requestReset {
  if (requestReset) {
    value = NO;
  }
  self.view.showsVerticalScrollIndicator = value;
  self.view.showsHorizontalScrollIndicator = value;
}

- (void)setScrollState:(LynxListScrollState)scrollState {
  if (self.currentScrollState == scrollState) {
    return;
  }
  switch (scrollState) {
    case LynxListScrollStateIdle: {
      if (!self.isInAutoScroll && !self.isInScrollToPosition) {
        [self.scrollEventManager sendScrollEvent:LynxEventScrollStateChange
                                      scrollView:self.view
                                          detail:@{
                                            @"state" : @(LynxListScrollStateIdle),
                                          }];
        [self.scrollEventManager sendScrollEvent:LynxEventScrollEnd scrollView:self.view];
      }
      [self postFluencyEventWithInfo:[self infoWithScrollView:self.view
                                                     selector:@selector
                                                     (scrollerDidEndDecelerating:)]];
      break;
    }
    case LynxListScrollStateFling: {
      [self.scrollEventManager sendScrollEvent:LynxEventScrollStateChange
                                    scrollView:self.view
                                        detail:@{
                                          @"state" : @(LynxListScrollStateFling),
                                        }];
      LynxScrollInfo *info = [self infoWithScrollView:self.view
                                             selector:@selector(scrollerDidEndDragging:
                                                                        willDecelerate:)];
      info.decelerate = YES;
      [self postFluencyEventWithInfo:info];
      break;
    }
    case LynxListScrollStateDragging: {
      [self.scrollEventManager sendScrollEvent:LynxEventScrollStateChange
                                    scrollView:self.view
                                        detail:@{
                                          @"state" : @(LynxListScrollStateDragging),
                                        }];
      [self
          postFluencyEventWithInfo:[self infoWithScrollView:self.view
                                                   selector:@selector(scrollerWillBeginDragging:)]];
      break;
    }
    case LynxListScrollStateScrollAnimation: {
      [self.scrollEventManager sendScrollEvent:LynxEventScrollStateChange
                                    scrollView:self.view
                                        detail:@{
                                          @"state" : @(LynxListScrollStateScrollAnimation),
                                        }];
      [self
          postFluencyEventWithInfo:[self infoWithScrollView:self.view
                                                   selector:@selector(scrollerWillBeginDragging:)]];
      break;
    }
    default:
      break;
  }
  self.currentScrollState = scrollState;
}

#pragma mark sticky

- (void)updateStickyInfoForInsertedChild:(LynxUIComponent *)child
                             stickyItems:
                                 (NSMutableDictionary<NSNumber *, LynxUIComponent *> *)stickyItems
                           stickyIndexes:(NSArray<NSNumber *> *)stickyIndexes
                                   index:(NSInteger)indexFromItemKey {
  if (!self.enableListSticky) {
    return;
  }

  // Insert child to sticky array

  [stickyIndexes
      enumerateObjectsUsingBlock:^(NSNumber *_Nonnull obj, NSUInteger idx, BOOL *_Nonnull stop) {
        if (obj.integerValue == indexFromItemKey) {
          stickyItems[@(indexFromItemKey)] = child;
        }
      }];
}

- (void)updateStickyInfoForDeletedChild:(LynxUIComponent *)child
                            stickyItems:
                                (NSMutableDictionary<NSNumber *, LynxUIComponent *> *)stickyItems {
  if (!self.enableListSticky) {
    return;
  }

  // Remove child from sticky array

  [[stickyItems copy] enumerateKeysAndObjectsUsingBlock:^(
                          NSNumber *_Nonnull key, LynxUI *_Nonnull obj, BOOL *_Nonnull stop) {
    if (obj == child) {
      [stickyItems removeObjectForKey:key];
      *stop = YES;
    }
  }];
}

- (void)updateStickyInfoForUpdatedChild:(LynxUIComponent *)child
                            stickyItems:
                                (NSMutableDictionary<NSNumber *, LynxUIComponent *> *)stickyItems
                          stickyIndexes:(NSArray<NSNumber *> *)stickyIndexes
                                  index:(NSInteger)indexFromItemKey {
  if (!self.enableListSticky) {
    return;
  }

  // Update child in sticky array

  // TODO(xiamengfei.moonface): Handle recycle sticky later

  [stickyIndexes
      enumerateObjectsUsingBlock:^(NSNumber *_Nonnull obj, NSUInteger idx, BOOL *_Nonnull stop) {
        if (obj.integerValue == indexFromItemKey) {
          stickyItems[@(indexFromItemKey)] = child;

        } else if (stickyItems[obj] == child) {
          [stickyItems removeObjectForKey:obj];
        }
      }];
}

- (void)updateStickyTops {
  if (!self.enableListSticky) {
    return;
  }

  // TODO(xiamengfei.monface) Support sticky with bounces for iOS

  CGFloat offset = MAX(0, self.view.contentOffset.y) + self.stickyOffset;
  LynxUI *stickyTopItem = nil;
  LynxUI *nextStickyTopItem = nil;

  // enumerate from bottom to top to find sticky top item
  for (NSNumber *topIndex in self.stickyTopIndexes.reverseObjectEnumerator) {
    LynxUIComponent *top = self.stickyTopItems[topIndex];
    if (!top) {
      continue;
    }
    if (CGRectGetMinY(top.frame) > offset) {
      // cache potential next sticky item
      nextStickyTopItem = top;
      // hold its position
      top.view.superview.frame = top.frame;
      top.view.frame = [LynxListContainerComponentWrapper getAlignedFrame:top.frame];
      top.view.superview.layer.zPosition = ((LynxUIComponent *)top).zIndex;
    } else if (stickyTopItem) {
      // sticky top item found, hold upper sticky top's position
      top.view.superview.frame = top.frame;
      top.view.frame = [LynxListContainerComponentWrapper getAlignedFrame:top.frame];
      top.view.superview.layer.zPosition = ((LynxUIComponent *)top).zIndex;
    } else {
      stickyTopItem = top;
    }
  }

  if (stickyTopItem) {
    if (self.prevStickyTopItem != stickyTopItem) {
      [self.scrollEventManager sendScrollEvent:LynxEventStickyTop
                                    scrollView:self.view
                                        detail:@{
                                          @"top" : ((LynxUIComponent *)stickyTopItem).itemKey,
                                        }];
      self.prevStickyTopItem = stickyTopItem;
    }

    CGFloat stickyTopY = offset;

    if (nextStickyTopItem) {
      CGFloat nextStickyTopItemDistanceFromOffset = CGRectGetMinY(nextStickyTopItem.frame) - offset;

      CGFloat squashStickyTopDelta =
          CGRectGetHeight(stickyTopItem.frame) - nextStickyTopItemDistanceFromOffset;

      if (squashStickyTopDelta > 0) {
        // need push sticky top item to upper
        stickyTopY -= squashStickyTopDelta;
      }
    }

    stickyTopItem.view.superview.frame = {{
                                              stickyTopItem.frame.origin.x,
                                              stickyTopY,
                                          },
                                          stickyTopItem.frame.size};
    stickyTopItem.view.frame =
        [LynxListContainerComponentWrapper getAlignedFrame:stickyTopItem.frame];
    [self.view bringSubviewToFront:stickyTopItem.view.superview];
    stickyTopItem.view.superview.layer.zPosition = NSIntegerMax;
  }
}

- (void)updateStickyBottoms {
  if (!self.enableListSticky) {
    return;
  }
  CGFloat offset = MIN(self.view.contentOffset.y,
                       MAX(0, self.view.contentSize.height - self.view.frame.size.height)) +
                   CGRectGetHeight(self.view.frame) - self.stickyOffset;
  LynxUI *stickyBottomItem = nil;
  LynxUI *nextStickyBottomItem = nil;

  // enumrate from top to bottom to find sticky top item
  for (NSNumber *bottomIndex in self.stickyBottomIndexes) {
    LynxUI *bottom = self.stickyBottomItems[bottomIndex];
    if (!bottom) {
      continue;
    }
    if (CGRectGetMaxY(bottom.frame) < offset) {
      // cache potential next sticky item
      nextStickyBottomItem = bottom;
      // hold its position
      bottom.view.superview.frame = bottom.frame;
      bottom.view.frame = [LynxListContainerComponentWrapper getAlignedFrame:bottom.frame];
      bottom.view.superview.layer.zPosition = ((LynxUIComponent *)bottom).zIndex;
    } else if (stickyBottomItem) {
      // sticky bottom item found, hold upper sticky top's position
      bottom.view.superview.frame = bottom.frame;
      bottom.view.frame = [LynxListContainerComponentWrapper getAlignedFrame:bottom.frame];
      bottom.view.superview.layer.zPosition = ((LynxUIComponent *)bottom).zIndex;
    } else {
      stickyBottomItem = bottom;
    }
  }

  if (stickyBottomItem) {
    if (self.prevStickyBottomItem != stickyBottomItem) {
      [self.scrollEventManager sendScrollEvent:LynxEventStickyBottom
                                    scrollView:self.view
                                        detail:@{
                                          @"bottom" : ((LynxUIComponent *)stickyBottomItem).itemKey,
                                        }];
      self.prevStickyBottomItem = stickyBottomItem;
    }

    CGFloat stickyTopY = offset - CGRectGetHeight(stickyBottomItem.frame);

    if (nextStickyBottomItem) {
      CGFloat nextStickyBottomItemDistanceFromOffset =
          offset - CGRectGetMaxY(nextStickyBottomItem.frame);

      CGFloat squashStickyBottomDelta =
          CGRectGetHeight(stickyBottomItem.frame) - nextStickyBottomItemDistanceFromOffset;

      if (squashStickyBottomDelta > 0) {
        stickyTopY += squashStickyBottomDelta;
      }
    }

    stickyBottomItem.view.superview.frame = {{
                                                 stickyBottomItem.frame.origin.x,
                                                 stickyTopY,
                                             },
                                             stickyBottomItem.frame.size};
    stickyBottomItem.view.frame =
        [LynxListContainerComponentWrapper getAlignedFrame:stickyBottomItem.frame];
    [self.view bringSubviewToFront:stickyBottomItem.view.superview];
    stickyBottomItem.view.superview.layer.zPosition = NSIntegerMax;
  }
}

#pragma mark scroll methods
LYNX_UI_METHOD(autoScroll) {
  if ([[params objectForKey:@"start"] boolValue]) {
    self.isInAutoScroll = YES;
    if (self.scrollToCallback) {
      self.scrollToCallback(kUIMethodParamInvalid,
                            @"the scroll has stopped, triggered by auto scroll");
      self.scrollToCallback = nil;
    }

    CGFloat rate = [self toPtWithUnitValue:[params objectForKey:@"rate"] fontSize:0];
    NSInteger preferredFramesPerSecond = 1000 / 16;

    // We can not move less than 1/scale pt in every frame, cause contentOffset will align to
    // 1/scale.
    while (ABS(rate / preferredFramesPerSecond) < 1.0 / UIScreen.mainScreen.scale) {
      preferredFramesPerSecond -= 1;
      if (preferredFramesPerSecond == 0) {
        if (callback) {
          self.isInAutoScroll = NO;
          callback(kUIMethodParamInvalid, @"rate is too small to scroll");
        }
        return;
      }
    };

    NSTimeInterval interval = 1.0 / preferredFramesPerSecond;
    rate *= interval;
    LynxScrollViewTouchBehavior behavior = LynxScrollViewTouchBehaviorNone;
    NSString *behaviorStr = [params objectForKey:@"touchBehavior"];
    if ([behaviorStr isEqualToString:@"forbid"]) {
      behavior = LynxScrollViewTouchBehaviorForbid;
    } else if ([behaviorStr isEqualToString:@"pause"]) {
      behavior = LynxScrollViewTouchBehaviorPause;
    } else if ([behaviorStr isEqualToString:@"stop"]) {
      behavior = LynxScrollViewTouchBehaviorStop;
    }

    BOOL autoStop = [([params objectForKey:@"autoStop"] ?: @(YES)) boolValue];

    __weak __typeof(self) weakSelf = self;
    [self.view autoScrollWithRate:rate
                         behavior:behavior
                         interval:interval
                         autoStop:autoStop
                         vertical:self.verticalOrientation
                         complete:^BOOL(BOOL scrollEnabledAtStart, BOOL completed) {
                           __strong __typeof(weakSelf) strongSelf = weakSelf;
                           if (strongSelf) {
                             if (completed) {
                               [strongSelf.scrollEventManager sendScrollEvent:LynxEventScrollEnd
                                                                   scrollView:strongSelf.view];
                             }
                           }
                           return strongSelf.view.scrollEnabled;
                         }];
    if (![self.view autoScrollWillReachToTheBounds]) {
      [self setScrollState:LynxListScrollStateScrollAnimation];
    }
  } else {
    [self.view stopScroll];
    self.isInAutoScroll = NO;
    [self setScrollState:LynxListScrollStateIdle];
  }
  if (callback) {
    callback(kUIMethodSuccess, nil);
  }
}

- (void)autoScrollStop {
  self.isInAutoScroll = NO;
  [self setScrollState:LynxListScrollStateIdle];
}

LYNX_UI_METHOD(scrollBy) {
  if (nil == callback) {
    return;
  }

  if (![params objectForKey:@"offset"]) {
    callback(kUIMethodParamInvalid,
             @{@"msg" : @"Invoke scrollBy failed due to index param is null"});
    return;
  }
  CGPoint preOffset = self.contentOffset;

  CGFloat offset = ((NSNumber *)[params objectForKey:@"offset"]).floatValue;
  NSArray<NSNumber *> *res = [self scrollBy:offset deltaY:offset];

  CGPoint postOffset = CGPointMake(res.firstObject.floatValue, res.lastObject.floatValue);

  int consumed_x = postOffset.x - preOffset.x;
  int consumed_y = postOffset.y - preOffset.y;
  int unconsumed_x = offset - consumed_x;
  int unconsumed_y = offset - consumed_y;

  NSDictionary *scrollResults = @{
    @"consumedX" : @(consumed_x),
    @"consumedY" : @(consumed_y),
    @"unconsumedX" : @(unconsumed_x),
    @"unconsumedY" : @(unconsumed_y)
  };

  callback(kUIMethodSuccess, scrollResults);
}

LYNX_UI_METHOD(scrollToPosition) {
  if (self.scrollToCallback) {
    self.scrollToCallback(kUIMethodUnknown,
                          @"the scroll has stopped, triggered by a new scrolling request");
    self.scrollToCallback = nil;
  }

  // Perform parameter parsing

  NSInteger position = 0;
  if ([params objectForKey:@"position"]) {
    position = ((NSNumber *)[params objectForKey:@"position"]).intValue;
  }
  if ([params objectForKey:@"index"]) {
    position = ((NSNumber *)[params objectForKey:@"index"]).intValue;
  }

  CGFloat offset = ((NSNumber *)[params objectForKey:@"offset"]).doubleValue;

  BOOL smooth = [[params objectForKey:@"smooth"] boolValue];

  if (position < 0 || (NSUInteger)position >= self.itemKeys.count) {
    if (callback) {
      callback(kUIMethodUnknown, @"position < 0 or position >= data count");
    }
    return;
  }

  NSInteger alignTo = 0;

  NSString *alignToStr = [params objectForKey:@"alignTo"];

  if ([alignToStr isEqualToString:@"top"]) {
    alignTo = 0;
  } else if ([alignToStr isEqualToString:@"middle"]) {
    alignTo = 1;
  } else if ([alignToStr isEqualToString:@"bottom"]) {
    alignTo = 2;
  }

  // Stop the current scroll
  self.isInScrollToPosition = YES;
  [self.view setContentOffset:self.view.contentOffset animated:NO];
  self.isInScrollToPosition = NO;

  // Tell ListElement that we want scroll to some position

  auto shellPtr = self.context.shellPtr;
  if (shellPtr) {
    if (smooth) {
      self.scrollToCallback = callback;
    }
    reinterpret_cast<lynx::shell::LynxShell *>(shellPtr)->ScrollToPosition(
        static_cast<int32_t>(self.sign), (int)position, offset, (int)alignTo, smooth);
    if (!smooth) {
      // TODO(xiamengfei.moonface) Invoke callback after ListElement did scroll on Most_On_Tasm
      callback(kUIMethodSuccess, nil);
    }
  } else {
    if (callback) {
      callback(kUIMethodUnknown, @"List has been destroyed");
    }
  }
}

- (void)updateScrollInfoWithEstimatedOffset:(CGFloat)estimatedOffset
                                     smooth:(BOOL)smooth
                                  scrolling:(BOOL)scrolling {
  // ListElement flush scrolling to platform

  NSInteger scrollRequestId = ++self.scrollRequestId;
  ((LynxListContainerView *)(self.view)).scrollEstimatedOffset = estimatedOffset;
  if (!scrolling) {
    // Scroll will begin !

    __weak __typeof(self) weakSelf = self;
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (600 * NSEC_PER_MSEC)),
                   dispatch_get_main_queue(), ^{
                     // Ensure that our scroll will be finished. It should be finished in 300ms,
                     // accroding to UIKit.

                     if (scrollRequestId == weakSelf.scrollRequestId &&
                         ((LynxListContainerView *)(weakSelf.view)).scrollEstimatedOffset !=
                             kLynxListContainerInvalidScrollEstimatedOffset) {
                       [weakSelf scrollStopped];
                     }
                   });

    ((LynxListContainerView *)(self.view)).scrollToLower =
        self.verticalOrientation ? (estimatedOffset > self.view.contentOffset.y)
                                 : (estimatedOffset > self.view.contentOffset.x);

    CGPoint target =
        CGPointMake(self.verticalOrientation ? self.view.contentOffset.x : estimatedOffset,
                    self.verticalOrientation ? estimatedOffset : self.view.contentOffset.y);

    if (smooth && !CGPointEqualToPoint(self.view.contentOffset, target)) {
      [self setScrollState:LynxListScrollStateScrollAnimation];
    }

    // Trigger scroll
    [self.view setContentOffset:target animated:smooth];

    LynxUIMethodCallbackBlock callback = self.scrollToCallback;

    if (callback && smooth) {
      dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (600 * NSEC_PER_MSEC)),
                     dispatch_get_main_queue(), ^{
                       if (callback == weakSelf.scrollToCallback) {
                         callback(kUIMethodSuccess, nil);
                         weakSelf.scrollToCallback = nil;
                       }
                     });
    } else {
      ((LynxListContainerView *)(self.view)).scrollEstimatedOffset =
          kLynxListContainerInvalidScrollEstimatedOffset;
      [self setScrollState:LynxListScrollStateIdle];
      // Send extra scrollend event in non smooth mode
      [self.scrollEventManager sendScrollEvent:LynxEventScrollEnd scrollView:self.view];
    }
  }
}

LYNX_UI_METHOD(getVisibleCells) {
  if (callback) {
    callback(kUIMethodSuccess, [self visibleCellsInfo]);
  }
}

#pragma mark delegate

- (BOOL)scrollViewShouldScrollToTop:(UIScrollView *)scrollView {
  if (scrollView.scrollsToTop && self.verticalOrientation) {
    return YES;
  }
  return NO;
}

- (void)scrollViewDidScroll:(UIScrollView *)scrollView {
  if (scrollView == self.view &&
      ![self.view respondToScrollViewDidScroll:self.view.gestureConsumer]) {
    return;
  }
  auto shellPtr = self.context.shellPtr;
  // If if the contentOffset was updated by targetContentOffset, which means now the contentOffset
  // is exactly the same with elementList, do not reenter ScrollByListContainer

  if (shellPtr && !_shouldBlockScrollByListContainer) {
    // Before sending scrollByListContainer, previousContentOffset should be updated to avoid
    // scrollViewDidScroll->scrollByListContainer->onNodeReady->setContentOffset->scrollViewDidScroll
    // loop
    [self updatePreviousContentOffset];
    reinterpret_cast<lynx::shell::LynxShell *>(shellPtr)->ScrollByListContainer(
        static_cast<int32_t>(self.sign), [self clampToValidScrollEdge:NO],
        [self clampToValidScrollEdge:YES], self.view.contentOffset.x, self.view.contentOffset.y);
  }
  [self updatePreviousContentOffset];
  [self updateStickyTops];
  [self updateStickyBottoms];
}

- (void)updatePreviousContentOffset {
  _previousContentOffset = CGPointMake(
      self.isRtl ? [self contentOffsetXRTL:self.view.contentOffset.x] : self.view.contentOffset.x,
      self.view.contentOffset.y);
}

- (CGFloat)clampToValidScrollEdge:(BOOL)isVertical {
  // The `contentInset` should not be took into account, cause that ListElement will not recognize
  // this iOS only feat
  if (isVertical) {
    CGFloat validOffsetY = MAX(0, self.view.contentOffset.y);
    validOffsetY = MIN(validOffsetY, [self orientationMaxScrollableDistance]);
    return validOffsetY;
  } else {
    CGFloat validOffsetX =
        self.isRtl ? [self contentOffsetXRTL:self.view.contentOffset.x] : self.view.contentOffset.x;
    validOffsetX = MAX(0, validOffsetX);
    validOffsetX = MIN(validOffsetX, [self orientationMaxScrollableDistance]);
    return validOffsetX;
  }
}

- (CGFloat)orientationMaxScrollableDistance {
  // The `contentInset` is not took into account, for ListElement only
  return MAX(0, self.verticalOrientation ? self.view.contentSize.height - self.frame.size.height
                                         : self.view.contentSize.width - self.frame.size.height);
}

- (CGFloat)orientationSize {
  return self.verticalOrientation ? self.view.frame.origin.y + self.view.frame.size.height
                                  : self.view.frame.origin.x + self.view.frame.size.width;
}

- (CGFloat)orientationContentSize {
  return self.verticalOrientation ? self.view.contentSize.height : self.view.contentSize.width;
}

#pragma mark scroll delegate
- (void)scrollViewDidEndDragging:(UIScrollView *)scrollView willDecelerate:(BOOL)decelerate {
  [self setScrollState:decelerate ? LynxListScrollStateFling : LynxListScrollStateIdle];
}

- (void)scrollViewWillEndDragging:(UIScrollView *)scrollView
                     withVelocity:(CGPoint)velocity
              targetContentOffset:(inout CGPoint *)targetContentOffset {
  if ([self.view stopDeceleratingIfNecessaryWithTargetContentOffset:targetContentOffset]) {
    return;
  }
  if (self.pagingAlignFactor != kInvalidSnapFactor) {
    CGPoint currentContentOffset = scrollView.contentOffset;

    NSMutableArray<UIView *> *subviews = [NSMutableArray array];

    for (UIView *subview in self.view.subviews) {
      if ([subview isKindOfClass:LynxListContainerComponentWrapper.class]) {
        [subviews addObject:subview];
      }
    }

    __weak typeof(self) weakSelf = self;

    CGPoint targetOffset = [scrollView targetContentOffset:*targetContentOffset
        withScrollingVelocity:velocity
        withVisibleItems:subviews
        getIndexFromView:^NSInteger(UIView *_Nonnull view) {
          if ([view isKindOfClass:LynxListContainerComponentWrapper.class]) {
            NSString *itemKey = ((LynxListContainerComponentWrapper *)view).holdingUI.itemKey;
            NSUInteger position = [weakSelf.itemKeys indexOfObject:itemKey];
            if (position == NSNotFound) {
              return -1;
            }
            return position;
          } else {
            return -1;
          }
        }
        getViewRectAtIndex:^CGRect(NSInteger index) {
          if (weakSelf.itemKeys.count <= 0) {
            return CGRectNull;
          }

          if (index >= (NSInteger)self.itemKeys.count) {
            index = weakSelf.itemKeys.count - 1;
          }
          if (index < 0) {
            index = 0;
          }

          NSString *targetItemKey = weakSelf.itemKeys[index];

          __block CGRect targetRect = CGRectNull;

          [weakSelf.view.subviews
              enumerateObjectsUsingBlock:^(__kindof UIView *_Nonnull obj, NSUInteger idx,
                                           BOOL *_Nonnull stop) {
                if ([obj isKindOfClass:LynxListContainerComponentWrapper.class]) {
                  NSString *itemKey = ((LynxListContainerComponentWrapper *)obj).holdingUI.itemKey;
                  if ([targetItemKey isEqualToString:itemKey]) {
                    targetRect = obj.frame;
                    *stop = YES;
                  }
                }
              }];

          return targetRect;
        }
        vertical:weakSelf.verticalOrientation
        rtl:[weakSelf isRtl]
        factor:weakSelf.pagingAlignFactor
        offset:weakSelf.pagingAlignOffset
        callback:^(NSInteger position, CGPoint offset) {
          if (position >= (NSInteger)weakSelf.itemKeys.count) {
            position = MAX(0, (NSInteger)weakSelf.itemKeys.count - 1);
          }
          [weakSelf.scrollEventManager sendScrollEvent:LynxEventSnap
                                            scrollView:weakSelf.view
                                                detail:@{
                                                  @"position" : @(position),
                                                  @"currentScrollLeft" : @(currentContentOffset.x),
                                                  @"currentScrollTop" : @(currentContentOffset.y),
                                                  @"targetScrollLeft" : @(offset.x),
                                                  @"targetScrollTop" : @(offset.y),
                                                }];
        }];
    targetContentOffset->x = targetOffset.x;
    targetContentOffset->y = targetOffset.y;
  } else if (self.maxFlingDistanceRatio > 0) {
    CGFloat forwardFlingOffset = 0;
    CGFloat backwardFlingOffset = 0;
    CGFloat currentOffset =
        self.verticalOrientation ? scrollView.contentOffset.y : scrollView.contentOffset.x;

    if (self.maxFlingDistanceRatio == kLynxListAutomaticMaxFlingRatio) {
      forwardFlingOffset = [self getAvailableScrollOffsetFromSubviews:YES];
      backwardFlingOffset = [self getAvailableScrollOffsetFromSubviews:NO];
    } else {
      CGFloat maxFlingDistanceInPoint =
          self.maxFlingDistanceRatio *
          (self.verticalOrientation ? self.view.frame.size.height : self.view.frame.size.width);
      forwardFlingOffset = currentOffset + maxFlingDistanceInPoint;
      backwardFlingOffset = currentOffset - maxFlingDistanceInPoint;
    }

    if (self.verticalOrientation) {
      if (targetContentOffset->y > scrollView.contentOffset.y) {
        targetContentOffset->y = MIN(targetContentOffset->y, forwardFlingOffset);
      } else {
        targetContentOffset->y = MAX(targetContentOffset->y, backwardFlingOffset);
      }
      targetContentOffset->y =
          [self clampContentOffset:targetContentOffset->y
                             lower:-scrollView.contentInset.top
                              size:scrollView.contentSize.height + scrollView.contentInset.bottom
                            height:scrollView.frame.size.height];
    } else {
      if (targetContentOffset->x > scrollView.contentOffset.x) {
        targetContentOffset->x = MIN(targetContentOffset->x, forwardFlingOffset);
      } else {
        targetContentOffset->x = MAX(targetContentOffset->x, backwardFlingOffset);
      }
      targetContentOffset->x =
          [self clampContentOffset:targetContentOffset->x
                             lower:-scrollView.contentInset.left
                              size:scrollView.contentSize.width + scrollView.contentInset.right
                            height:scrollView.frame.size.width];
    }
  }
}

- (CGFloat)clampContentOffset:(CGFloat)offset
                        lower:(CGFloat)lower
                         size:(CGFloat)size
                       height:(CGFloat)height {
  offset = MIN(offset, size - height);
  offset = MAX(offset, lower);
  return offset;
}

- (CGFloat)getAvailableScrollOffsetFromSubviews:(BOOL)forward {
  __block CGFloat max = CGFLOAT_MIN;
  __block CGFloat min = CGFLOAT_MAX;
  BOOL vertical = self.verticalOrientation;
  if (forward) {
    [self.view.subviews enumerateObjectsUsingBlock:^(__kindof UIView *_Nonnull obj, NSUInteger idx,
                                                     BOOL *_Nonnull stop) {
      if ([obj isKindOfClass:LynxListContainerComponentWrapper.class]) {
        max = MAX(max, vertical ? CGRectGetMaxY(obj.frame) : CGRectGetMaxX(obj.frame));
      }
    }];
    max = max - (vertical ? self.view.frame.size.height : self.view.frame.size.width);
  } else {
    [self.view.subviews enumerateObjectsUsingBlock:^(__kindof UIView *_Nonnull obj, NSUInteger idx,
                                                     BOOL *_Nonnull stop) {
      if ([obj isKindOfClass:LynxListContainerComponentWrapper.class]) {
        min = MIN(min, vertical ? CGRectGetMinY(obj.frame) : CGRectGetMinX(obj.frame));
      }
    }];
  }

  return forward ? max : min;
}

- (void)scrollViewDidEndDecelerating:(UIScrollView *)scrollView {
  [self setScrollState:LynxListScrollStateIdle];
}

- (void)scrollViewDidEndScrollingAnimation:(UIScrollView *)scrollView {
  [self scrollStopped];
  if (self.scrollToCallback) {
    self.scrollToCallback(kUIMethodSuccess, nil);
    self.scrollToCallback = nil;
  }
  if (!self.isInScrollToPosition) {
    [self setScrollState:LynxListScrollStateIdle];
  }
}

- (void)scrollViewWillBeginDragging:(UIScrollView *)scrollView {
  [self scrollStopped];
  [self setScrollState:LynxListScrollStateDragging];
  if (self.scrollToCallback) {
    self.scrollToCallback(kUIMethodUnknown,
                          @"the scroll has stopped, triggered by dragging events");
    self.scrollToCallback = nil;
  }
}

- (void)scrollStopped {
  if (((LynxListContainerView *)(self.view)).scrollEstimatedOffset ==
      kLynxListContainerInvalidScrollEstimatedOffset) {
    return;
  }

  // Mark finish scroll and notify ListElement to stop updating offset to platform

  ((LynxListContainerView *)(self.view)).scrollEstimatedOffset =
      kLynxListContainerInvalidScrollEstimatedOffset;
  auto shellPtr = self.context.shellPtr;
  if (shellPtr) {
    reinterpret_cast<lynx::shell::LynxShell *>(shellPtr)->ScrollStopped(
        static_cast<int32_t>(self.sign));
  }
}

#pragma mark native storage
- (BOOL)initialPropsFlushed:(NSString *)initialPropKey cacheKey:(NSString *)cacheKey {
  NSMutableSet *initialPropFlushSet = [self.initialFlushPropCache valueForKey:cacheKey];
  if (!initialPropFlushSet || initialPropFlushSet.count == 0) {
    return NO;
  }
  return [initialPropFlushSet containsObject:initialPropKey];
}

- (void)setInitialPropsHasFlushed:(NSString *)initialPropKey cacheKey:(nonnull NSString *)cacheKey {
  NSMutableSet *initialPropFlushSet =
      [self.initialFlushPropCache valueForKey:cacheKey] ?: [NSMutableSet set];
  if (initialPropFlushSet) {
    [initialPropFlushSet addObject:initialPropKey];
  }
  [self.initialFlushPropCache setValue:initialPropFlushSet forKey:cacheKey];
}

#pragma mark utils
- (NSInteger)getIndexFromItemKey:(NSString *)itemKey {
  __block NSInteger target = -1;
  [self.itemKeys
      enumerateObjectsUsingBlock:^(NSString *_Nonnull obj, NSUInteger idx, BOOL *_Nonnull stop) {
        if ([obj isEqualToString:itemKey]) {
          target = idx;
          *stop = YES;
        }
      }];
  return target;
}

- (BOOL)isVisibleCellVertical:(UIView *)cellView {
  CGFloat minY = CGRectGetMinY(cellView.frame);
  CGFloat maxY = CGRectGetMaxY(cellView.frame);
  CGFloat offsetMinY = self.view.contentOffset.y;
  CGFloat offsetMaxY = offsetMinY + CGRectGetHeight(self.view.frame);
  return ((minY <= offsetMinY && maxY >= offsetMinY) ||
          (minY <= offsetMaxY && maxY >= offsetMaxY) || (minY >= offsetMinY && maxY <= offsetMaxY));
}

- (BOOL)isVisibleCellHorizontal:(UIView *)cellView {
  CGFloat minX = CGRectGetMinX(cellView.frame);
  CGFloat maxX = CGRectGetMaxX(cellView.frame);
  CGFloat offsetMinX = self.view.contentOffset.x;
  CGFloat offsetMaxX = offsetMinX + CGRectGetWidth(self.view.frame);
  return ((minX <= offsetMinX && maxX >= offsetMinX) ||
          (minX <= offsetMaxX && maxX >= offsetMaxX) || (minX >= offsetMinX && maxX <= offsetMaxX));
}

- (NSArray<NSDictionary *> *)visibleCellsInfo {
  NSMutableArray<NSDictionary *> *attachedCells = [NSMutableArray array];
  [self.view.subviews
      enumerateObjectsUsingBlock:^(__kindof LynxListContainerComponentWrapper *_Nonnull obj,
                                   NSUInteger idx, BOOL *_Nonnull stop) {
        if ([obj isKindOfClass:LynxListContainerComponentWrapper.class]) {
          if (self.verticalOrientation ? [self isVisibleCellVertical:obj]
                                       : [self isVisibleCellHorizontal:obj]) {
            CGFloat cellTop = obj.frame.origin.y - self.view.contentOffset.y;
            CGFloat cellLeft = obj.frame.origin.x - self.view.contentOffset.x;
            [attachedCells addObject:@{
              @"id" : (obj.holdingUI.idSelector ?: @"unknown"),
              @"position" : @([self getIndexFromItemKey:obj.holdingUI.itemKey]),
              @"index" : @([self getIndexFromItemKey:obj.holdingUI.itemKey]),
              @"itemKey" : obj.holdingUI.itemKey ?: @"",
              @"top" : @(cellTop),
              @"bottom" : @(cellTop + obj.frame.size.height),
              @"left" : @(cellLeft),
              @"right" : @(cellLeft + obj.frame.size.width),
            }];
          }
        }
      }];

  [attachedCells sortUsingComparator:^NSComparisonResult(NSDictionary *_Nonnull lhs,
                                                         NSDictionary *_Nonnull rhs) {
    NSInteger lhsPosition = [lhs[@"position"] integerValue];
    NSInteger rhsPosition = [rhs[@"position"] integerValue];

    if (lhsPosition < rhsPosition) {
      return NSOrderedAscending;
    }

    if (lhsPosition > rhsPosition) {
      return NSOrderedDescending;
    }

    return NSOrderedSame;
  }];

  return attachedCells;
}

- (CGFloat)contentOffsetXRTL:(CGFloat)contentOffsetX {
  // Caltulate RTL contentOffset
  return MAX(self.view.contentSize.width - contentOffsetX - self.view.frame.size.width, 0.f);
}

- (id<LynxEventTarget>)findHitTestTarget:(CGPoint)point withEvent:(UIEvent *)event {
  __block id<LynxEventTarget> hitTarget = [self findHitTargetInStickyItems:point withEvent:event];
  if (hitTarget) {
    return hitTarget;
  }
  // if the zIndex of cells are assigned according to their index
  // we then use containsPoints to test each cell form the max zIndex to the min zIndex.
  NSArray<LynxListContainerComponentWrapper *> *visibleCells = self.view.subviews;
  NSArray<LynxListContainerComponentWrapper *> *visibleCellsSortedByZIndexReversely =
      [visibleCells sortedArrayUsingComparator:^NSComparisonResult(
                        id<LynxListCell> _Nonnull cellA, id<LynxListCell> _Nonnull cellB) {
        NSInteger ZPositionA = ((UIView *)cellA).layer.zPosition;
        NSInteger ZPositionB = ((UIView *)cellB).layer.zPosition;
        if (ZPositionA < ZPositionB) {
          return NSOrderedDescending;
        } else {
          return NSOrderedAscending;
        }
        return NSOrderedSame;
      }];
  [visibleCellsSortedByZIndexReversely
      enumerateObjectsUsingBlock:^(LynxListContainerComponentWrapper *_Nonnull obj, NSUInteger idx,
                                   BOOL *_Nonnull stop) {
        if ([obj isKindOfClass:[LynxListContainerComponentWrapper class]]) {
          CGPoint pointInCell = [obj.holdingUI.view convertPoint:point fromView:self.view];
          if ([obj.holdingUI containsPoint:pointInCell inHitTestFrame:obj.bounds]) {
            hitTarget = [obj.holdingUI hitTest:pointInCell withEvent:event];
            if (hitTarget) {
              *stop = YES;
            }
          }
        }
      }];
  return hitTarget;
}

- (LynxListContainerComponentWrapper *)visibleWrapperAtPoint:(CGPoint)point {
  __block LynxListContainerComponentWrapper *target;
  [self.view.subviews enumerateObjectsUsingBlock:^(LynxListContainerComponentWrapper *obj,
                                                   NSUInteger idx, BOOL *_Nonnull stop) {
    CGPoint pointInLayer = [self.view convertPoint:point toView:obj];
    BOOL contains = [obj.layer containsPoint:pointInLayer];
    if (contains) {
      target = obj;
      *stop = YES;
    }
  }];
  return target;
}

- (id<LynxEventTarget>)hitTest:(CGPoint)point withEvent:(UIEvent *)event {
  if (self.context.enableEventRefactor) {
    return [self findHitTestTarget:point withEvent:event] ?: self;
  } else {
    id<LynxEventTarget> target = [self findHitTargetInStickyItems:point withEvent:event];
    if (target) {
      return target;
    }
    LynxListContainerComponentWrapper *wrapper = [self visibleWrapperAtPoint:point];
    if (!wrapper) return self;
    return [wrapper.holdingUI hitTest:[self.view convertPoint:point toView:wrapper.holdingUI.view]
                            withEvent:event];
  }
}

- (id<LynxEventTarget>)findHitTargetInStickyItems:(CGPoint)point withEvent:(UIEvent *)event {
  __block id<LynxEventTarget> hitTarget;
  [_stickyTopItems enumerateKeysAndObjectsUsingBlock:^(NSNumber *_Nonnull key, LynxUI *_Nonnull obj,
                                                       BOOL *_Nonnull stop) {
    CGPoint pointInCell = [obj.view convertPoint:point fromView:self.view];
    if ([obj containsPoint:pointInCell inHitTestFrame:obj.view.bounds]) {
      hitTarget = [obj hitTest:pointInCell withEvent:event];
      if (hitTarget) {
        *stop = YES;
      }
    }
  }];
  // early return if stickyTop contains hitPoint
  if (hitTarget) {
    return hitTarget;
  }
  [_stickyBottomItems enumerateKeysAndObjectsUsingBlock:^(
                          NSNumber *_Nonnull key, LynxUI *_Nonnull obj, BOOL *_Nonnull stop) {
    CGPoint pointInCell = [obj.view convertPoint:point fromView:self.view];
    if ([obj containsPoint:pointInCell inHitTestFrame:obj.view.bounds]) {
      hitTarget = [obj hitTest:pointInCell withEvent:event];
      if (hitTarget) {
        *stop = YES;
      }
    }
  }];
  return hitTarget;
}

@end
