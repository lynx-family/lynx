// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <Lynx/LynxBaseScrollView+Internal.h>
#import <Lynx/LynxBaseScrollView+Public.h>
#import <Lynx/LynxPropsProcessor.h>
#import <Lynx/LynxUI+Internal.h>
#import <Lynx/LynxUIScrollViewInternal.h>
#import <Lynx/LynxUnitUtils.h>
#import <Lynx/LynxView+Internal.h>
#import <Lynx/UIScrollView+Lynx.h>

@interface LynxUIScrollViewEventHelper : NSObject <LynxBaseScrollViewDelegate>

@property(nonatomic, assign) CGFloat upperThreshold;
@property(nonatomic, assign) CGFloat lowerThreshold;
@property(nonatomic, assign) CGFloat throttle;
@property(nonatomic, weak) LynxUIScrollViewInternal *ui;
@property(nonatomic, assign) CFTimeInterval lastUpdateTime;
@property(nonatomic, assign) CGPoint lastContentOffset;
@property(nonatomic, assign) BOOL atUpper;
@property(nonatomic, assign) BOOL atLower;

@end

@implementation LynxUIScrollViewEventHelper

- (void)scrollStateChangedFrom:(LynxBaseScrollViewScrollState)from
                            to:(LynxBaseScrollViewScrollState)to {
  switch (to) {
    case LynxBaseScrollViewScrollStateIdle:
      [self sendScrollEvent:@"scrollend" params:nil];
      break;
    case LynxBaseScrollViewScrollStateDragging:
      [self sendScrollEvent:@"scrollstart" params:nil];
      break;
    case LynxBaseScrollViewScrollStateAnimating:
      if (from == LynxBaseScrollViewScrollStateIdle) {
        [self sendScrollEvent:@"scrollstart" params:nil];
      }
      break;
    case LynxBaseScrollViewScrollStateFling:
      break;
    default:
      break;
  }

  [self sendScrollEvent:@"scrollstatechange"
                 params:@{@"previousState" : @(from), @"currentState" : @(to)}];
}

- (NSArray<UIScrollView *> *)getHitTestChainForNestedScrollViews {
  return self.ui.context.lynxContext.getLynxView.nestedScrollViewsChain;
}

- (void)scrollViewDidScroll:(LynxBaseScrollView *)scrollView {
  [self tryToSendScrollEvent];
  [self updateScrollPosition];
}

- (void)tryToSendScrollEvent {
  if (_throttle) {
    CFTimeInterval currentTime = CFAbsoluteTimeGetCurrent();
    if (currentTime - _lastUpdateTime >= _throttle) {
      [self sendScrollEvent:@"scroll"
                     params:@{
                       @"deltaX" : @(self.ui.view.contentOffset.x - _lastContentOffset.x),
                       @"deltaY" : @(self.ui.view.contentOffset.y - _lastContentOffset.y),
                     }];
      _lastContentOffset = self.ui.view.contentOffset;
      _lastUpdateTime = currentTime;
    }
  } else {
    [self sendScrollEvent:@"scroll"
                   params:@{
                     @"deltaX" : @(self.ui.view.contentOffset.x - _lastContentOffset.x),
                     @"deltaY" : @(self.ui.view.contentOffset.y - _lastContentOffset.y),
                   }];
    _lastContentOffset = self.ui.view.contentOffset;
  }
}

- (void)updateScrollPosition {
  LynxBaseScrollView *scrollView = self.ui.view;

  CGFloat scrollOffset = scrollView.vertical ? [scrollView getScrollOffsetVertically]
                                             : [scrollView getScrollOffsetHorizontally];
  CGFloat scrollRange[2] = {0, 0};
  scrollView.vertical ? [scrollView getScrollRangeVertically:&scrollRange]
                      : [scrollView getScrollRangeHorizontally:&scrollRange];

  BOOL atUpper = scrollOffset <= scrollRange[0] + _upperThreshold;
  BOOL atLower = scrollOffset >= scrollRange[1] - _lowerThreshold;

  if (atUpper && !_atUpper) {
    [self sendScrollEvent:@"scrolltoupper" params:nil];
  }
  if (atLower && !_atLower) {
    [self sendScrollEvent:@"scrolltolower" params:nil];
  }

  _atUpper = atUpper;
  _atLower = atLower;
  [self updateSticky:[scrollView getScrollOffset]];
}

- (void)updateSticky:(CGPoint)contentOffset {
  [self.ui.children
      enumerateObjectsUsingBlock:^(LynxUI *_Nonnull ui, NSUInteger idx, BOOL *_Nonnull stop) {
        [ui checkStickyOnParentScroll:contentOffset.x withOffsetY:contentOffset.y];
      }];
}

- (void)sendScrollEvent:(NSString *)name params:(NSDictionary *)params {
  LynxBaseScrollView *scrollView = self.ui.view;
  NSMutableDictionary *scrollEventDetail = [NSMutableDictionary dictionaryWithDictionary:@{
    @"scrollLeft" : @(scrollView.contentOffset.x),  // scrollOffset
    @"scrollTop" : @(scrollView.contentOffset.y),
    @"scrollHeight" : @(scrollView.contentSize.width),  // contentSize
    @"scrollWidth" : @(scrollView.contentSize.height),
    @"isDragging" : @(scrollView.isDragging),
    @"scrollState" : @([scrollView currentScrollState]),
  }];

  [scrollEventDetail addEntriesFromDictionary:params];

  LynxCustomEvent *scrollEvent = [[LynxDetailEvent alloc] initWithName:name
                                                            targetSign:self.ui.sign
                                                                detail:scrollEventDetail];
  [self.ui.context.eventEmitter dispatchCustomEvent:scrollEvent];
}

@end

@interface LynxUIScrollViewInternal ()
@property(nonatomic, strong) LynxUIScrollViewEventHelper *eventHelper;
@property(nonatomic, strong) NSMutableArray<LynxNodeReadyBlock> *firstRenderBlockArray;
@property(nonatomic, assign) BOOL enableSticky;
@end

@implementation LynxUIScrollViewInternal

+ (CGPoint)getPositionOf:(LynxUI *)target
                    from:(LynxUIScrollViewInternal *)scroll
                   isRTL:(BOOL)isRTL {
  CGPoint result = CGPointZero;
  if (scroll && target) {
    if (isRTL) {
      CGFloat scrollRange[4] = {0, 0, 0, 0};
      [scroll.view getScrollRange:&scrollRange];
      result.x = scrollRange[1] - target.frame.origin.x - target.frame.size.width;
      result.y = target.frame.origin.y;
    } else {
      result.x = target.frame.origin.x;
      result.y = target.frame.origin.y;
    }
  }
  return result;
}

+ (CGPoint)addOffset:(CGPoint)offset by:(CGPoint)delta isRTL:(BOOL)isRTL {
  offset.x += isRTL ? -delta.x : delta.x;
  offset.y += delta.y;
  return offset;
}

+ (CGPoint)formatOffset:(CGPoint)offset from:(LynxUIScrollViewInternal *)scroll isRTL:(BOOL)isRTL {
  if (isRTL) {
    CGFloat scrollRange[4] = {0, 0, 0, 0};
    [scroll.view getScrollRange:&scrollRange];
    offset.x = scrollRange[1] - offset.x;
  }
  return offset;
}

LYNX_PROP_DEFINE("scroll-orientation", setScrollOrientation, NSString *) {
  BOOL vertical = YES;
  if ([@"horizontal" isEqualToString:value]) {
    vertical = NO;
  }
  self.view.vertical = vertical;
}

LYNX_PROP_DEFINE("enable-scroll-bar", setEnableScrollBar, BOOL) {
  self.view.showsHorizontalScrollIndicator = value;
  self.view.showsVerticalScrollIndicator = value;
}

LYNX_PROP_DEFINE("enable-scroll", setEnableScroll, BOOL) { self.view.scrollEnabled = value; }

LYNX_PROP_DEFINE("bounces", setBounces, BOOL) { self.view.bounces = value; }

LYNX_PROP_DEFINE("forwards-nested-scroll", setForwardsNestedScroll, NSInteger) {
  self.view.forwardsNestedScrollMode = value;
}

LYNX_PROP_DEFINE("backwards-nested-scroll", setBackwardsNestedScroll, NSInteger) {
  self.view.backwardsNestedScrollMode = value;
}

LYNX_PROP_DEFINE("initial-scroll-index", setInitialScrollIndex, NSInteger) {
  [_firstRenderBlockArray addObject:^(LynxUI *ui) {
    LynxUIScrollViewInternal *scroll = (LynxUIScrollViewInternal *)ui;
    if (value >= 0 && (NSUInteger)value < [scroll.children count]) {
      LynxUI *target = [scroll.children objectAtIndex:value];
      [scroll.view scrollTo:[LynxUIScrollViewInternal getPositionOf:target
                                                               from:scroll
                                                              isRTL:scroll.isRtl]];
    }
  }];
}

LYNX_PROP_DEFINE("initial-scroll-offset", setInitialScrollOffset, NSString *) {
  CGFloat offset = [self toPtWithUnitValue:value fontSize:0];
  [_firstRenderBlockArray addObject:^(LynxUI *ui) {
    LynxUIScrollViewInternal *scroll = (LynxUIScrollViewInternal *)ui;
    [scroll.view scrollTo:[LynxUIScrollViewInternal formatOffset:CGPointMake(offset, offset)
                                                            from:scroll
                                                           isRTL:scroll.isRtl]];
  }];
}

LYNX_PROP_DEFINE("upper-threshold", setUpperThreshold, NSString *) {
  _eventHelper.upperThreshold = [self toPtWithUnitValue:value fontSize:0];
}

LYNX_PROP_DEFINE("lower-threshold", setLowerThreshold, NSString *) {
  _eventHelper.lowerThreshold = [self toPtWithUnitValue:value fontSize:0];
}

LYNX_PROP_DEFINE("scroll-event-throttle", setScrollEventThrottle, NSNumber *) {
  _eventHelper.throttle = [value doubleValue] / 1000.0;
}

- (void)scrollTo:(NSDictionary *)params withResult:(LynxUIMethodCallbackBlock)callback {
  CGFloat offset = [LynxUnitUtils toPtFromIDUnitValue:[params objectForKey:@"offset"]
                                        withDefaultPt:0.];
  NSInteger index = [[params objectForKey:@"index"] ?: @(0) integerValue];
  BOOL animated =
      [([params objectForKey:@"smooth"] ?: [params objectForKey:@"animated"]) ?: @(YES) boolValue];

  if (index >= 0 && (NSUInteger)index < [self.children count]) {
    LynxUI *target = [self.children objectAtIndex:index];
    CGPoint targetOffset = [LynxUIScrollViewInternal getPositionOf:target
                                                              from:self
                                                             isRTL:self.isRtl];

    targetOffset = [LynxUIScrollViewInternal addOffset:targetOffset
                                                    by:CGPointMake(offset, offset)
                                                 isRTL:self.isRtl];

    if (animated) {
      [self.view animatedScrollTo:targetOffset
                         complete:^(BOOL completed){
                         }];
    } else {
      [self.view scrollTo:targetOffset];
    }
    callback(kUIMethodSuccess, nil);
  } else {
    callback(kUIMethodParamInvalid,
             [NSString stringWithFormat:@"scrollTo index: %ld is out of range[0, %ld).", index,
                                        self.children.count]);
  }
}

- (void)scrollBy:(NSDictionary *)params withResult:(LynxUIMethodCallbackBlock)callback {
  CGFloat offset = [LynxUnitUtils toPtFromIDUnitValue:[params objectForKey:@"offset"]
                                        withDefaultPt:0.];
  CGPoint targetOffset = [LynxUIScrollViewInternal addOffset:[self.view getScrollOffset]
                                                          by:CGPointMake(offset, offset)
                                                       isRTL:self.isRtl];

  [self.view scrollTo:targetOffset];

  callback(kUIMethodSuccess, nil);
}

- (void)autoScroll:(NSDictionary *)params withResult:(LynxUIMethodCallbackBlock)callback {
  if ([[params objectForKey:@"start"] ?: @(YES) boolValue]) {
    CGFloat rate = [self toPtWithUnitValue:[params objectForKey:@"rate"] fontSize:0];
    NSInteger preferredFramesPerSecond = 1000 / 16;

    // We can not move less than 1/scale pt in every frame, cause contentOffset
    // will align to 1/scale.
    while (ABS(rate / preferredFramesPerSecond) < 1.0 / UIScreen.mainScreen.scale) {
      preferredFramesPerSecond -= 1;
      if (preferredFramesPerSecond == 0) {
        callback(kUIMethodParamInvalid, @"rate is too small to scroll");
        return;
      }
    };

    NSTimeInterval interval = 1.0 / preferredFramesPerSecond;
    rate *= interval;

    [self.view autoScrollWithRate:rate
                         interval:interval
                         complete:^(BOOL completed){
                         }];
  } else {
    [self.view stopScrolling];
    [self.view tryToUpdateScrollState:LynxBaseScrollViewScrollStateIdle];
  }

  callback(kUIMethodSuccess, nil);
}

- (LynxBaseScrollView *)createView {
  self.firstRender = YES;
  _firstRenderBlockArray = [NSMutableArray array];
  _eventHelper = [[LynxUIScrollViewEventHelper alloc] init];
  _eventHelper.ui = self;
  LynxBaseScrollView *view = [[LynxBaseScrollView alloc] initWithVertical:YES layoutFromEnd:NO];
  view.scrollDelegate = _eventHelper;
  return view;
}

- (void)onNodeReady {
  [super onNodeReady];
}

- (void)layoutDidFinished {
  [super layoutDidFinished];
  __block CGSize contentSize = CGSizeZero;

  [self.children
      enumerateObjectsUsingBlock:^(LynxUI *_Nonnull obj, NSUInteger idx, BOOL *_Nonnull stop) {
        contentSize.width += obj.frame.size.width + obj.margin.left + obj.margin.right;
        contentSize.height += obj.frame.size.height + obj.margin.top + obj.margin.bottom;
      }];
  [self.view setScrollContentSize:contentSize];
  [self flushFirstRenderOperations];
  [_eventHelper updateScrollPosition];
}

- (BOOL)isScrollContainer {
  return YES;
}

- (BOOL)notifyParent {
  return YES;
}

- (void)flushFirstRenderOperations {
  if (self.firstRender) {
    [_firstRenderBlockArray enumerateObjectsUsingBlock:^(LynxNodeReadyBlock _Nonnull obj,
                                                         NSUInteger idx, BOOL *_Nonnull stop) {
      obj(self);
    }];
    [_firstRenderBlockArray removeAllObjects];
  }
  self.firstRender = NO;
}

- (void)scrollInto:(LynxUI *)value isSmooth:(BOOL)isSmooth type:(NSString *)type {
  CGPoint scrollTarget = CGPointZero;
  if ([@"center" isEqualToString:type]) {
    scrollTarget.y -= (self.view.frame.size.height - value.view.frame.size.height) / 2;
    scrollTarget.x -= (self.view.frame.size.width - value.view.frame.size.width) / 2;
  } else if ([@"end" isEqualToString:type]) {
    scrollTarget.y -= (self.view.frame.size.height - value.view.frame.size.height);
    scrollTarget.x -= (self.view.frame.size.width - value.view.frame.size.width);
  }
  while (value != self) {
    scrollTarget.y += value.view.frame.origin.y;
    scrollTarget.x += value.view.frame.origin.x;
    value = value.parent;
  }
  if (isSmooth) {
    [self.view animatedScrollTo:scrollTarget
                       complete:^(BOOL completed){
                       }];
  } else {
    [self.view scrollTo:scrollTarget];
  }
}

@end
