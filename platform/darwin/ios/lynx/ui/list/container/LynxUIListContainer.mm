// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxComponentRegistry.h>
#import <Lynx/LynxLog.h>
#import <Lynx/LynxPropsProcessor.h>
#import <Lynx/LynxScrollEventManager.h>
#import <Lynx/LynxScrollView.h>
#import <Lynx/LynxSubErrorCode.h>
#import <Lynx/LynxUI+Fluency.h>
#import <Lynx/LynxUI+Gesture.h>
#import <Lynx/LynxUI+Internal.h>
#import <Lynx/LynxUIImage.h>
#import <Lynx/LynxUIListContainer.h>
#import <Lynx/LynxUIMethodProcessor.h>
#import <Lynx/UIScrollView+Lynx.h>

#import <Lynx/LynxUIContext+Internal.h>

#import "LynxListItemHelper.h"
#import "LynxListStickyManager.h"

#import "core/public/list_container_proxy.h"
#import "core/public/list_engine_proxy.h"

#include <math.h>

static const CGFloat kLynxListContainerInvalidScrollEstimatedOffset = -1.0;
static const CGFloat kInvalidSnapFactor = -1;
static const CGFloat kFadeInAnimationDefaultDuration = 0.1;
static const CGFloat kLynxListAutomaticMaxFlingRatio = CGFLOAT_MAX;
static const NSInteger kDefaultMaxSnapCount = 1;
typedef NS_ENUM(NSInteger, LynxListScrollState) {
  LynxListScrollStateIdle = 1,
  LynxListScrollStateDragging = 2,
  LynxListScrollStateFling = 3,
  LynxListScrollStateScrollAnimation = 4,
};

@interface LynxListContainerComponentWrapper () <LynxListItemWrapper>
@end

@implementation LynxListContainerComponentWrapper

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

@interface LynxUIListContainer () <LynxListItemHelperOwner, LynxListStickyManagerOwner> {
  std::unique_ptr<lynx::shell::ListContainerProxy> _listContainerProxy;
}
@property(nonatomic, assign) BOOL verticalOrientation;
@property(nonatomic, strong) NSArray<NSString *> *itemKeys;
@property(nonatomic, copy) LynxUIMethodCallbackBlock scrollToCallback;
@property(nonatomic, assign) NSInteger scrollRequestId;
// True if this setContentOffset is triggered by onNodeReady
@property(nonatomic, assign) BOOL shouldBlockScrollByListContainer;
@property(nonatomic, assign) CGPoint previousContentOffset;
@property(nonatomic, strong) LynxListItemHelper *itemHelper;
@property(nonatomic, strong) LynxListStickyManager *stickyManager;
@property(nonatomic, assign) BOOL enableFadeInAnimation;
@property(nonatomic, assign) BOOL enableBatchRender;
@property(nonatomic, assign) BOOL enableInsertPlatformViewOperation;
@property(nonatomic, assign) CGFloat updateAnimationFadeInDuration;
@property(nonatomic, assign) CGFloat maxFlingDistanceRatio;
@property(nonatomic, assign) BOOL isInScrollToPosition;
@property(nonatomic, assign) BOOL isInAutoScroll;
@property(nonatomic, assign) LynxListScrollState currentScrollState;
@property(nonatomic, assign) BOOL enableNeedVisibleItemInfo;
@property(nonatomic, assign) NSInteger pagingMaxSnapCount;
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
    _itemHelper = [[LynxListItemHelper alloc] initWithOwner:self];
    _stickyManager = [[LynxListStickyManager alloc] initWithOwner:self];
    _verticalOrientation = YES;  // Default Vertical
    self.enableScrollY = YES;
    _pagingAlignFactor = kInvalidSnapFactor;
    _pagingMaxSnapCount = kDefaultMaxSnapCount;
    _updateAnimationFadeInDuration = kFadeInAnimationDefaultDuration;
    _maxFlingDistanceRatio = -1;
    _listNativeStateCache = [NSMutableDictionary dictionary];
    _initialFlushPropCache = [NSMutableDictionary dictionary];
    _enableBatchRender = NO;
    _enableInsertPlatformViewOperation = NO;
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

- (NSMutableArray *)restoreNativeStateBlockArray {
  if (!_restoreNativeStateBlockArray) {
    _restoreNativeStateBlockArray = [NSMutableArray array];
  }
  return _restoreNativeStateBlockArray;
}

- (BOOL)isScrollContainer {
  return YES;
}

- (void)onNodeReady {
  [super onNodeReady];

  if (_restoreNativeStateBlockArray) {
    NSArray *blockArray = _restoreNativeStateBlockArray;
    _restoreNativeStateBlockArray = nil;
    for (RestoreNativeStateBlock restoreNativeState in blockArray) {
      restoreNativeState();
    }
  }

  auto listNodeInfoFetcher = self.context.fetcher;
  auto listEngineProxyPtr = [listNodeInfoFetcher getListEngineProxyPtr];
  if (_listContainerProxy == nullptr && listEngineProxyPtr != 0) {
    auto listEngineProxy = reinterpret_cast<lynx::shell::ListEngineProxy *>(listEngineProxyPtr);
    _listContainerProxy = std::make_unique<lynx::shell::ListContainerProxy>(listEngineProxy);
  }

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

    // The filtering logic here has a relatively big risk because the
    // contentOffset might be modified externally through KVO. However, if there
    // is no filtering, incorrect behavior will occur in the refresh scenario.
    // TODO(xiamengfei.moonface): Use a way similar to Android to replace
    // MJRefresh to implement the pull-down refreshing.
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
  [self.stickyManager updateStickyItems];
}

- (void)detachedFromWindow {
  [self setScrollState:LynxListScrollStateIdle];
  // TODO(fangzhou.fz) should we stop autoscroll here?
  self.isInScrollToPosition = NO;
}

- (void)updateContentSize {
  // Override the old updateContentSize and do nothing. Use contentSize from
  // c++.
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
}

- (void)removeChild:(id)child atIndex:(NSInteger)index {
  [super removeChild:child atIndex:index];
}

- (void)propsDidUpdate {
  [super propsDidUpdate];
  [self.stickyManager propsDidUpdate];
}

#pragma mark component update
- (void)onComponentLayoutUpdated:(LynxUIComponent *)component {
  LynxListContainerComponentWrapper *wrapper =
      (LynxListContainerComponentWrapper *)component.view.superview;
  if ([wrapper isKindOfClass:LynxListContainerComponentWrapper.class]) {
    [self.itemHelper updateLayoutForComponent:component inWrapper:wrapper];
    wrapper.layer.zPosition = component.zIndex;
  }
  [self.stickyManager didLayoutComponent:component];
}

- (void)onAsyncComponentLayoutUpdated:(LynxUIComponent *)component
                          operationID:(int64_t)operationID {
  // If enable batch render, no need to insert platform view in
  // finishLayoutOperation()
  if (!self.enableBatchRender && !self.enableInsertPlatformViewOperation) {
    [self insertListComponent:component];
  }
}

- (void)insertListComponent:(LynxUIComponent *)component {
  if (![component.view.superview isKindOfClass:LynxListContainerComponentWrapper.class]) {
    // Insert platform view.
    LynxListContainerComponentWrapper *wrapper = [[LynxListContainerComponentWrapper alloc] init];
    wrapper.holdingUI = component;
    [component.view removeFromSuperview];
    [self.itemHelper attachComponent:component toWrapper:wrapper];
    [self.view addSubview:wrapper];
    wrapper.layer.zPosition = component.zIndex;
    LynxUIListContainerRestartAnimationRecursively(component);
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
    [self.delegate insertListComponent:component wrapper:wrapper];
  }
  [self.stickyManager didAttachComponent:component];
}

- (void)removeListComponent:(LynxUIComponent *)component {
  [self.stickyManager willDetachComponent:component];
  if (component.view.superview.superview == self.view) {
    [component.view.superview removeFromSuperview];
    [component.view removeFromSuperview];
    [self.delegate removeListComponent:component];
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
  if ([value isKindOfClass:NSDictionary.class] && value.count) {
    CGFloat factor = [value[@"factor"] doubleValue];
    if (factor < 0 || factor > 1) {
      [NSException raise:@"item-snap arguments invalid!"
                  format:@"The factor should be constrained to the range of [0,1]."];
      [self.context
          reportLynxError:[LynxError lynxErrorWithCode:ECLynxComponentListInvalidPropsArg
                                               message:@"item-snap invalid!"
                                         fixSuggestion:@"The factor should be constrained "
                                                       @"to the range of [0,1]."
                                                 level:LynxErrorLevelWarn]];
      factor = 0;
    }
    CGFloat offset = [value[@"offset"] doubleValue];
    NSInteger maxSnapCount = kDefaultMaxSnapCount;
    id maxSnapCountValue = value[@"maxSnapCount"];
    if (maxSnapCountValue) {
      maxSnapCount = [maxSnapCountValue integerValue];
      if (maxSnapCount < kDefaultMaxSnapCount) {
        [self.context
            reportLynxError:[LynxError lynxErrorWithCode:ECLynxComponentListInvalidPropsArg
                                                 message:@"item-snap invalid!"
                                           fixSuggestion:@"The maxSnapCount should be greater "
                                                         @"than 0."
                                                   level:LynxErrorLevelWarn]];
        maxSnapCount = kDefaultMaxSnapCount;
      }
    }
    self.view.pagingEnabled = NO;
    self.pagingAlignFactor = factor;
    self.pagingAlignOffset = offset;
    self.pagingMaxSnapCount = maxSnapCount;
    self.view.decelerationRate = UIScrollViewDecelerationRateFast;
  } else {
    self.pagingAlignFactor = kInvalidSnapFactor;
    self.pagingMaxSnapCount = kDefaultMaxSnapCount;
    self.view.decelerationRate = UIScrollViewDecelerationRateNormal;
  }
}

LYNX_PROP_SETTER("list-container-info", setStickyInfo, NSDictionary *) {
  _itemKeys = value[@"itemkeys"];
  [self.stickyManager setStickyStartIndexes:value[@"stickyStart"] endIndexes:value[@"stickyEnd"]];
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
LYNX_PROP_SETTER("sticky", setEnableSticky, BOOL) { self.stickyManager.enabled = value; }

LYNX_PROP_SETTER("sticky-offset", setStickyOffset, CGFloat) { self.stickyManager.offset = value; }

LYNX_PROP_SETTER("enable-fade-in-animation", setEnableFadeInAnimation, BOOL) {
  self.enableFadeInAnimation = value;
}

LYNX_PROP_SETTER("update-animation-fade-in-duration", setUpdateAnimationFadeInDuration, NSInteger) {
  self.updateAnimationFadeInDuration = value / 1000.;
}

LYNX_PROP_SETTER("enable-insert-platform-view-operation", setEnableInsertPlatformViewOperation,
                 BOOL) {
  self.enableInsertPlatformViewOperation = value;
}

LYNX_PROP_SETTER("need-visible-item-info", setNeedVisibleItemInfo, BOOL) {
  self.enableNeedVisibleItemInfo = value;
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
        [self sendScrollStateChangeEvent:scrollState];
        [self sendScrollEndEvent];
      }
      [self postFluencyEventWithInfo:[self infoWithScrollView:self.view
                                                     selector:@selector
                                                     (scrollerDidEndDecelerating:)]];
      break;
    }
    case LynxListScrollStateFling: {
      [self sendScrollStateChangeEvent:scrollState];
      LynxScrollInfo *info = [self infoWithScrollView:self.view
                                             selector:@selector(scrollerDidEndDragging:
                                                                        willDecelerate:)];
      info.decelerate = YES;
      [self postFluencyEventWithInfo:info];
      break;
    }
    case LynxListScrollStateDragging: {
      [self sendScrollStateChangeEvent:scrollState];
      [self
          postFluencyEventWithInfo:[self infoWithScrollView:self.view
                                                   selector:@selector(scrollerWillBeginDragging:)]];

      break;
    }
    case LynxListScrollStateScrollAnimation: {
      [self sendScrollStateChangeEvent:scrollState];
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

    // We can not move less than 1/scale pt in every frame, cause contentOffset
    // will align to 1/scale.
    // TODO(xiamengfei.moonface): [ResizableWindowScale] Check whether this should use window or
    // physical screen metrics.
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
                               [strongSelf sendScrollEndEvent];
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

  // Perform parameter parsing (item-key first)

  NSInteger position = 0;
  if ([params objectForKey:@"position"]) {
    position = ((NSNumber *)[params objectForKey:@"position"]).intValue;
  }
  if ([params objectForKey:@"index"]) {
    position = ((NSNumber *)[params objectForKey:@"index"]).intValue;
  }

  NSString *itemKey = [params objectForKey:@"itemKey"];
  NSInteger resolvedPosition = position;
  if (itemKey.length) {
    NSInteger idx = [self getIndexFromItemKey:itemKey];
    if (idx >= 0) {
      resolvedPosition = idx;
    }
  }

  CGFloat offset = ((NSNumber *)[params objectForKey:@"offset"]).doubleValue;

  BOOL smooth = [[params objectForKey:@"smooth"] boolValue];

  if (resolvedPosition < 0 || (NSUInteger)resolvedPosition >= self.itemKeys.count) {
    if (callback) {
      callback(kUIMethodOperationError, @"position < 0 or position >= data count");
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

  if (smooth) {
    self.scrollToCallback = callback;
  }

  // Tell ListElement that we want scroll to some position
  [self scrollToPosition:resolvedPosition offset:offset align:(int)alignTo smooth:smooth];

  auto listNodeInfoFetcher = self.context.fetcher;

  if (!listNodeInfoFetcher) {
    if (callback) {
      callback(kUIMethodUnknown, @"List has been destroyed");
    }
  } else {
    if (!smooth) {
      // TODO(xiamengfei.moonface) Invoke callback after ListElement did scroll
      // on Most_On_Tasm
      callback(kUIMethodSuccess, nil);
    }
  }
}

- (void)scrollToPosition:(NSInteger)position
                  offset:(float)offset
                   align:(int)align
                  smooth:(BOOL)smooth {
  if (position < 0 || (NSUInteger)position >= self.itemKeys.count) {
    return;
  }

  // Stop the current scroll
  self.isInScrollToPosition = YES;
  [self.view setContentOffset:self.view.contentOffset animated:NO];
  self.isInScrollToPosition = NO;

  auto listNodeInfoFetcher = self.context.fetcher;
  // LynxShell and NodeInfoFetcher has been destroyed, just return.
  if (listNodeInfoFetcher == nil) {
    LLogError(@"LynxShell has been destroyed,when invoking scrollToPosition()");
    return;
  }

  // Tell ListElement that we want scroll to some position

  if (_listContainerProxy) {
    _listContainerProxy->ScrollToPosition(static_cast<int32_t>(self.sign), (int)position, offset,
                                          align, smooth);
  } else {
    [listNodeInfoFetcher scrollToPosition:static_cast<int32_t>(self.sign)
                                 position:(int)position
                                   offset:offset
                                    align:align
                                   smooth:smooth];
  }
}

- (void)updateScrollInfoWithEstimatedOffset:(CGFloat)estimatedOffset
                                     smooth:(BOOL)smooth
                                  scrolling:(BOOL)scrolling {
  // ListElement flush scrolling to platform
  ((LynxListContainerView *)(self.view)).scrollEstimatedOffset = estimatedOffset;
  if (!scrolling) {
    // Scroll will begin !

    // Workaround Logic: Stop current scroll if 600ms has passed
    NSInteger scrollRequestId = ++self.scrollRequestId;
    __weak __typeof(self) weakSelf = self;
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (600 * NSEC_PER_MSEC)),
                   dispatch_get_main_queue(), ^{
                     // Ensure that our scroll will be finished. It should be
                     // finished in 300ms, accroding to UIKit.

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
      [self sendScrollEndEvent];
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
  [self updateLayerMaskOnFrameChanged];

  auto listNodeInfoFetcher = self.context.fetcher;
  // If if the contentOffset was updated by targetContentOffset, which means now the contentOffset
  // is exactly the same with elementList, do not reenter ScrollByListContainer

  // LynxShell and NodeInfoFetcher has been destroyed, just return.
  if (listNodeInfoFetcher == nil) {
    LLogError(@"LynxShell has been destroyed,when invoking scrollViewDidScroll()");
    return;
  }

  if (!_shouldBlockScrollByListContainer) {
    // Before sending scrollByListContainer, previousContentOffset should be updated to avoid
    // scrollViewDidScroll->scrollByListContainer->onNodeReady->setContentOffset->scrollViewDidScroll
    // loop
    [self updatePreviousContentOffset];

    if (_listContainerProxy) {
      _listContainerProxy->ScrollByListContainer(
          (static_cast<int32_t>(self.sign)), [self clampToValidScrollEdge:NO],
          [self clampToValidScrollEdge:YES], self.view.contentOffset.x, self.view.contentOffset.y);
    } else {
      [listNodeInfoFetcher scrollByListContainer:(static_cast<int32_t>(self.sign))
                                               x:[self clampToValidScrollEdge:NO]
                                               y:[self clampToValidScrollEdge:YES]
                                       originalX:self.view.contentOffset.x
                                       originalY:self.view.contentOffset.y];
    }

    [self.stickyManager updateStickyItems];
  }
}

- (void)updatePreviousContentOffset {
  _previousContentOffset = CGPointMake(
      self.isRtl ? [self contentOffsetXRTL:self.view.contentOffset.x] : self.view.contentOffset.x,
      self.view.contentOffset.y);
}

- (CGFloat)clampToValidScrollEdge:(BOOL)isVertical {
  // The `contentInset` should not be took into account, cause that ListElement
  // will not recognize this iOS only feat
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
                                         : self.view.contentSize.width - self.frame.size.width);
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

          if (index >= (NSInteger)weakSelf.itemKeys.count) {
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
        maxSnapCount:weakSelf.pagingMaxSnapCount
        callback:^(NSInteger position, CGPoint offset) {
          [weakSelf sendSnapEventWithPosition:position
                         currentContentOffset:currentContentOffset
                                 targetOffset:offset];
        }];
    targetContentOffset->x = targetOffset.x;
    targetContentOffset->y = targetOffset.y;
  } else if (self.maxFlingDistanceRatio > 0) {
    CGFloat forwardFlingOffset = 0;
    CGFloat backwardFlingOffset = 0;
    CGFloat currentOffset =
        self.verticalOrientation ? scrollView.contentOffset.y : scrollView.contentOffset.x;

    if (self.maxFlingDistanceRatio == kLynxListAutomaticMaxFlingRatio) {
      forwardFlingOffset = [self getAvailableScrollOffsetFromSubviews:YES offset:currentOffset];
      backwardFlingOffset = [self getAvailableScrollOffsetFromSubviews:NO offset:currentOffset];
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

- (void)sendSnapEventWithPosition:(NSInteger)position
             currentContentOffset:(CGPoint)currentContentOffset
                     targetOffset:(CGPoint)targetOffset {
  if (position >= (NSInteger)self.itemKeys.count) {
    position = MAX(0, (NSInteger)self.itemKeys.count - 1);
  }
  [self.scrollEventManager sendScrollEvent:LynxEventSnap
                                scrollView:self.view
                                    detail:@{
                                      @"position" : @(position),
                                      @"currentScrollLeft" : @(currentContentOffset.x),
                                      @"currentScrollTop" : @(currentContentOffset.y),
                                      @"targetScrollLeft" : @(targetOffset.x),
                                      @"targetScrollTop" : @(targetOffset.y),
                                    }];
}

- (CGFloat)clampContentOffset:(CGFloat)offset
                        lower:(CGFloat)lower
                         size:(CGFloat)size
                       height:(CGFloat)height {
  offset = MIN(offset, size - height);
  offset = MAX(offset, lower);
  return offset;
}

- (CGFloat)getAvailableScrollOffsetFromSubviews:(BOOL)forward offset:(CGFloat)offset {
  if (self.view.subviews.count == 0) {
    return offset;
  }
  __block CGFloat max = CGFLOAT_MIN;
  __block CGFloat min = CGFLOAT_MAX;
  BOOL vertical = self.verticalOrientation;
  if (forward) {
    CGFloat contentSize = vertical ? self.view.contentSize.height : self.view.contentSize.width;
    CGFloat paddingEnd = vertical ? self.padding.bottom : self.padding.right;
    [self.view.subviews enumerateObjectsUsingBlock:^(__kindof UIView *_Nonnull obj, NSUInteger idx,
                                                     BOOL *_Nonnull stop) {
      if ([obj isKindOfClass:LynxListContainerComponentWrapper.class]) {
        max = MAX(max, vertical ? CGRectGetMaxY(obj.frame) : CGRectGetMaxX(obj.frame));
      }
    }];
    if (fabs(max + paddingEnd - contentSize) < CGFLOAT_EPSILON) {
      max = contentSize;
    }
    max = max - (vertical ? self.view.frame.size.height : self.view.frame.size.width);
  } else {
    CGFloat paddingStart = vertical ? self.padding.top : self.padding.left;
    [self.view.subviews enumerateObjectsUsingBlock:^(__kindof UIView *_Nonnull obj, NSUInteger idx,
                                                     BOOL *_Nonnull stop) {
      if ([obj isKindOfClass:LynxListContainerComponentWrapper.class]) {
        min = MIN(min, vertical ? CGRectGetMinY(obj.frame) : CGRectGetMinX(obj.frame));
      }
    }];
    if (fabs(min - paddingStart) < CGFLOAT_EPSILON) {
      min = 0.f;
    }
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

  // Mark finish scroll and notify ListElement to stop updating offset to
  // platform

  ((LynxListContainerView *)(self.view)).scrollEstimatedOffset =
      kLynxListContainerInvalidScrollEstimatedOffset;

  auto fetcher = self.context.fetcher;

  // LynxShell and NodeInfoFetcher has been destroyed, just return.
  if (fetcher == nil) {
    LLogError(@"LynxShell has been destroyed,when invoking scrollStopped()");
    return;
  }

  if (_listContainerProxy) {
    _listContainerProxy->ScrollStopped(static_cast<int32_t>(self.sign));
  } else {
    [fetcher scrollStopped:static_cast<int32_t>(self.sign)];
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
  return [self.itemHelper indexForItemKey:itemKey];
}

- (NSArray<LynxListContainerComponentWrapper *> *)visibleCells {
  return (NSArray<LynxListContainerComponentWrapper *> *)self.itemHelper.visibleItemWrappers;
}

- (NSArray<NSDictionary *> *)visibleCellsInfo {
  return self.itemHelper.visibleItemInfo;
}

- (CGFloat)contentOffsetXRTL:(CGFloat)contentOffsetX {
  // Caltulate RTL contentOffset
  return MAX(self.view.contentSize.width - contentOffsetX - self.view.frame.size.width, 0.f);
}

- (void)sendScrollEndEvent {
  NSMutableDictionary *detail = [[NSMutableDictionary alloc] init];
  detail[@"listWidth"] = @(CGRectGetWidth(self.view.frame));
  detail[@"listHeight"] = @(CGRectGetHeight(self.view.frame));
  if (self.isRtl && !self.verticalOrientation) {
    detail[@"scrollLeft"] = @([self clampToValidScrollEdge:NO]);
  }
  [self.scrollEventManager sendScrollEvent:LynxEventScrollEnd scrollView:self.view detail:detail];
}

- (void)sendScrollStateChangeEvent:(LynxListScrollState)scrollState {
  NSMutableDictionary *detail = [[NSMutableDictionary alloc] init];
  detail[@"state"] = @(scrollState);
  if (self.enableNeedVisibleItemInfo) {
    NSArray *attachedCellsArray = [self visibleCellsInfo];
    detail[@"attachedCells"] = attachedCellsArray;
  }
  if (self.isRtl && !self.verticalOrientation) {
    detail[@"scrollLeft"] = @([self clampToValidScrollEdge:NO]);
  }
  [self.scrollEventManager sendScrollEvent:LynxEventScrollStateChange
                                scrollView:self.view
                                    detail:detail];
}

- (id<LynxEventTarget>)findHitTestTarget:(CGPoint)point withEvent:(UIEvent *)event {
  id<LynxEventTarget> hitTarget = [self.stickyManager findHitTargetAtPoint:point withEvent:event];
  if (hitTarget) {
    return hitTarget;
  }
  return [self.itemHelper findHitTargetAtPoint:point withEvent:event];
}

- (LynxListContainerComponentWrapper *)visibleWrapperAtPoint:(CGPoint)point {
  return (LynxListContainerComponentWrapper *)[self.itemHelper firstSubviewAtPoint:point];
}

- (id<LynxEventTarget>)hitTest:(CGPoint)point withEvent:(UIEvent *)event {
  [self resetInterceptGesture];
  if (self.context.enableEventRefactor) {
    return [self findHitTestTarget:point withEvent:event] ?: self;
  } else {
    id<LynxEventTarget> target = [self.stickyManager findHitTargetAtPoint:point withEvent:event];
    if (target) {
      return target;
    }
    LynxListContainerComponentWrapper *wrapper = [self visibleWrapperAtPoint:point];
    if (!wrapper) return self;
    return [wrapper.holdingUI hitTest:[self.view convertPoint:point toView:wrapper.holdingUI.view]
                            withEvent:event];
  }
}

#pragma mark LynxListItemHelperOwner

- (UIScrollView *)scrollViewForListItemHelper {
  return self.view;
}

- (BOOL)isVerticalForListItemHelper {
  return self.verticalOrientation;
}

- (NSArray<NSString *> *)itemKeysForListItemHelper {
  return self.itemKeys;
}

#pragma mark LynxListStickyManagerOwner

- (UIScrollView *)scrollViewForListStickyManager {
  return self.view;
}

- (BOOL)isVerticalForListStickyManager {
  return self.verticalOrientation;
}

- (NSArray<NSString *> *)itemKeysForListStickyManager {
  return self.itemKeys;
}

- (LynxScrollEventManager *)eventManagerForListStickyManager {
  return self.scrollEventManager;
}

void LynxUIListContainerRestartAnimationRecursively(LynxUI *ui) {
  if ([ui isKindOfClass:[LynxUIImage class]]) {
    LynxUIImage *uiImage = (LynxUIImage *)ui;

    if (uiImage.isAnimated) {
      // to avoid GIF from being stopped after reuse, we manually call startAnimating here.
      [uiImage startAnimating];
    }
  }
  [ui.animationManager restartAnimation];
  [ui.children enumerateObjectsUsingBlock:^(id _Nonnull obj, NSUInteger idx, BOOL *_Nonnull stop) {
    LynxUIListContainerRestartAnimationRecursively(obj);
  }];
}

@end
