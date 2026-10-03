// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxUIListScroll.h"

#import <Lynx/LynxPropsProcessor.h>
#import <Lynx/LynxScrollEventManager.h>
#import <Lynx/LynxUI+Internal.h>
#import <Lynx/LynxUIContext+Internal.h>
#import <Lynx/LynxView+Internal.h>

#import "LynxListStickyManager.h"

@implementation LynxListScrollComponentWrapper
@end

@interface LynxUIListScroll () <LynxBaseScrollViewDelegate,
                                LynxListItemHelperOwner,
                                LynxListScrollHelperOwner,
                                LynxListStickyManagerOwner> {
  NSMutableArray<void (^)(void)> *_restoreNativeStateBlockArray;
}
@property(nonatomic, strong) LynxListItemHelper *itemHelper;
@property(nonatomic, strong) LynxListScrollHelper *scrollHelper;
@property(nonatomic, strong) LynxListStickyManager *stickyManager;
@property(nonatomic, strong)
    NSMutableDictionary<NSString *, NSMutableSet<NSString *> *> *initialFlushPropCache;
@property(nonatomic, strong, nullable) NSArray<NSString *> *itemKeys;
@property(nonatomic, strong, nullable) LynxScrollEventManager *scrollEventManager;
@property(nonatomic, assign) BOOL enableFadeInAnimation;
@property(nonatomic, assign) BOOL enableBatchRender;
@property(nonatomic, assign) BOOL enableInsertPlatformViewOperation;
@property(nonatomic, assign) NSTimeInterval updateAnimationFadeInDuration;
@property(nonatomic, assign) LynxBaseScrollViewScrollState currentScrollState;
@property(nonatomic, assign) CGPoint previousContentOffset;
@end

@implementation LynxUIListScroll

@synthesize listNativeStateCache = _listNativeStateCache;

- (instancetype)init {
  self = [super init];
  if (self) {
    _itemHelper = [[LynxListItemHelper alloc] initWithOwner:self];
    _scrollHelper = [[LynxListScrollHelper alloc] initWithOwner:self];
    _stickyManager = [[LynxListStickyManager alloc] initWithOwner:self];
    _listNativeStateCache = [NSMutableDictionary dictionary];
    _initialFlushPropCache = [NSMutableDictionary dictionary];
    _updateAnimationFadeInDuration = 0.1;
  }
  return self;
}

- (UIView *)createView {
  LynxListScrollView *view = [LynxListScrollView new];
  view.autoresizesSubviews = NO;
  view.clipsToBounds = YES;
  view.showsVerticalScrollIndicator = NO;
  view.showsHorizontalScrollIndicator = NO;
  view.bounces = NO;
  // BaseScrollView owns UIKit's delegate. List view-owner hooks are connected with the full
  // renderer scroll contract; this stage only consumes scroll callbacks for item/sticky state.
  view.scrollDelegate = self;
  if (@available(iOS 11.0, *)) {
    view.contentInsetAdjustmentBehavior = UIScrollViewContentInsetAdjustmentNever;
  }
  return view;
}

- (LynxListScrollView *)view {
  return (LynxListScrollView *)[super view];
}

- (BOOL)isScrollContainer {
  return YES;
}

- (void)insertChild:(LynxUI *)child atIndex:(NSInteger)index {
  if (!child) {
    return;
  }
  // Keep list-container compatibility: logical children do not attach their platform views
  // until the list engine requests insertion or the asynchronous layout callback allows it.
  child.parent = self;
  if ((NSUInteger)index > self.children.count) {
    [self.children addObject:child];
  } else {
    [self.children insertObject:child atIndex:index];
  }
  ((LynxUIComponent *)child).layoutObserver = self;
}

- (void)onComponentLayoutUpdated:(LynxUIComponent *)component {
  UIView *wrapper = component.view.superview;
  if ([wrapper isKindOfClass:LynxListScrollComponentWrapper.class]) {
    [self.itemHelper updateLayoutForComponent:component
                                    inWrapper:(LynxListScrollComponentWrapper *)wrapper];
    wrapper.layer.zPosition = component.zIndex;
  }
  [self.stickyManager didLayoutComponent:component];
}

- (void)onAsyncComponentLayoutUpdated:(LynxUIComponent *)component
                          operationID:(int64_t)operationID {
  if (!self.enableBatchRender && !self.enableInsertPlatformViewOperation) {
    [self insertListComponent:component];
  }
}

- (void)insertListComponent:(LynxUIComponent *)component {
  if (![component.view.superview isKindOfClass:LynxListScrollComponentWrapper.class]) {
    LynxListScrollComponentWrapper *wrapper = [LynxListScrollComponentWrapper new];
    wrapper.holdingUI = component;
    [component.view removeFromSuperview];
    [self.itemHelper attachComponent:component toWrapper:wrapper];
    [self.view addSubview:wrapper];
    wrapper.layer.zPosition = component.zIndex;
    if (self.enableFadeInAnimation) {
      component.view.alpha = 0;
      [UIView animateWithDuration:self.updateAnimationFadeInDuration
                            delay:0
                          options:UIViewAnimationOptionAllowUserInteraction
                       animations:^{
                         component.view.alpha = 1;
                       }
                       completion:nil];
    }
    [self.delegate insertListComponent:component wrapper:wrapper];
  }
  // Keep list-container compatibility: notify sticky even for an already attached wrapper.
  [self.stickyManager didAttachComponent:component];
}

- (void)removeListComponent:(LynxUIComponent *)component {
  // Keep list-container compatibility: reset sticky before detaching; notify the delegate only
  // after both views are detached, using the same parent ownership check as the legacy list.
  [self.stickyManager willDetachComponent:component];
  if (component.view.superview.superview == self.view) {
    [component.view.superview removeFromSuperview];
    [component.view removeFromSuperview];
    [self.delegate removeListComponent:component];
  }
}

- (NSMutableArray<void (^)(void)> *)restoreNativeStateBlockArray {
  if (!_restoreNativeStateBlockArray) {
    _restoreNativeStateBlockArray = [NSMutableArray array];
  }
  return _restoreNativeStateBlockArray;
}

- (void)onNodeReady {
  [super onNodeReady];
  // Keep list-container compatibility: expose the original mutable queue, and detach it before
  // invoking callbacks so reentrant enqueues are retained for the next node-ready pass.
  NSArray<void (^)(void)> *tasks = _restoreNativeStateBlockArray;
  _restoreNativeStateBlockArray = nil;
  for (void (^restore)(void) in tasks) {
    restore();
  }
  if (self.needAdjustContentOffset) {
    self.needAdjustContentOffset = NO;
    self.previousContentOffset = [self.scrollHelper applyContentSize:self.targetContentSize
                                                         offsetDelta:self.targetDelta
                                               previousContentOffset:self.previousContentOffset
                                              disableScrollFiltering:NO];
    self.targetDelta = CGPointZero;
  }
  [self.stickyManager updateStickyItems];
}

- (BOOL)initialPropsFlushed:(NSString *)initialPropKey cacheKey:(NSString *)cacheKey {
  return [self.initialFlushPropCache[cacheKey] containsObject:initialPropKey];
}

- (void)setInitialPropsHasFlushed:(NSString *)initialPropKey cacheKey:(NSString *)cacheKey {
  NSMutableSet *props = self.initialFlushPropCache[cacheKey];
  if (!props) {
    props = [NSMutableSet set];
    self.initialFlushPropCache[cacheKey] = props;
  }
  [props addObject:initialPropKey];
}

- (void)propsDidUpdate {
  [super propsDidUpdate];
  [self.stickyManager propsDidUpdate];
}

- (void)eventDidSet {
  [super eventDidSet];
  if (!self.scrollEventManager) {
    self.scrollEventManager = [[LynxScrollEventManager alloc] initWithContext:self.context
                                                                         sign:self.sign
                                                                     eventSet:self.eventSet];
  }
}

LYNX_PROP_SETTER("list-container-info", setStickyInfo, NSDictionary *) {
  self.itemKeys = value[@"itemkeys"];
  [self.stickyManager setStickyStartIndexes:value[@"stickyStart"] endIndexes:value[@"stickyEnd"]];
}

LYNX_PROP_SETTER("sticky", setEnableSticky, BOOL) { self.stickyManager.enabled = value; }
LYNX_PROP_SETTER("sticky-offset", setStickyOffset, CGFloat) { self.stickyManager.offset = value; }
LYNX_PROP_SETTER("experimental-batch-render-strategy", setBatchRenderStrategy, NSInteger) {
  self.enableBatchRender = value > 0;
}
LYNX_PROP_SETTER("enable-insert-platform-view-operation", setEnableInsertPlatformViewOperation,
                 BOOL) {
  self.enableInsertPlatformViewOperation = value;
}
LYNX_PROP_SETTER("enable-fade-in-animation", setEnableFadeInAnimation, BOOL) {
  self.enableFadeInAnimation = value;
}
LYNX_PROP_SETTER("update-animation-fade-in-duration", setUpdateAnimationFadeInDuration, NSInteger) {
  self.updateAnimationFadeInDuration = value / 1000.;
}
LYNX_PROP_SETTER("vertical-orientation", setVerticalOrientation, BOOL) {
  self.view.verticalOrientation = value;
}
LYNX_PROP_SETTER("scroll-orientation", setScrollOrientation, NSString *) {
  self.view.verticalOrientation =
      ![value isKindOfClass:NSString.class] || ![value isEqualToString:@"horizontal"];
}

- (NSInteger)getIndexFromItemKey:(NSString *)itemKey {
  return [self.itemHelper indexForItemKey:itemKey];
}
- (NSArray<UIView<LynxListItemWrapper> *> *)visibleCells {
  return self.itemHelper.visibleItemWrappers;
}
- (NSArray<NSDictionary *> *)visibleCellsInfo {
  return self.itemHelper.visibleItemInfo;
}
- (id<LynxEventTarget>)hitTest:(CGPoint)point withEvent:(UIEvent *)event {
  return [self.stickyManager findHitTargetAtPoint:point withEvent:event] ?: [self.itemHelper
      findHitTargetAtPoint:point
                withEvent:event] ?: self;
}

- (NSArray<UIScrollView *> *)getHitTestChainForNestedScrollViews {
  return self.context.lynxContext.getLynxView.nestedScrollViewsChain ?: @[];
}
- (void)scrollStateChangedFrom:(LynxBaseScrollViewScrollState)from
                            to:(LynxBaseScrollViewScrollState)to {
  self.currentScrollState = to;
  if (from == LynxBaseScrollViewScrollStateAnimating && to == LynxBaseScrollViewScrollStateIdle) {
    [self.scrollHelper finishProgrammaticScrollRequest];
  }
}
- (void)scrollViewDidScroll:(UIScrollView *)scrollView {
  [self.stickyManager updateStickyItems];
}

- (UIScrollView *)scrollViewForListItemHelper {
  return self.view;
}
- (BOOL)isVerticalForListItemHelper {
  return self.view.verticalOrientation;
}
- (NSArray<NSString *> *)itemKeysForListItemHelper {
  return self.itemKeys;
}
- (UIScrollView *)scrollViewForListStickyManager {
  return self.view;
}
- (BOOL)isVerticalForListStickyManager {
  return self.view.verticalOrientation;
}
- (NSArray<NSString *> *)itemKeysForListStickyManager {
  return self.itemKeys;
}
- (LynxScrollEventManager *)eventManagerForListStickyManager {
  return self.scrollEventManager;
}

// These renderer-facing primitives also make the native-state capability available to recycled
// children. Command parsing, engine notifications and scroll events are integrated separately.
- (void)updateScrollInfoWithEstimatedOffset:(CGFloat)estimatedOffset
                                     smooth:(BOOL)smooth
                                  scrolling:(BOOL)scrolling {
  [self.scrollHelper updateScrollInfoWithEstimatedOffset:estimatedOffset
                                                  smooth:smooth
                                               scrolling:scrolling];
}
- (UIScrollView<LynxListScrollHelperView> *)scrollViewForListScrollHelper {
  return self.view;
}
- (BOOL)isVerticalForListScrollHelper {
  return self.view.verticalOrientation;
}
- (BOOL)isRTLForListScrollHelper {
  return self.isRtl;
}
- (CGRect)listFrameForListScrollHelper {
  return self.frame;
}
- (UIEdgeInsets)listPaddingForListScrollHelper {
  return self.padding;
}
- (BOOL)listScrollHelperShouldDeferCompletionForSmoothScroll:(BOOL)smooth
                                              needsAnimation:(BOOL)needsAnimation {
  return needsAnimation && self.currentScrollState == LynxBaseScrollViewScrollStateAnimating;
}
- (id)listScrollHelperCompletionToken {
  return self.currentScrollState == LynxBaseScrollViewScrollStateAnimating ? self.view : nil;
}
- (BOOL)listScrollHelperShouldFinishTimeoutForPendingScroll {
  return YES;
}
- (void)listScrollHelperWillStartScrollAnimation {
  [self.view tryToUpdateScrollState:LynxBaseScrollViewScrollStateAnimating];
}
- (void)listScrollHelperDidStopProgrammaticScroll {
  // BaseScrollView notifies before committing its state; do not reenter an idle transition.
  if (self.currentScrollState != LynxBaseScrollViewScrollStateIdle) {
    [self.view tryToUpdateScrollState:LynxBaseScrollViewScrollStateIdle];
  }
}
- (void)listScrollHelperDidFinishProgrammaticScrollWithReason:
    (LynxListProgrammaticScrollCompletionReason)reason {
  [self.scrollHelper invalidateProgrammaticScrollRequest];
  [self listScrollHelperDidStopProgrammaticScroll];
}

@end
