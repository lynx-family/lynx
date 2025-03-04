// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxScrollEventManager.h"
#import "LynxUIContext.h"

NSString *const LynxEventScroll = @"scroll";
NSString *const LynxEventScrollEnd = @"scrollend";
NSString *const LynxEventScrollToUpper = @"scrolltoupper";
NSString *const LynxEventScrollToUpperEdge = @"scrolltoupperedge";
NSString *const LynxEventScrollToLower = @"scrolltolower";
NSString *const LynxEventScrollToLowerEdge = @"scrolltoloweredge";
NSString *const LynxEventScrollToNormalState = @"scrolltonormalstate";
NSString *const LynxEventContentSizeChange = @"contentsizechanged";
NSString *const LynxEventScrollStateChange = @"scrollstatechange";
NSString *const LynxEventScrollToBounce = @"scrolltobounce";
NSString *const LynxScrollViewInitialScrollOffset = @"initialScrollOffset";
NSString *const LynxScrollViewInitialScrollIndex = @"initialScrollIndex";
NSString *const LynxEventStickyTop = @"stickytop";
NSString *const LynxEventStickyBottom = @"stickybottom";
NSString *const LynxEventSnap = @"snap";

@interface LynxScrollEventManager ()
@property(nonatomic, weak) LynxUIContext *context;
@property(nonatomic, assign) NSInteger sign;
@property(nonatomic, strong, nullable) NSDictionary *eventSet;
@end

@implementation LynxScrollEventManager
- (instancetype)initWithContext:(LynxUIContext *)context
                           sign:(NSInteger)sign
                       eventSet:(NSDictionary *_Nullable)eventSet {
  if (self = [super init]) {
    self.context = context;
    self.sign = sign;
    self.eventSet = eventSet;
  }
  return self;
}

- (BOOL)eventBound:(NSString *)name {
  return [self.eventSet objectForKey:name];
}

- (void)sendScrollEvent:(NSString *)name scrollView:(UIScrollView *)scrollView {
  if (![self eventBound:name]) {
    return;
  }
  [self sendScrollEvent:name scrollView:scrollView deltaX:0 deltaY:0];
}

- (void)sendScrollEvent:(NSString *)name
             scrollView:(UIScrollView *)scrollView
                 detail:(NSDictionary *)detail {
  if (![self eventBound:name]) {
    return;
  }
  [self sendScrollEvent:name
              scrollTop:scrollView.contentOffset.y
             scrollLeft:scrollView.contentOffset.x
           scrollHeight:scrollView.contentSize.height
            scrollWidth:scrollView.contentSize.width
                 deltaX:0
                 deltaY:0
             isDragging:scrollView.isDragging
                 detail:detail];
}

- (void)sendScrollEvent:(NSString *)name
             scrollView:(UIScrollView *)scrollView
    targetContentOffset:(CGPoint)targetContentOffset {
  if (![self eventBound:name]) {
    return;
  }
  [self sendScrollEvent:name
              scrollTop:targetContentOffset.y
             scrollLeft:targetContentOffset.x
           scrollHeight:scrollView.contentSize.height
            scrollWidth:scrollView.contentSize.width
                 deltaX:0
                 deltaY:0
             isDragging:scrollView.isDragging
                 detail:@{}];
}

- (void)sendScrollEvent:(NSString *)name
             scrollView:(UIScrollView *)scrollView
                 deltaX:(CGFloat)x
                 deltaY:(CGFloat)y {
  if (![self eventBound:name]) {
    return;
  }
  [self sendScrollEvent:name
              scrollTop:scrollView.contentOffset.y
             scrollLeft:scrollView.contentOffset.x
           scrollHeight:scrollView.contentSize.height
            scrollWidth:scrollView.contentSize.width
                 deltaX:x
                 deltaY:y
             isDragging:scrollView.isDragging
                 detail:@{}];
}

- (void)sendScrollEvent:(NSString *)name
              scrollTop:(CGFloat)top
             scrollLeft:(CGFloat)left
           scrollHeight:(CGFloat)height
            scrollWidth:(CGFloat)width
                 deltaX:(CGFloat)x
                 deltaY:(CGFloat)y
             isDragging:(BOOL)isDragging
                 detail:(NSDictionary *)detail {
  NSMutableDictionary *scrollEventBaseInfo = [NSMutableDictionary dictionaryWithDictionary:@{
    @"deltaX" : @(x),
    @"deltaY" : @(y),
    @"scrollLeft" : @(left),
    @"scrollTop" : @(top),
    @"scrollHeight" : @(height),
    @"scrollWidth" : @(width),
    @"isDragging" : @(isDragging)
  }];
  [scrollEventBaseInfo addEntriesFromDictionary:detail];
  LynxCustomEvent *scrollEventInfo = [[LynxDetailEvent alloc] initWithName:name
                                                                targetSign:self.sign
                                                                    detail:scrollEventBaseInfo];
  [self.context.eventEmitter dispatchCustomEvent:scrollEventInfo];
}

@end  // LynxScrollEventUtils
