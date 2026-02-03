// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxPropsProcessor.h>
#import <Lynx/LynxUICollection.h>
#import <Lynx/LynxUIMethodProcessor.h>
#import <Lynx/LynxWeakProxy.h>
#import <MJRefresh/MJRefresh.h>
#import <MJRefresh/MJRefreshHeader.h>
#import <XElement/LynxUIRefresh.h>
#import <XElement/LynxUIRefreshHeader.h>

typedef NS_ENUM(NSUInteger, LynxUIRefreshState) {
  LynxUIRefreshStateIdle = 0,
  LynxUIRefreshStateOverDragRelease,
  LynxUIRefreshStateRefreshing,
};

@interface LynxUIRefreshHeaderView : MJRefreshHeader

@property(nonatomic, weak) id<LynxUIRefreshDelegate> delegate;
@property(nonatomic, strong) UIView *headerView;
@property(assign, nonatomic) SEL releasedAction;
@property(assign, nonatomic) SEL idleAction;
@property(assign, nonatomic) CGFloat preReboundPercent;

@end

@interface LynxUIRefreshHeaderView ()

@property(nonatomic) CADisplayLink *displayLink;

@end

@implementation LynxUIRefreshHeaderView

- (void)prepare {
  [super prepare];
  if (!self.headerView) {
    self.headerView = [UIView new];
  }
  [self addSubview:self.headerView];
}

- (void)setPullingPercent:(CGFloat)pullingPercent {
  CGFloat oldValue = self.pullingPercent;
  [super setPullingPercent:pullingPercent];

  // Only triggering headeroffset with pullingPercent = 0 when header refresh ends.
  if (self.pullingPercent == 0 && (self.scrollView.isDragging || oldValue == 1)) {
    return;
  }

  if ([self.delegate respondsToSelector:@selector(refreshview:didUpdatePullingPrecent:)]) {
    [self.delegate refreshview:self didUpdatePullingPrecent:pullingPercent];
  }

  if (self.pullingPercent != 1) {
    if ([self.delegate respondsToSelector:@selector(refreshview:didUpdateShowingPrecent:)]) {
      [self.delegate refreshview:self didUpdateShowingPrecent:pullingPercent];
    }
  }

  if (self.pullingPercent == 0) {
    [_displayLink setPaused:YES];
  }
}

- (void)withReleasedAction:(SEL)action {
  self.releasedAction = action;
}

- (void)withIdleAction:(SEL)action {
  self.idleAction = action;
}

- (void)headerRebound {
  if (self.scrollView.layer.presentationLayer.bounds.origin.y > 0) {
    return;
  }

  CGFloat offsetY = self.scrollView.layer.presentationLayer.bounds.origin.y;
  CGFloat happenOffsetY = -self.scrollViewOriginalInset.top;
  CGFloat reboundPercent = (happenOffsetY - offsetY) / (self.mj_h == 0 ? 1 : self.mj_h);
  if (fabs(reboundPercent - _preReboundPercent) < CGFLOAT_EPSILON) {
    return;
  }

  if ([self.delegate respondsToSelector:@selector(refreshview:didUpdateShowingPrecent:)]) {
    [self.delegate refreshview:self didUpdateShowingPrecent:reboundPercent];
  }
  _preReboundPercent = reboundPercent;
}

- (void)setState:(MJRefreshState)state {
  if (state == MJRefreshStateIdle) {
    if ([self.refreshingTarget respondsToSelector:self.idleAction]) {
      MJRefreshMsgSend(MJRefreshMsgTarget(self.refreshingTarget), self.idleAction, self);
    }
  }
  [super setState:state];
}

- (void)scrollViewContentOffsetDidChange:(NSDictionary *)change {
  // When the finger is lifted with the header appears totally, trigger the headerreleased event.
  if (!self.scrollView.isDragging && self.state == MJRefreshStatePulling) {
    if ([self.refreshingTarget respondsToSelector:self.releasedAction]) {
      MJRefreshMsgSend(MJRefreshMsgTarget(self.refreshingTarget), self.releasedAction, self);
    }
  }

  if (!self.scrollView.isDragging && self.scrollView.contentOffset.y <= 0) {
    if (_displayLink == nil) {
      _displayLink = [CADisplayLink displayLinkWithTarget:[LynxWeakProxy proxyWithTarget:self]
                                                 selector:@selector(headerRebound)];
      if (@available(iOS 10.0, *)) {
        _displayLink.preferredFramesPerSecond = 20;
      } else {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
        _displayLink.frameInterval = 60 / 20;
#pragma clang diagnostic pop
      }
      [_displayLink addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSDefaultRunLoopMode];
    } else {
      [_displayLink setPaused:NO];
    }
  }

  CGFloat originH = self.mj_h;
  if (self.mj_h == 0) {
    self.mj_h = 1;
  }

  [super scrollViewContentOffsetDidChange:change];

  self.mj_h = originH;
}
@end

@interface LynxUIRefresh ()

@property(nonatomic, assign) CGRect selfFrame;
@property(nonatomic, strong) UIScrollView *scrollView;
@property(nonatomic, weak) LynxUIRefreshHeader *lynxHeader;
@property(nonatomic, weak) LynxUI *lynxList;
@property(nonatomic, assign) BOOL enableRefresh;
@property(nonatomic, assign) BOOL manualRefresh;

@end

@implementation LynxUIRefresh

- (instancetype)init {
  if (self = [super init]) {
    _enableRefresh = YES;
  }
  return self;
}

- (UIView *)createView {
  UIView *view = [[UIView alloc] init];
  return view;
}

- (void)insertChild:(LynxUI *)child atIndex:(NSInteger)index {
  [super insertChild:child atIndex:index];
  if ([child isKindOfClass:[LynxUIRefreshHeader class]]) {
    self.lynxHeader = (LynxUIRefreshHeader *)child;
  } else {
    self.lynxList = child;
  }
}

- (void)finishLayoutOperation {
  UIScrollView *preScrollView = self.scrollView;
  NSMutableArray *excludeViews = [NSMutableArray array];
  if (self.lynxHeader.view) {
    [excludeViews addObject:self.lynxHeader.view];
  }

  self.scrollView = (UIScrollView *)[self findViewWithKind:[UIScrollView class]
                                                  fromView:[self view]
                                              excludeViews:excludeViews];
  self.scrollView.bounces = YES;
  if (!self.scrollView) {
    return;
  }

  if (!CGRectEqualToRect(self.selfFrame, self.view.frame) || self.scrollView != preScrollView) {
    self.selfFrame = self.view.frame;
    self.scrollView.frame = self.view.bounds;
    [self loadHeaderAndFooter];
  }
}

- (id<LynxEventTarget>)hitTest:(CGPoint)point withEvent:(UIEvent *)event {
  self.scrollView.bounces = YES;
  CGPoint hp = [[self view] convertPoint:point toView:self.lynxHeader.view];
  CGPoint pt = [[self view] convertPoint:point toView:self.lynxList.view];
  if ([self.lynxHeader.view pointInside:hp withEvent:event]) {
    return [self.lynxHeader hitTest:hp withEvent:event];
  }

  if ([self.lynxList.view pointInside:pt withEvent:event]) {
    // (pt) is resulted by adding contentOffset
    return [self.lynxList hitTest:pt withEvent:event];
  }
  return [super hitTest:point withEvent:event];
}

#pragma mark - Private

- (void)loadHeaderAndFooter {
  [self loadHeader];
  [self loadFooter];
}

- (void)loadHeader {
  if (self.enableRefresh && self.lynxHeader.view) {
    LynxUIRefreshHeaderView *header =
        [LynxUIRefreshHeaderView headerWithRefreshingTarget:self
                                           refreshingAction:@selector(startRefresh)];
    [header withReleasedAction:@selector(headerReleased)];
    [header withIdleAction:@selector(headerIdle)];
    header.delegate = self;
    header.mj_h = self.lynxHeader.view.bounds.size.height;
    header.headerView.frame = self.lynxHeader.view.bounds;
    [header.headerView addSubview:self.lynxHeader.view];
    self.scrollView.mj_header = header;
  } else {
    [self.lynxHeader.view removeFromSuperview];
  }
}

- (void)loadFooter {
  // Deprecated
}

- (void)startRefresh {
  if (self.enableRefresh && self.lynxHeader.view) {
    LynxCustomEvent *event =
        [[LynxDetailEvent alloc] initWithName:@"startrefresh"
                                   targetSign:[self sign]
                                       detail:@{@"isManual" : @(self.manualRefresh)}];
    [self.context.eventEmitter sendCustomEvent:event];

    [self.context.eventEmitter
        sendCustomEvent:[[LynxDetailEvent alloc]
                            initWithName:@"refreshstatechange"
                              targetSign:[self sign]
                                  detail:@{@"state" : @(LynxUIRefreshStateRefreshing)}]];
  }
  self.manualRefresh = YES;
}

- (void)startLoadMore {
  // Deprecated
}

- (void)headerReleased {
  if (self.enableRefresh && self.lynxHeader.view) {
    // When the finger is lifted, if the drop-down distance of the header is greater than or equal
    // to the height of the header, the headerreleased event will be triggered. It is before the
    // startrefresh event.
    LynxCustomEvent *event = [[LynxDetailEvent alloc] initWithName:@"headerreleased"
                                                        targetSign:[self sign]
                                                            detail:@{}];
    [self.context.eventEmitter sendCustomEvent:event];

    [self.context.eventEmitter
        sendCustomEvent:[[LynxDetailEvent alloc]
                            initWithName:@"refreshstatechange"
                              targetSign:[self sign]
                                  detail:@{@"state" : @(LynxUIRefreshStateOverDragRelease)}]];
  }
}

- (void)headerIdle {
  if (self.enableRefresh && self.lynxHeader.view) {
    [self.context.eventEmitter
        sendCustomEvent:[[LynxDetailEvent alloc]
                            initWithName:@"refreshstatechange"
                              targetSign:[self sign]
                                  detail:@{@"state" : @(LynxUIRefreshStateIdle)}]];
  }
}

- (void)footerReleased {
  // Deprecated
}

- (nullable UIView *)findViewWithKind:(Class)clz
                             fromView:(UIView *)view
                         excludeViews:(nullable NSArray<UIView *> *)excludeViews {
  __block UIView *resultView;
  NSArray *subviews = view.subviews;

  if (view.subviews.count <= 0) {
    return nil;
  }

  [subviews enumerateObjectsUsingBlock:^(UIView *subView, NSUInteger idx, BOOL *_Nonnull stop) {
    if ([subView isKindOfClass:clz]) {
      resultView = subView;
      *stop = YES;
    }
  }];

  if (!resultView) {
    [subviews enumerateObjectsUsingBlock:^(UIView *subView, NSUInteger idx, BOOL *_Nonnull stop) {
      if (![excludeViews containsObject:subView]) {
        resultView = [self findViewWithKind:clz fromView:subView excludeViews:excludeViews];
        if (resultView) {
          *stop = YES;
        }
      }
    }];
  }

  return resultView;
}

#pragma mark - LynxUIRefreshDelegate

- (void)refreshview:(MJRefreshComponent *)refreshview
    didUpdatePullingPrecent:(CGFloat)pullingPercent {
  // Ensure that the header is not obscured by elements inside the scrollview.
  if (self.scrollView.mj_header.layer.zPosition != CGFLOAT_MAX) {
    self.scrollView.mj_header.layer.zPosition = CGFLOAT_MAX;
  }
}

- (void)refreshview:(MJRefreshComponent *)refreshview
    didUpdateShowingPrecent:(CGFloat)showingPercent {
  // legacy API
  if (refreshview == self.scrollView.mj_header) {
    LynxCustomEvent *event =
        [[LynxDetailEvent alloc] initWithName:@"headershow"
                                   targetSign:[self sign]
                                       detail:@{
                                         @"isDragging" : @(refreshview.scrollView.isDragging),
                                         @"offsetPercent" : @(showingPercent)
                                       }];
    [self.context.eventEmitter sendCustomEvent:event];
  }
  if (refreshview == self.scrollView.mj_header) {
    LynxCustomEvent *event =
        [[LynxDetailEvent alloc] initWithName:@"headeroffset"
                                   targetSign:[self sign]
                                       detail:@{
                                         @"isDragging" : @(refreshview.scrollView.isDragging),
                                         @"offsetPercent" : @(showingPercent)
                                       }];
    [self.context.eventEmitter sendCustomEvent:event];
  }
}

LYNX_PROP_SETTER("enable-refresh", enableRefresh, BOOL) {
  if (_enableRefresh != value) {
    _enableRefresh = value;
    [self loadHeader];
  }
}

LYNX_UI_METHOD(finishRefresh) {
  [self.scrollView.mj_header endRefreshing];
  if (callback) {
    callback(kUIMethodSuccess, nil);
  }
}

LYNX_UI_METHOD(autoStartRefresh) {
  self.manualRefresh = false;
  [self.scrollView.mj_header beginRefreshing];
  if (callback) {
    callback(kUIMethodSuccess, nil);
  }
}

@end
