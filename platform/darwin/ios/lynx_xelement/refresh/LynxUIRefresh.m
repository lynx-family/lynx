// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <XElement/LynxUIRefresh.h>
#import <XElement/LynxUIRefreshHeader.h>
#import <Lynx/LynxUIMethodProcessor.h>
#import <Lynx/LynxPropsProcessor.h>
#import <MJRefresh/MJRefreshHeader.h>
#import <MJRefresh/MJRefreshAutoFooter.h>
#import <MJRefresh/MJRefresh.h>
#import <Lynx/LynxUICollection.h>
#import <Lynx/LynxWeakProxy.h>

@protocol BDXRefreshDelegate <NSObject>

- (void)refreshview:(MJRefreshComponent *)refreshview didUpdatePullingPrecent:(CGFloat)pullingPercent;
- (void)refreshview:(MJRefreshComponent *)refreshview didUpdateShowingPrecent:(CGFloat)showingPercent;

@end

@interface BDXRefreshHeader : MJRefreshHeader

@property (nonatomic, weak) id<BDXRefreshDelegate> delegate;
@property (nonatomic, strong) UIView *headerView;
@property (assign, nonatomic) SEL releasedAction;
@property (assign, nonatomic) CGFloat preReboundPercent;

@end

@interface BDXRefreshHeader ()

@property (nonatomic) CADisplayLink *displayLink;

@end

@implementation BDXRefreshHeader

- (void)prepare
{
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

- (void)withReleasedAction:(SEL)action
{
    self.releasedAction = action;
}

- (void)headerRebound {
    if (self.scrollView.layer.presentationLayer.bounds.origin.y > 0) {
        return;
    }
    
    CGFloat offsetY = self.scrollView.layer.presentationLayer.bounds.origin.y;
    CGFloat happenOffsetY = - self.scrollViewOriginalInset.top;
    CGFloat reboundPercent = (happenOffsetY - offsetY) / (self.mj_h == 0 ? 1 : self.mj_h);
    if (fabs(reboundPercent - _preReboundPercent) < CGFLOAT_EPSILON) {
        return;
    }
    
    if ([self.delegate respondsToSelector:@selector(refreshview:didUpdateShowingPrecent:)]) {
        [self.delegate refreshview:self didUpdateShowingPrecent:reboundPercent];
    }
    _preReboundPercent = reboundPercent;
}

- (void)scrollViewContentOffsetDidChange:(NSDictionary *)change
{
    // When the finger is lifted with the header appears totally, trigger the headerreleased event.
    if (!self.scrollView.isDragging && self.state == MJRefreshStatePulling) {
        if ([self.refreshingTarget respondsToSelector:self.releasedAction]) {
            MJRefreshMsgSend(MJRefreshMsgTarget(self.refreshingTarget), self.releasedAction, self);
        }
    }
    
    if (!self.scrollView.isDragging && self.scrollView.contentOffset.y <= 0) {
        if (_displayLink == nil) {
            _displayLink = [CADisplayLink displayLinkWithTarget:[LynxWeakProxy proxyWithTarget:self] selector:@selector(headerRebound)];
            if (@available(iOS 10.0, *)) {
              _displayLink.preferredFramesPerSecond = 20;
            } else {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
              _displayLink.frameInterval = 60 / 20;
#pragma clang diagnostic pop
            }
            [_displayLink addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSDefaultRunLoopMode];
        }
        else {
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

@interface BDXRefreshFooter : MJRefreshAutoFooter

@property (nonatomic, weak) id<BDXRefreshDelegate> delegate;
@property (nonatomic, strong) UIView *footerView;
@property (assign, nonatomic) SEL releasedAction;
@property (nonatomic, assign) BOOL hasMore; // Record the value of has_more of the ui method finishLoadMore.
@property (nonatomic, assign) NSInteger triggerTimes; // Record the number of times startloadmore was able to be triggered.
// Record whether to trigger the startloadmore event automatically when the footer appears without finger lifted.
@property (nonatomic, assign) BOOL autoLoadMore;
// Compatible with older versions. Record whether the footer can automatically rebound by calling finishLoadMore.
@property (nonatomic, assign) BOOL canRebound;
// Sets whether loadmore can be triggered when UIScrollView's scrolling is controlled by the client with scroll disabled.
@property (nonatomic, assign) BOOL canLoadMoreWhenScrollDisabled;

@end

@implementation BDXRefreshFooter

- (id)init
{
    if (self = [super init]) {
        self.hasMore = YES;
        self.autoLoadMore = YES;
        self.canRebound = NO;
        self.canLoadMoreWhenScrollDisabled = NO;
    }
    
    return self;
}

- (void)setPullingPercent:(CGFloat)pullingPercent
{
    [super setPullingPercent:pullingPercent];
    if ([self.delegate respondsToSelector:@selector(refreshview:didUpdatePullingPrecent:)]) {
        [self.delegate refreshview:self didUpdatePullingPrecent:pullingPercent];
    }
}

- (void)withReleasedAction:(SEL)action
{
    self.releasedAction = action;
}

- (void)placeSubviews
{
    [super placeSubviews];
    CGSize size = _footerView.frame.size;
    _footerView.frame = CGRectMake(0, 0, size.width, size.height);
}

- (void)setYPosition
{
    // When the content height exceeds the window height, we put the footer on the bottom edge of the content, otherwise,
    // we hide the footer on the bottom edge of the window.
    // In addition, in LynxUIOwner's layoutDidFinish, the layout triggering order of label elements is random, so
    // the calling order of willMoveToSuperview and scrollViewContentSizeDidChange is not fixed. Therefore, we need to
    // call setYPosition in both places to ensure the correct position of the footer.
    if (self.scrollView.mj_contentH > self.scrollView.mj_h) {
        self.mj_y = self.scrollView.mj_contentH + self.ignoredScrollViewContentInsetBottom;
    }
    else {
        self.mj_y = self.scrollView.mj_h + self.ignoredScrollViewContentInsetBottom;
    }
}

- (void)willMoveToSuperview:(UIView *)newSuperview
{
    struct objc_super superReceiver = { self, [MJRefreshComponent class] };
    void (*funcCall)(void *, SEL, UIView *) = (void *)objc_msgSendSuper;
    funcCall(&superReceiver, _cmd, newSuperview);

    if (newSuperview) {
        [self setYPosition];
    }
}
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wobjc-missing-super-calls"
- (void)scrollViewContentSizeDidChange:(NSDictionary *)change
{
    CGPoint old = [change[@"old"] CGPointValue];
    CGPoint new = [change[@"new"] CGPointValue];
    if (old.y == new.y) {
        return;
    }
    
    [self setYPosition];
    
    if (self.state == MJRefreshStateRefreshing || (!self.canRebound && self.autoLoadMore)) {
        [self setInsetBottom:self.mj_h];
    }
}
#pragma clang diagnostic pop

- (BOOL)tryRefresh:(CGFloat)triggerOffset
{
    if (self.scrollView.mj_contentH <= self.scrollView.mj_h) {  // The content is not full.
        if (self.scrollView.mj_offsetY >= triggerOffset) {
            return YES;
        }
    }
    else {  // The content is full.
        if (self.scrollView.mj_offsetY >= self.scrollView.mj_contentH - self.scrollView.mj_h + triggerOffset) {
            return YES;
        }
    }
    
    return NO;
}
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wobjc-missing-super-calls"
- (void)scrollViewContentOffsetDidChange:(NSDictionary *)change
{
    CGPoint old = [change[@"old"] CGPointValue];
    CGPoint new = [change[@"new"] CGPointValue];
    BOOL canRefresh = NO;
    // On the iOS side, MJRefresh monitors the scrolling of UIScrollView, and then judges whether to trigger the loadmore event
    // according to the scrolling state and contentoffset. However, in the case of mixed nested scrolling on the client side,
    // the scrolling state of UIScrollView is disabled, which will cause the loadmore event to fail to trigger. Therefore, we
    // add a switch to control whether to trigger the loadmore event in this case.
    BOOL canLoadMore = !self.scrollView.scrollEnabled && self.canLoadMoreWhenScrollDisabled;
    
    // Prevent triggering footeroffset when header refresh ends.
    if (old.y <= 0) {
        return;
    }
    
    if ([self tryRefresh:0.5]) {
        CGFloat footerOffset = (self.scrollView.mj_contentH <= self.scrollView.mj_h) ? self.scrollView.mj_offsetY : (self.scrollView.mj_offsetY - self.scrollView.mj_contentH + self.scrollView.mj_h);
        CGFloat footerHeight = self.mj_h == 0 ? 1 : self.mj_h;
        self.pullingPercent = footerOffset / footerHeight;
    }
    
    // refresh is no longer triggered during refreshing.
    if (self.state == MJRefreshStateRefreshing || (!self.canLoadMoreWhenScrollDisabled && self.triggerTimes <= 0)) {
        return;
    }
    
    if (self.autoLoadMore) {
        // Attempts to trigger a refresh when the footer appears.
        if (self.scrollView.isTracking || (self.scrollView.isDecelerating && old.y > new.y) || canLoadMore) {
            canRefresh = [self tryRefresh:0.5];
        }
    }
    else {
        // Attempts to trigger a refresh when the finger is lifted with the footer appears totally.
        if (!self.scrollView.isDragging || canLoadMore) {
            // When the finger is lifted with the footer appears totally and enableAutoLoadMore is false, trigger the
            // footerreleased event.
          if ((canRefresh = [self tryRefresh:(self.mj_h == 0 ? 1 : self.mj_h)])) {
                if ([self.refreshingTarget respondsToSelector:self.releasedAction]) {
                    MJRefreshMsgSend(MJRefreshMsgTarget(self.refreshingTarget), self.releasedAction, self);
                }
            }
        }
    }
    
    if (canRefresh) {
        struct objc_super superReceiver = { self, [MJRefreshComponent class] };
        void (*funcCall)(void *, SEL) = (void *)objc_msgSendSuper;
        if ([MJRefreshComponent instancesRespondToSelector:@selector(beginRefreshing)]) {
            funcCall(&superReceiver, @selector(beginRefreshing));
        }
    }
}
#pragma clang diagnostic pop

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wobjc-missing-super-calls"
- (void)scrollViewPanStateDidChange:(NSDictionary *)change
{
    // triggerTimes is not resetted during refreshing.
    if (self.state == MJRefreshStateRefreshing) return;
    
    if (self.scrollView.panGestureRecognizer.state == UIGestureRecognizerStateBegan) {
        self.triggerTimes = 1;
    }
}
#pragma clang diagnostic pop

- (void)setInsetBottom:(CGFloat)insetBottom
{
    // When the content height doesn't exceed the window height, we need to increase contentinset.bottom so that the
    // footer can stay when pull-up finished.
    // When the content height is 0, calling finishLoadMore will trigger a bug in the compatibility logic, causing 
    // the scrollable area at the bottom of the scrollable container to add the height of the scrollable container, 
    // causing the UI to display abnormally. Therefore, we added a null protection in setInsetBottom.
    if (fabs(self.scrollView.mj_contentH) > CGFLOAT_EPSILON && self.scrollView.mj_contentH <= self.scrollView.mj_h) {
        insetBottom += self.scrollView.mj_h - self.scrollView.mj_contentH;
    }
    
    if (self.scrollView.mj_insetB != insetBottom) {
        self.scrollView.mj_insetB = insetBottom;
    }
}

- (void)rebound
{
    // Decrease contentinset.bottom to make footer rebound and hide.
    if (self.canRebound) {
        [UIView animateWithDuration:MJRefreshSlowAnimationDuration
                         animations:^{
            [self setInsetBottom:0];
        }
                         completion:^(BOOL finished) {
            if (self.endRefreshingCompletionBlock) {
                self.endRefreshingCompletionBlock();
            }
        }];
    }
    else {
        if (self.endRefreshingCompletionBlock) {
            self.endRefreshingCompletionBlock();
        }
    }
}

- (void)footerRefreshingAction
{
    __weak typeof(self) weakSelf = self;
    dispatch_async(dispatch_get_main_queue(), ^{
        typeof(weakSelf) self = weakSelf;
        if (self.canRebound) {
            [UIView animateWithDuration:MJRefreshFastAnimationDuration
                             animations:^{
                [self setInsetBottom:self.mj_h];
                
                // Set the footer's stop position.
                CGPoint offset = self.scrollView.contentOffset;
                CGFloat offsetY = 0;
                if (self.scrollView.mj_contentH > self.scrollView.mj_h) {
                    offsetY = self.scrollView.mj_contentH - self.scrollView.mj_h;
                }
                if (offset.y > offsetY + self.mj_h)
                {
                    offset.y = offsetY + self.mj_h;
                }
                [self.scrollView setContentOffset:offset animated:NO];
            }
                             completion:^(BOOL finished) {
                [self executeRefreshingCallback];
            }];
        }
        else {
            [self executeRefreshingCallback];
        }
    });
}

- (void )footerEndAction
{
    [self rebound];
    self.pullingPercent = 0;
    self.triggerTimes = 0;
    
    if (self.scrollView.pagingEnabled) {
        CGPoint offset = self.scrollView.contentOffset;
        offset.y -= self.mj_h;
        [UIView animateWithDuration:MJRefreshSlowAnimationDuration
                         animations:^{ self.scrollView.contentOffset = offset; }];
    }
}

- (void)setState:(MJRefreshState)state
{
    MJRefreshState oldState = self.state;
    if (state == oldState) return;

    // Call the setState of MJRefreshComponent to avoid triggering startloadmore twice.
    struct objc_super superReceiver = { self, [MJRefreshComponent class] };
    void (*funcCall)(void *, SEL, MJRefreshState) = (void *)objc_msgSendSuper;
    funcCall(&superReceiver, _cmd, state);
    
    if (state == MJRefreshStateRefreshing && self.hasMore == YES) {
        // When trigger refresh.
        [self footerRefreshingAction];
    }
    else if (state == MJRefreshStateNoMoreData || state == MJRefreshStateIdle) {
        // when refresh is complete or trigger finishLoadMore with has_more being false or first render.
        [self footerEndAction];
    }
}
@end

@implementation LynxUIRefreshView

@end

@interface LynxUIRefresh () <BDXRefreshDelegate>

@property (nonatomic, assign) CGRect selfFrame;
@property (nonatomic, strong) UIScrollView *scrollView;
@property (nonatomic, weak) LynxUIRefreshHeader *lynxHeader;
//@property (nonatomic, weak) BDXLynxRefreshFooter *lynxFooter;
@property (nonatomic, weak) LynxUI *lynxList;
@property (nonatomic, assign) BOOL enableRefresh;
@property (nonatomic, assign) BOOL enableLoadMore;
@property (nonatomic, assign) BOOL enableAutoLoadMore;
@property (nonatomic, assign) BOOL enableFooterRebound;
@property (nonatomic, assign) BOOL manualRefresh;
@property (nonatomic, assign) BOOL enableLoadMoreWhenScrollDisabled;

@end

@implementation LynxUIRefresh

- (instancetype)init
{
    self = [super init];
    if (self) {
        _enableRefresh = true;
        _enableLoadMore = true;
        _enableAutoLoadMore = true;
        _enableFooterRebound = false;
        _manualRefresh = true;
        _enableLoadMoreWhenScrollDisabled = NO;
    }
    return self;
}

- (UIView *)createView {
    LynxUIRefreshView *view = [LynxUIRefreshView new];
    return view;
}

- (void)insertChild:(LynxUI *)child atIndex:(NSInteger)index {
    [super insertChild:child atIndex:index];
//    if ([child isKindOfClass:[BDXLynxRefreshFooter class]]) {
//        self.lynxFooter = (BDXLynxRefreshFooter *)child;
//    } else 
      if ([child isKindOfClass:[LynxUIRefreshHeader class]]) {
        self.lynxHeader = (LynxUIRefreshHeader *)child;
    } else {
        self.lynxList = child;
    }
}



- (void)finishLayoutOperation {
    UIScrollView *preScrollView = self.scrollView;
    { // re-fetch scrollview
        NSMutableArray *excludeViews = [NSMutableArray array];
        if (self.lynxHeader.view) {
            [excludeViews addObject:self.lynxHeader.view];
        }
//        if (self.lynxFooter.view) {
//            [excludeViews addObject:self.lynxFooter.view];
//        }
//        if ([self.lynxList.view conformsToProtocol:@protocol(LynxUIRefreshConfigInject)]) {
//            id<LynxUIRefreshConfigInject> inject = (id)self.lynxList.view;
//            __weak typeof(self) weakSelf = self;
//            [inject refreshViewInjection:^(UIScrollView * _Nonnull scrollView) {
//                __strong typeof(weakSelf) strongSelf = weakSelf;
//                if (![scrollView isKindOfClass:[UIScrollView class]]) {
//                    return;
//                }
//                strongSelf.scrollView = scrollView;
//                [strongSelf loadHeaderAndFooter];
//            }];
//        } else
        
        {
            self.scrollView = (UIScrollView *)[self findViewWithKind:[UIScrollView class] fromView:[self view] excludeViews:excludeViews];
            if (!self.scrollView) {
                return;
            }
        }
    }
    if (!CGRectEqualToRect(self.selfFrame, self.view.frame) || self.scrollView != preScrollView) {
        self.selfFrame = self.view.frame;
        self.scrollView.frame = self.view.bounds;
        [self loadHeaderAndFooter];
    }
}

- (id<LynxEventTarget>)hitTest:(CGPoint)point withEvent:(UIEvent *)event {
//    CGPoint fp = [[self view] convertPoint:point toView:self.lynxFooter.view];
    CGPoint hp = [[self view] convertPoint:point toView:self.lynxHeader.view];
    CGPoint pt = [[self view] convertPoint:point toView:self.lynxList.view];
    if([self.lynxHeader.view pointInside:hp withEvent:event]) {
        return [self.lynxHeader hitTest:hp withEvent:event];
    }
    
//    if([self.lynxFooter.view pointInside:fp withEvent:event]) {
//        return [self.lynxFooter hitTest:fp withEvent:event];
//    }
//    
    if ([self.lynxList.view pointInside:pt withEvent:event]) {
      // (pt) is resulted by adding contentOffset
      return [self.lynxList hitTest:pt withEvent:event];;
    }
    return [super hitTest:point withEvent:event];
}

#pragma mark - Private

- (void)loadHeaderAndFooter {
    [self loadHeader];
//    [self loadFooter];
}

- (void)loadHeader {
    if (self.enableRefresh && self.lynxHeader.view) {
        BDXRefreshHeader *header = [BDXRefreshHeader headerWithRefreshingTarget:self refreshingAction:@selector(startRefresh)];
        [header withReleasedAction:@selector(headerReleased)];
        header.delegate = self;
        header.mj_h = self.lynxHeader.view.bounds.size.height;
        header.headerView.frame = self.lynxHeader.view.bounds;
        [header.headerView addSubview:self.lynxHeader.view];
        self.scrollView.mj_header = header;
    } else {
        [self.lynxHeader.view removeFromSuperview];
    }
}

//- (void)loadFooter {
//    if (self.enableLoadMore && self.lynxFooter.view) {
//        BDXRefreshFooter *footer = [BDXRefreshFooter footerWithRefreshingTarget:self refreshingAction:@selector(startLoadMore)];
//        [footer withReleasedAction:@selector(footerReleased)];
//        footer.delegate = self;
//        footer.mj_h = self.lynxFooter.view.bounds.size.height;
//        footer.footerView = self.lynxFooter.view;
//        [footer addSubview:self.lynxFooter.view];
//        self.scrollView.mj_footer = footer;
//        footer.autoLoadMore = self.enableAutoLoadMore;
//        footer.canRebound = self.enableFooterRebound;
//        footer.canLoadMoreWhenScrollDisabled = self.enableLoadMoreWhenScrollDisabled;
//    } else {
//        [self.lynxFooter.view removeFromSuperview];
//    }
//}

- (void)startRefresh {
    if (self.enableRefresh && self.lynxHeader.view) {
        LynxCustomEvent *event = [[LynxDetailEvent alloc] initWithName:@"startrefresh" targetSign:[self sign] detail:@{
            @"isManual": @(self.manualRefresh)
        }];
        [self.context.eventEmitter sendCustomEvent:event];
    }
    self.manualRefresh = true;
}

//- (void)startLoadMore {
//    if (self.enableLoadMore && self.lynxFooter.view) {
//        LynxCustomEvent *event = [[LynxDetailEvent alloc] initWithName:@"startloadmore" targetSign:[self sign] detail:@{}];
//        [self.context.eventEmitter sendCustomEvent:event];
//    }
//}

- (void)headerReleased {
    if (self.enableRefresh && self.lynxHeader.view) {
        // When the finger is lifted, if the drop-down distance of the header is greater than or equal to the height of
        // the header, the headerreleased event will be triggered. It is before the startrefresh event.
        LynxCustomEvent *event = [[LynxDetailEvent alloc] initWithName:@"headerreleased" targetSign:[self sign] detail:@{}];
        [self.context.eventEmitter sendCustomEvent:event];
    }
}

//- (void)footerReleased {
//    if (self.enableLoadMore && self.lynxFooter.view) {
//        // When the finger is lifted with enableAutoLoadMore is true, if the drop-up distance of the footer is greater than
//        // or equal to the height of the footer, the footerreleased event will be triggered.
//        // It is before the startloadmore event.
//        LynxCustomEvent *event = [[LynxDetailEvent alloc] initWithName:@"footerreleased" targetSign:[self sign] detail:@{}];
//        [self.context.eventEmitter sendCustomEvent:event];
//    }
//}

- (nullable UIView *)findViewWithKind:(Class)aClass fromView:(UIView *)view excludeViews:(nullable NSArray<UIView *> *)excludeViews {
    
    __block UIView * reslutview ;
    NSArray * subviews = view.subviews;

    if (view.subviews.count <= 0) {
        return nil;
    }

    [subviews enumerateObjectsUsingBlock:^(UIView * subView, NSUInteger idx, BOOL * _Nonnull stop) {
        if ([subView isKindOfClass:aClass]) {
            reslutview = subView;
            *stop = YES;
        }
    }];

    if (!reslutview) {
        [subviews enumerateObjectsUsingBlock:^(UIView * subView, NSUInteger idx, BOOL * _Nonnull stop) {
            if (![excludeViews containsObject:subView]) {
                reslutview = [self findViewWithKind:aClass fromView:subView excludeViews:excludeViews];
                if (reslutview) {
                    *stop = YES;
                }
            }
        }];
    }

    return reslutview;
}

#pragma mark - BDXRefreshDelegate

- (void)refreshview:(MJRefreshComponent *)refreshview didUpdatePullingPrecent:(CGFloat)pullingPercent {
  
  // Ensure that the header is not obscured by elements inside the scrollview.
  if (self.scrollView.mj_header.layer.zPosition != CGFLOAT_MAX) {
    self.scrollView.mj_header.layer.zPosition = CGFLOAT_MAX;
  }
  
    if (refreshview == self.scrollView.mj_footer) {
        LynxCustomEvent *event = [[LynxDetailEvent alloc] initWithName:@"footeroffset" targetSign:[self sign] detail:@{
            @"isDragging" : @(refreshview.scrollView.isDragging),
            @"offsetPercent" : @(pullingPercent)
        }];
        [self.context.eventEmitter sendCustomEvent:event];
    }
}

- (void)refreshview:(MJRefreshComponent *)refreshview didUpdateShowingPrecent:(CGFloat)showingPercent {
    // legacy API 
    if (refreshview == self.scrollView.mj_header) {
        LynxCustomEvent *event = [[LynxDetailEvent alloc] initWithName:@"headershow" targetSign:[self sign] detail:@{
            @"isDragging" : @(refreshview.scrollView.isDragging),
            @"offsetPercent" : @(showingPercent)
        }];
        [self.context.eventEmitter sendCustomEvent:event];
    }
    if (refreshview == self.scrollView.mj_header) {
      LynxCustomEvent *event = [[LynxDetailEvent alloc] initWithName:@"headeroffset" targetSign:[self sign] detail:@{
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

LYNX_PROP_SETTER("enable-loadmore", enableLoadMore, BOOL) {
    if (_enableLoadMore != value) {
        _enableLoadMore = value;
//        [self loadFooter];
    }
}

LYNX_UI_METHOD(finishRefresh) {
    [self.scrollView.mj_header endRefreshing];
    if (callback) {
      callback(kUIMethodSuccess, nil);
    }
}

LYNX_UI_METHOD(finishLoadMore) {
    BOOL value = ((BDXRefreshFooter *)self.scrollView.mj_footer).hasMore;
    if ([[params allKeys] containsObject:@"has_more"]) {
        value = [[params objectForKey:@"has_more"] boolValue];
        ((BDXRefreshFooter *)self.scrollView.mj_footer).hasMore = value;
    }
    
    if (value) {
        [self.scrollView.mj_footer endRefreshing];
    } else {
        [self.scrollView.mj_footer endRefreshingWithNoMoreData];
    }
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

LYNX_PROPS_GROUP_DECLARE(
	LYNX_PROP_DECLARE("ios-enable-loadmore-when-scroll-disabled", setIosEnableLoadmoreWhenScrollDisabled, BOOL),
	LYNX_PROP_DECLARE("enable-footer-rebound", setEnableFooterRebound, BOOL),
    LYNX_PROP_DECLARE("enable-auto-loadmore", setEnableAutoLoadmore, BOOL))

/**
 * @name: enable-auto-loadmore
 * @description: Sets whether to trigger a load more event automatically when scrolling to the bottom
 * @category: stable
 * @standardAction: keep
 * @supportVersion: 2.8
**/
LYNX_PROP_DEFINE("enable-auto-loadmore", setEnableAutoLoadmore, BOOL) {
    _enableAutoLoadMore = value;
    if (self.scrollView.mj_footer) {
        ((BDXRefreshFooter *)self.scrollView.mj_footer).autoLoadMore = self.enableAutoLoadMore;
    }
}

/**
 * @name: enable-footer-rebound
 * @description: Control whether the footer can automatically rebound by calling finishLoadMore
 * @category: temporary
 * @standardAction: remove
 * @supportVersion: 2.8
**/
LYNX_PROP_DEFINE("enable-footer-rebound", setEnableFooterRebound, BOOL) {
    _enableFooterRebound = value;
    if (self.scrollView.mj_footer) {
        ((BDXRefreshFooter *)self.scrollView.mj_footer).canRebound = self.enableFooterRebound;
    }
}

/**
 * @name: ios-enable-loadmore-when-scroll-disabled
 * @description: Usually, the loadmore event will be triggered only if the UIScrollView is scrollable. However, sometimes, the Native Container needs to manage the scrolling behavior themselves by disabling the scroll ability of Lynx's UIScrollView. Therefore, this switch enables the loadmore event even if our scroll is disabled.
 * @category: stable
 * @standardAction: keep
 * @supportVersion: 2.10
**/
LYNX_PROP_DEFINE("ios-enable-loadmore-when-scroll-disabled", setIosEnableLoadmoreWhenScrollDisabled, BOOL) {
    _enableLoadMoreWhenScrollDisabled = value;
    if (self.scrollView.mj_footer) {
        ((BDXRefreshFooter *)self.scrollView.mj_footer).canLoadMoreWhenScrollDisabled = self.enableLoadMoreWhenScrollDisabled;
    }
}

@end
