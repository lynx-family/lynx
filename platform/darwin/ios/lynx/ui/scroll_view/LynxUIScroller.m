// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxUIScroller.h"
#import <objc/runtime.h>
#import "LynxComponentRegistry.h"
#import "LynxLayoutStyle.h"
#import "LynxPropsProcessor.h"
#import "LynxUI+Fluency.h"
#import "LynxUI+Internal.h"
#import "LynxUIMethodProcessor.h"
#import "LynxWeakProxy.h"

#import "LynxBounceView.h"
#import "LynxGlobalObserver.h"
#import "LynxImpressionView.h"
#import "LynxPropertyDiffMap.h"
#import "LynxScrollEventManager.h"
#import "LynxScrollView.h"
#import "LynxTemplateRender+Internal.h"
#import "LynxTraceEvent.h"
#import "LynxTraceEventWrapper.h"
#import "LynxUI+Gesture.h"
#import "LynxUICollection.h"
#import "LynxUIContext+Internal.h"
#import "LynxUnitUtils.h"
#import "LynxView+Internal.h"
#import "LynxView.h"
#import "UIScrollView+Lynx.h"
#import "UIScrollView+LynxFadingEdge.h"
#import "UIScrollView+LynxGesture.h"
#import "UIScrollView+Nested.h"

const NSInteger kScrollEdgeThreshold = 1;
const NSInteger kInvalidBounceDistance = -1;
const NSInteger kScrollToCallBackNonChange = -1;
static const CGFloat SCROLL_BY_EPSILON = 0.1f;

typedef void (^LynxUIScrollToCallBack)(int code, id _Nullable data);

@interface LynxUIScrollerProxy : NSObject
@property(nonatomic, weak) LynxUIScroller *scroller;
@property(nonatomic, assign) CGFloat rate;
@property(nonatomic, assign) BOOL enableScrollY;

- (instancetype)initWithScroller:(LynxUIScroller *)scroller
                            rate:(CGFloat)rate
                   enableScrollY:(BOOL)enableScrollY;
- (void)displayLinkAction;
@end

// to mock native flick
@interface FlickParameter : NSObject
@end

@implementation FlickParameter {
  float _duration;
  float _delta;
}

- (id)initFlick:(float)initvelocity
    decelerationRate:(float)decelerationRate
           threshold:(float)threshold
    oppositeBoundary:(float)oppositeBoundary
    positiveBoundary:(float)positiveBoundary {
  self = [super init];
  if (fabs(initvelocity) < 1e-3) {
    _duration = 0;
    _delta = 0;
    return self;
  }
  float dCoeff = 1000 * logf(decelerationRate);
  _duration = logf(-dCoeff * threshold / fabs(initvelocity)) / dCoeff;
  _delta = -initvelocity / dCoeff;
  // Boundary check
  if (_delta > positiveBoundary || _delta < oppositeBoundary) {
    _delta = initvelocity > 0 ? positiveBoundary : oppositeBoundary;
    _duration = logf(dCoeff * _delta / initvelocity + 1.) / logf(decelerationRate) * 0.001;
  }
  return self;
}

- (float)delta {
  return _delta;
}

- (void)setContentOffset:(UIScrollView *)scrollView destination:(CGPoint)offset {
  if (scrollView == nil || _duration < 1e-3) {
    return;
  }
  [scrollView layoutIfNeeded];
  dispatch_async(dispatch_get_main_queue(), ^{
    [UIView animateWithDuration:self->_duration
                          delay:0.
                        options:UIViewAnimationOptionCurveEaseOut
                     animations:^(void) {
                       [scrollView setContentOffset:offset];
                     }
                     completion:NULL];
  });
}
@end

@interface UIScrollView (Impression) <LynxImpressionParentView>

@end

@implementation UIScrollView (Impression)

- (void)setShouldManualExposure:(BOOL)shouldManualExposure {
  objc_setAssociatedObject(self, @selector(shouldManualExposure), @(shouldManualExposure),
                           OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}

- (BOOL)shouldManualExposure {
  return [objc_getAssociatedObject(self, _cmd) boolValue];
}

@end

@implementation LynxUIScroller {
  BOOL _hasReachBottom;
  BOOL _hasReachTop;
  CGFloat _preScrollTop;
  CGFloat _preScrollLeft;
  CADisplayLink *_displayLink;
  NSMutableArray *_scrollerDelegates;
  NSInteger _upperThreshold;
  NSInteger _lowerThreshold;
  // for impression view
  CGFloat _sensitivity;
  BOOL _forceImpression;
  CGPoint _lastScrollPoint;

  HoverPosition hoverPosition;
  CGFloat _triggerBounceEventDistance;
  CGFloat _fadingEdge;
  BOOL _nestedUpdated;
  LynxUIScrollToCallBack _scrollToCallBack;

  // For list native storage
  NSInteger _listSign;
  // value may not be the latest as reused UI with same prop will not update propMap. Instead, it
  // will update through onNodeReady
  LynxPropertyDiffMap *_propMap;
}

#if LYNX_LAZY_LOAD
LYNX_LAZY_REGISTER_UI("scroll-view")
#else
LYNX_REGISTER_UI("scroll-view")
#endif

static Class<LynxScrollViewUIDelegate> kUIDelegate = nil;
+ (Class<LynxScrollViewUIDelegate>)UIDelegate {
  return kUIDelegate;
}

+ (void)setUIDelegate:(Class<LynxScrollViewUIDelegate>)UIDelegate {
  kUIDelegate = UIDelegate;
}

- (instancetype)init {
  self = [super init];
  if (self) {
    _enableScrollY = NO;
    _hasReachBottom = NO;
    _hasReachTop = NO;
    _preScrollTop = 0;
    _preScrollLeft = 0;
    _enableSticky = NO;
    _scrollerDelegates = [NSMutableArray new];
    _lowerThreshold = 0;
    _upperThreshold = 0;
    // for impression view
    _sensitivity = 4.f;
    _forceImpression = NO;
    _lastScrollPoint = CGPointMake(INFINITY, INFINITY);
    _nestedUpdated = NO;
    self.firstRender = YES;
    [self ensureUpdateContentSize];
    _propMap = [[LynxPropertyDiffMap alloc] init];
  }
  return self;
}

- (UIView *)createView {
  LynxScrollView *scrollView = [LynxScrollView new];
  scrollView.autoresizesSubviews = NO;
  scrollView.clipsToBounds = YES;
  scrollView.showsVerticalScrollIndicator = NO;
  scrollView.showsHorizontalScrollIndicator = NO;
  scrollView.scrollEnabled = YES;
  scrollView.delegate = self;
  scrollView.enableNested = NO;
  scrollView.scrollY = NO;
  scrollView.weakUIScroller = self;
  if (@available(iOS 11.0, *)) {
    scrollView.contentInsetAdjustmentBehavior = UIScrollViewContentInsetAdjustmentNever;
  }

  scrollView.shouldManualExposure = YES;
  [[NSNotificationCenter defaultCenter]
      addObserver:self
         selector:@selector(lynxImpressionWillManualExposureNotification:)
             name:LynxImpressionWillManualExposureNotification
           object:nil];

  return scrollView;
}

- (void)adjustContentOffsetForRTL:(CGFloat)prevXOffset {
  // for update, the content offset should be reset to old position instead of edge.
  if (!_enableScrollY && self.isRtl) {
    float lengthToShift = self.view.contentSize.width - self.view.frame.size.width;
    id scrollDelegate = self.view.delegate;
    self.view.delegate = nil;
    [[self view]
        setContentOffset:CGPointMake(MAX(lengthToShift - prevXOffset, -self.view.contentInset.left),
                                     self.view.contentOffset.y)
                animated:NO];
    self.view.delegate = scrollDelegate;
  }
}

- (void)propsDidUpdate {
  [super propsDidUpdate];
  self.view.name = self.name;
}

- (void)layoutDidFinished {
  [self updateContentSize];
  if (_enableSticky) {
    [self onScrollSticky:self.view.contentOffset.x withOffsetY:self.view.contentOffset.y];
  }
  _lastScrollPoint = CGPointMake(INFINITY, INFINITY);
  [self triggerSubviewsImpression];
  if (_lowerBounceUI || _upperBounceUI || _defaultBounceUI) {
    [self autoAddBounceView];
  }
}

- (void)ensureUpdateContentSize {
  [self.nodeReadyBlockArray addObject:^(LynxUI *ui) {
    LynxUIScroller *scroll = (LynxUIScroller *)ui;
    /*
      As layoutDidFinished was postpone after a patch, it's now later than onNodeReady. Thus, we
      need to call an additional updateContentSize before the flush of readyBlockArray, or
      scroll-left/top and scrollToIndex will fail due to zero contentSize.
    */
    [scroll updateContentSize];
  }];
}

- (void)onNodeReload {
  [super onNodeReload];
  [self resetContentOffset];
}

- (void)onNodeReady {
  [super onNodeReady];

  [self.view updateFadingEdgeWithSize:_fadingEdge horizontal:!self.enableScrollY];
  // Handle enable-nested
  [(UIScrollView *)self.view updateChildren];

  [self ensureUpdateContentSize];
  self.firstRender = NO;
}

- (void)insertChild:(LynxUI *)child atIndex:(NSInteger)index {
  if ([child isKindOfClass:LynxBounceView.class]) {
    LynxBounceView *bounceChild = (LynxBounceView *)child;
    if (bounceChild.direction == LynxBounceViewDirectionTop ||
        bounceChild.direction == LynxBounceViewDirectionLeft) {
      _upperBounceUI = bounceChild;
    } else {
      _lowerBounceUI = bounceChild;
    }
  }

  [super insertChild:child atIndex:index];
}

- (void)applyRTL:(BOOL)rtl {
  ((LynxScrollView *)self.view).isRTL = rtl;
}

- (void)updateContentSize {
  float contentWidth = 0;
  float contentHeight = 0;
  if (!_enableScrollY) {
    for (LynxUI *child in self.children) {
      if (![child isKindOfClass:LynxBounceView.class]) {
        contentWidth = MAX(contentWidth, child.updatedFrame.size.width +
                                             child.updatedFrame.origin.x + child.margin.right);
      }
    }
    contentWidth += self.padding.right;
    contentHeight = self.frame.size.height - self.padding.bottom - self.padding.top;
  } else {
    for (LynxUI *child in self.children) {
      if (![child isKindOfClass:LynxBounceView.class]) {
        contentHeight = MAX(contentHeight, child.updatedFrame.size.height +
                                               child.updatedFrame.origin.y + child.margin.bottom);
      }
    }
    contentHeight += self.padding.bottom;
    contentWidth = self.frame.size.width - self.padding.left - self.padding.right;
  }

  if ([self view].contentSize.width != contentWidth ||
      [self view].contentSize.height != contentHeight) {
    CGFloat prevXOffset =
        self.view.contentSize.width - self.view.contentOffset.x - self.view.frame.size.width;
    [self view].contentSize = CGSizeMake(contentWidth, contentHeight);
    [self adjustContentOffsetForRTL:MAX(prevXOffset, -self.view.contentInset.right)];
    [self contentSizeDidChanged];
  }
}

- (CGPoint)contentOffset {
  return self.view.contentOffset;
}

- (BOOL)isScrollContainer {
  return YES;
}

- (void)resetContentOffset {
  if (self.enableScrollY) {
    self.view.contentOffset = CGPointMake(self.view.contentOffset.x, -self.view.contentInset.top);
  } else {
    if (self.isRtl) {
      self.view.contentOffset =
          CGPointMake(MAX(self.view.contentSize.width - self.view.frame.size.width +
                              self.view.contentInset.right,
                          -self.view.contentInset.right),
                      self.view.contentOffset.y);
    } else {
      self.view.contentOffset =
          CGPointMake(-self.view.contentInset.left, self.view.contentOffset.y);
    }
  }
}

- (void)setScrollY:(BOOL)value requestReset:(BOOL)requestReset {
  if (requestReset) {
    value = NO;
  }
  _enableScrollY = value;
  self.view.scrollY = value;
  [self updateContentSize];
}

- (void)setScrollX:(BOOL)value requestReset:(BOOL)requestReset {
  if (requestReset) {
    value = NO;
  }
  _enableScrollY = !value;
  self.view.scrollY = !value;
  [self updateContentSize];
}

- (void)setScrollOrientation:(NSString *)value requestReset:(BOOL)requestReset {
  BOOL scrollY = _enableScrollY;

  if (requestReset) {
    value = @"vertical";
  }
  if ([value isEqualToString:@"vertical"]) {
    scrollY = YES;
  } else if ([value isEqualToString:@"horizontal"]) {
    scrollY = NO;
  }
  //(TODO)fangzhou.fz: If it becomes necessary in the future, extend the 'both' mode.

  [self setEnableScrollY:scrollY];
  [self updateContentSize];
}

- (void)setEnableScrollY:(BOOL)enableScrollY {
  _enableScrollY = enableScrollY;
  self.view.scrollY = _enableScrollY;
}

- (void)setScrollYReverse:(BOOL)value requestReset:(BOOL)requestReset {
  if (requestReset) {
    value = NO;
  }
  _enableScrollY = value;
  [self updateContentSize];
  CGFloat offsetY = self.view.contentOffset.y;
  if (value) {
    offsetY = self.view.contentSize.height - self.view.frame.size.height;
  }
  [[self view] setContentOffset:CGPointMake(self.view.contentOffset.x, offsetY) animated:NO];
}

- (void)setScrollXReverse:(BOOL)value requestReset:(BOOL)requestReset {
  if (requestReset) {
    value = NO;
  }
  _enableScrollY = !value;
  [self updateContentSize];
  CGFloat offsetX = self.view.contentOffset.x;
  if (value) {
    offsetX = self.view.contentSize.width - self.view.frame.size.width;
  }
  [[self view] setContentOffset:CGPointMake(offsetX, self.view.contentOffset.y) animated:NO];
}

- (void)setEnableNested:(BOOL)value requestReset:(BOOL)requestReset {
  if (requestReset) {
    value = NO;
  }
  self.view.enableNested = value;
}

- (void)setScrollLeft:(int)value requestReset:(BOOL)requestReset {
  if (requestReset) {
    value = 0;
  }

  [self.nodeReadyBlockArray addObject:^(LynxUI *ui) {
    LynxUIScroller *scroll = (LynxUIScroller *)ui;
    scroll.view.contentOffset = [scroll clampScrollToPosition:value callback:nil isSmooth:NO];
  }];
}

- (void)setScrollTop:(int)value requestReset:(BOOL)requestReset {
  if (requestReset) {
    value = 0;
  }

  [self.nodeReadyBlockArray addObject:^(LynxUI *ui) {
    LynxUIScroller *scroll = (LynxUIScroller *)ui;
    scroll.view.contentOffset = [scroll clampScrollToPosition:value callback:nil isSmooth:NO];
  }];
}

- (void)initialScrollOffsetInner:(int)value {
  self.view.contentOffset = [self clampScrollToPosition:value callback:nil isSmooth:NO];
}

- (void)setInitialScrollOffset:(int)value requestReset:(BOOL)requestReset {
  if (requestReset) {
    value = 0;
    // reset means this UI now don't have this prop
    [_propMap deleteKey:LynxScrollViewInitialScrollOffset];
  } else {
    // Keep the _propMap the latest updated value. It is allowed to change initial-offset more than
    // once before the first render.
    [_propMap putValue:@(value) forKey:LynxScrollViewInitialScrollOffset];
  }
  [self.nodeReadyBlockArray addObject:^(LynxUI *ui) {
    LynxUIScroller *scroll = (LynxUIScroller *)ui;
    // This UI is first rendering or it is inside list-item and reused, and this
    // spot(itemKey-idSelector) in list is first rendering
    if (scroll.firstRender ||
        (!requestReset && ![scroll initialPropsFlushed:LynxScrollViewInitialScrollOffset])) {
      [scroll setInitialPropsHasFlushed:LynxScrollViewInitialScrollOffset];
      [scroll initialScrollOffsetInner:value];
    }
  }];
}

- (void)setScrollToIndex:(int)value requestReset:(BOOL)requestReset {
  if (requestReset) {
    value = 0;
  }

  [self.nodeReadyBlockArray addObject:^(LynxUI *ui) {
    LynxUIScroller *scroll = (LynxUIScroller *)ui;
    if ([scroll view].subviews.count == 0 || value < 0) {
      return;
    }
    if (value >= 0 && (NSUInteger)value < [scroll.children count]) {
      LynxUI *target = [scroll.children objectAtIndex:value];
      CGPoint targetOffset =
          scroll.enableScrollY
              ? [scroll clampScrollToPosition:target.view.frame.origin.y callback:nil isSmooth:NO]
              : [scroll clampScrollToPosition:target.view.frame.origin.x callback:nil isSmooth:NO];
      [scroll.view setContentOffset:targetOffset];
    }
  }];
}

- (void)initialScrollToIndexInner:(int)value {
  if (self.view.subviews.count == 0 || value < 0) {
    return;
  }
  if (value >= 0 && (NSUInteger)value < [self.children count]) {
    LynxUI *target = [self.children objectAtIndex:value];
    CGPoint targetOffset =
        self.enableScrollY
            ? [self clampScrollToPosition:target.view.frame.origin.y callback:nil isSmooth:NO]
            : [self clampScrollToPosition:target.view.frame.origin.x callback:nil isSmooth:NO];
    [self.view setContentOffset:targetOffset];
  }
}

- (void)setInitialScrollToIndex:(int)value requestReset:(BOOL)requestReset {
  if (requestReset) {
    value = 0;
    // reset means this UI (maybe reused from elsewhere) now don't have this prop
    [_propMap deleteKey:LynxScrollViewInitialScrollIndex];
  } else {
    // Keep the _propMap the latest updated value. It is allowed to change initial-to-index more
    // than once before the first render.
    [_propMap putValue:@(value) forKey:LynxScrollViewInitialScrollIndex];
  }
  [self.nodeReadyBlockArray addObject:^(LynxUI *ui) {
    LynxUIScroller *scroll = (LynxUIScroller *)ui;
    if (scroll.firstRender ||
        (!requestReset && ![scroll initialPropsFlushed:LynxScrollViewInitialScrollIndex])) {
      [scroll setInitialPropsHasFlushed:LynxScrollViewInitialScrollIndex];
      [scroll initialScrollToIndexInner:value];
    }
  }];
}

- (void)setEnableScrollMonitor:(BOOL)value requestReset:(BOOL)requestReset {
  if (requestReset) {
    value = NO;
  }
  _enableScrollMonitor = value;
}

- (void)setScrollMonitorTag:(NSString *)value requestReset:(BOOL)requestReset {
  if (requestReset) {
    value = nil;
  }
  _scrollMonitorTagName = value;
}

- (void)setScrollBarEnable:(BOOL)value requestReset:(BOOL)requestReset {
  if (requestReset) {
    value = NO;
  }
  self.view.showsVerticalScrollIndicator = value;
  self.view.showsHorizontalScrollIndicator = value;
}

- (void)setBounces:(BOOL)value requestReset:(BOOL)requestReset {
  if (requestReset) {
    value = YES;
  }
  self.view.bounces = value;
}

- (void)setEnableScroll:(BOOL)value requestReset:(BOOL)requestReset {
  if (requestReset) {
    value = YES;
  }
  self.view.scrollEnabled = value;
}

- (void)setFadingEdge:(NSString *)value requestReset:(BOOL)requestReset {
  if (requestReset) {
    value = nil;
  }

  _fadingEdge = (NSInteger)[self toPtWithUnitValue:value fontSize:0];
}

- (void)exprIOSIncreaseFrequency:(BOOL)value requestReset:(BOOL)requestReset {
  if (requestReset) {
    value = NO;
  }
  ((LynxScrollView *)self.view).increaseFrequencyWithGesture = value;
  [self enableIncreaseFrequencyIfNecessary];
}

- (void)enableIncreaseFrequencyIfNecessary {
  if (((LynxScrollView *)self.view).gestureEnabled &&
      ((LynxScrollView *)self.view).increaseFrequencyWithGesture) {
    // Allow native scroll make sure that the CPU frequency will be increased during scroll
    self.view.scrollEnabled = YES;
  } else {
    // When we add new gesture, forbid the default scrolling behaviors
    self.view.scrollEnabled = NO;
  }
}

- (void)addScrollerDelegate:(id<LynxUIScrollerDelegate>)delegate {
  LynxWeakProxy *proxy = [LynxWeakProxy proxyWithTarget:delegate];
  [_scrollerDelegates addObject:proxy];
}

- (void)removeScrollerDelegate:(id<LynxUIScrollerDelegate>)delegate {
  for (LynxWeakProxy *proxy in _scrollerDelegates) {
    if (proxy.target == delegate) {
      [_scrollerDelegates removeObject:proxy];
      break;
    }
  }
}

- (void)setUpperThreshold:(NSInteger)value requestReset:(BOOL)requestReset {
  if (requestReset) {
    value = 0;
  }
  _upperThreshold = value;
}

- (void)setLowerThreshold:(NSInteger)value requestReset:(BOOL)requestReset {
  if (requestReset) {
    value = 0;
  }
  _lowerThreshold = value;
}

- (CGPoint)getHitTestPoint:(CGPoint)inPoint {
  return CGPointMake(
      self.view.contentOffset.x + inPoint.x - self.getTransationX - self.frame.origin.x,
      self.view.contentOffset.y + inPoint.y - self.getTransationY - self.frame.origin.y);
}

- (void)eventDidSet {
  [super eventDidSet];
  if (!_scrollEventManager) {
    _scrollEventManager = [[LynxScrollEventManager alloc] initWithContext:self.context
                                                                     sign:self.sign
                                                                 eventSet:self.eventSet];
  }
}

- (void)onScrollSticky:(CGFloat)offsetX withOffsetY:(CGFloat)offsetY {
  for (NSUInteger index = 0; index < self.children.count; index++) {
    LynxUI *ui = self.children[index];
    [ui checkStickyOnParentScroll:offsetX withOffsetY:offsetY];
  }
}

- (BOOL)shouldSendScrollToLowerEvent:(CGFloat)lowerThreshold {
  if (_enableScrollY) {
    return self.view.contentSize.height - self.view.contentOffset.y - lowerThreshold <=
           self.view.frame.size.height;
  } else {
    if (self.isRtl) {
      return self.view.contentOffset.x <= lowerThreshold;
    } else {
      return self.view.contentSize.width - self.view.contentOffset.x - lowerThreshold <=
             self.view.frame.size.width;
    }
  }
}

- (BOOL)shouldSendScrollToUpperEvent:(CGFloat)upperThreshold {
  if (_enableScrollY) {
    return self.view.contentOffset.y <= upperThreshold;
  } else {
    if (self.isRtl) {
      return self.view.contentSize.width - self.view.contentOffset.x - upperThreshold <=
             self.view.frame.size.width;
    } else {
      return self.view.contentOffset.x <= upperThreshold;
    }
  }
}

- (void)sendScrollEvent:(UIScrollView *)scrollView {
  CGFloat scrollTop = scrollView.contentOffset.y;
  CGFloat scrollLeft = scrollView.contentOffset.x;
  CGFloat deltaX = scrollLeft - _preScrollLeft;
  CGFloat deltaY = scrollTop - _preScrollTop;

  if (ABS(deltaX) <= CGFLOAT_EPSILON && ABS(deltaY) <= CGFLOAT_EPSILON) {
    return;
  }
  [self sendScrollEdgeEvent:scrollView];

  CGFloat lowerThreshold = MAX(_lowerThreshold, kScrollEdgeThreshold);

  // Rule for ReachToBottom/Top for RTL:
  // RTL.ReachToBottom = LTR.ReachToTop & RTL.ReachToTop = LTR.ReachToBottom

  NSString *eventType = [_scrollEventManager eventBound:LynxEventScroll] ? LynxEventScroll : nil;
  if ([_scrollEventManager eventBound:LynxEventScrollToLower] &&
      [self shouldSendScrollToLowerEvent:lowerThreshold]) {
    if (!_hasReachBottom) {
      eventType = LynxEventScrollToLower;
      [_scrollEventManager sendScrollEvent:LynxEventScroll
                                scrollView:scrollView
                                    deltaX:deltaX
                                    deltaY:deltaY];
    }
    _hasReachBottom = YES;
  } else {
    _hasReachBottom = NO;
  }

  if ([_scrollEventManager eventBound:LynxEventScrollToUpper] &&
      [self shouldSendScrollToUpperEvent:_upperThreshold]) {
    if (!_hasReachTop) {
      eventType = LynxEventScrollToUpper;
      [_scrollEventManager sendScrollEvent:LynxEventScroll
                                scrollView:scrollView
                                    deltaX:deltaX
                                    deltaY:deltaY];
    }
    _hasReachTop = YES;
  } else {
    _hasReachTop = NO;
  }

  if (eventType) {
    [_scrollEventManager sendScrollEvent:eventType
                              scrollView:scrollView
                                  deltaX:deltaX
                                  deltaY:deltaY];
  }

  _preScrollTop = scrollTop;
  _preScrollLeft = scrollLeft;
}

- (void)sendScrollEdgeEvent:(UIScrollView *)scrollView {
  BOOL inNormalState = YES;
  if (scrollView.contentOffset.x < scrollView.contentInset.left ||
      scrollView.contentOffset.y < scrollView.contentInset.top) {
    return;
  }
  if (_enableScrollY) {
    if (scrollView.contentSize.height > scrollView.frame.size.height &&
        scrollView.contentOffset.y + scrollView.frame.size.height >
            scrollView.contentSize.height + scrollView.contentInset.bottom) {
      return;
    }
    if (scrollView.contentSize.height < scrollView.frame.size.height &&
        scrollView.contentOffset.y > 0) {
      return;
    }
  } else {
    if (scrollView.contentSize.width > scrollView.frame.size.width &&
        scrollView.contentOffset.x + scrollView.frame.size.width >
            scrollView.contentSize.width + scrollView.contentInset.right) {
      return;
    }
    if (scrollView.contentSize.width < scrollView.frame.size.width &&
        scrollView.contentOffset.x > 0) {
      return;
    }
  }
  if ([self shouldSendScrollToUpperEvent:kScrollEdgeThreshold]) {
    [_scrollEventManager sendScrollEvent:LynxEventScrollToUpperEdge scrollView:scrollView];
    inNormalState = NO;
  }
  if ([self shouldSendScrollToLowerEvent:kScrollEdgeThreshold]) {
    [_scrollEventManager sendScrollEvent:LynxEventScrollToLowerEdge scrollView:scrollView];
    inNormalState = NO;
  }
  if (inNormalState) {
    [_scrollEventManager sendScrollEvent:LynxEventScrollToNormalState scrollView:scrollView];
  }
}

#pragma mark - UIScrollViewDelegate

- (void)scrollViewDidScroll:(UIScrollView *)scrollView {
  if (scrollView == self.view &&
      ![scrollView respondToScrollViewDidScroll:((LynxScrollView *)scrollView).gestureConsumer]) {
    return;
  }
  // Notify scroll-view did scroll.
  [self.context.observer notifyScroll:nil];
  [self triggerSubviewsImpression];
  if ([self.view respondsToSelector:@selector(triggerNestedScrollView:)]) {
    if (self.view.enableNested && !_nestedUpdated) {
      _nestedUpdated = YES;
      if (!self.view.parentScrollView &&
          (!scrollView.childrenScrollView || scrollView.childrenScrollView.count == 0)) {
        [self.view updateChildren];
      }
    }
  }
  [scrollView triggerNestedScrollView:_enableScrollY];
  CGFloat scrollTop = scrollView.contentOffset.y;
  CGFloat scrollLeft = scrollView.contentOffset.x;
  if (_enableSticky) {
    [self onScrollSticky:scrollLeft withOffsetY:scrollTop];
  }

  [self sendScrollEvent:scrollView];
  [self updateLayerMaskOnFrameChanged];

  if (self.context != nil) {
    if (self.view.isDragging || self.view.isDecelerating) {
      [self.context onGestureRecognizedByUI:self];
    }
    [self postFluencyEventWithInfo:[self infoWithScrollView:scrollView
                                                   selector:@selector(scrollerDidScroll:)]];
  }

  for (id<LynxUIScrollerDelegate> delegate in _scrollerDelegates) {
    if ([delegate respondsToSelector:@selector(scrollerDidScroll:)]) {
      LYNX_TRACE_SECTION(LYNX_TRACE_CATEGORY_WRAPPER, @"LynxUIScrollerDelegate::scrollerDidScroll");
      [delegate scrollerDidScroll:scrollView];
      LYNX_TRACE_END_SECTION(LYNX_TRACE_CATEGORY_WRAPPER);
    }
  }
  if (_upperBounceUI || _lowerBounceUI) {
    [self triggerBounceWhileScroll:scrollView];
  }
}

- (void)scrollViewDidEndDecelerating:(UIScrollView *)scrollView {
  [_scrollEventManager sendScrollEvent:LynxEventScrollEnd scrollView:scrollView];
  [self postFluencyEventWithInfo:[self infoWithScrollView:scrollView
                                                 selector:@selector(scrollerDidEndDecelerating:)]];
  for (id<LynxUIScrollerDelegate> delegate in _scrollerDelegates) {
    if ([delegate respondsToSelector:@selector(scrollerDidEndDecelerating:)]) {
      LYNX_TRACE_SECTION(LYNX_TRACE_CATEGORY_WRAPPER,
                         @"LynxUIScrollerDelegate::scrollerDidEndDecelerating");
      [delegate scrollerDidEndDecelerating:scrollView];
      LYNX_TRACE_END_SECTION(LYNX_TRACE_CATEGORY_WRAPPER);
    }
  }
}

- (void)scrollViewWillEndDragging:(UIScrollView *)scrollView
                     withVelocity:(CGPoint)velocity
              targetContentOffset:(inout CGPoint *)targetContentOffset {
  if ([self.view stopDeceleratingIfNecessaryWithTargetContentOffset:targetContentOffset]) {
    return;
  }
}

- (void)scrollViewDidEndDragging:(UIScrollView *)scrollView willDecelerate:(BOOL)decelerate {
  [self sendScrollEvent:scrollView];
  if (!decelerate) {
    [_scrollEventManager sendScrollEvent:LynxEventScrollEnd scrollView:scrollView];
  }

  LynxScrollInfo *info = [self infoWithScrollView:scrollView
                                         selector:@selector(scrollerDidEndDragging:
                                                                    willDecelerate:)];
  info.decelerate = decelerate;
  [self postFluencyEventWithInfo:info];

  for (id<LynxUIScrollerDelegate> delegate in _scrollerDelegates) {
    if ([delegate respondsToSelector:@selector(scrollerDidEndDragging:willDecelerate:)]) {
      LYNX_TRACE_SECTION(LYNX_TRACE_CATEGORY_WRAPPER,
                         @"LynxUIScrollerDelegate::scrollerDidEndDragging");
      [delegate scrollerDidEndDragging:scrollView willDecelerate:decelerate];
      LYNX_TRACE_END_SECTION(LYNX_TRACE_CATEGORY_WRAPPER);
    }
  }
  _isTransferring = NO;
}

- (void)scrollViewWillBeginDragging:(UIScrollView *)scrollView {
  [self postFluencyEventWithInfo:[self infoWithScrollView:scrollView
                                                 selector:@selector(scrollerWillBeginDragging:)]];

  for (id<LynxUIScrollerDelegate> delegate in _scrollerDelegates) {
    if ([delegate respondsToSelector:@selector(scrollerWillBeginDragging:)]) {
      LYNX_TRACE_SECTION(LYNX_TRACE_CATEGORY_WRAPPER,
                         @"LynxUIScrollerDelegate::scrollerWillBeginDragging");
      [delegate scrollerWillBeginDragging:scrollView];
      LYNX_TRACE_END_SECTION(LYNX_TRACE_CATEGORY_WRAPPER);
    }
  }
}

- (void)scrollViewDidEndScrollingAnimation:(UIScrollView *)scrollView {
  [_scrollEventManager sendScrollEvent:LynxEventScrollEnd scrollView:scrollView];

  [self postFluencyEventWithInfo:[self infoWithScrollView:scrollView
                                                 selector:@selector
                                                 (scrollerDidEndScrollingAnimation:)]];
  for (id<LynxUIScrollerDelegate> delegate in _scrollerDelegates) {
    if ([delegate respondsToSelector:@selector(scrollerDidEndScrollingAnimation:)]) {
      LYNX_TRACE_SECTION(LYNX_TRACE_CATEGORY_WRAPPER,
                         @"LynxUIScrollerDelegate::scrollerDidEndScrollingAnimation");
      [delegate scrollerDidEndScrollingAnimation:scrollView];
      LYNX_TRACE_END_SECTION(LYNX_TRACE_CATEGORY_WRAPPER)
    }
  }
  if (_scrollToCallBack) {
    _scrollToCallBack(kScrollToCallBackNonChange, nil);
    _scrollToCallBack = nil;
  }
}

- (void)scrollInto:(LynxUI *)value
          isSmooth:(BOOL)isSmooth
         blockType:(NSString *)blockType
        inlineType:(NSString *)inlineType {
  CGFloat scrollDistance = 0;
  if (_enableScrollY) {
    if ([@"nearest" isEqualToString:blockType]) {
      return;
    }
    if ([@"center" isEqualToString:blockType]) {
      scrollDistance -= (self.view.frame.size.height - value.view.frame.size.height) / 2;
    } else if ([@"end" isEqualToString:blockType]) {
      scrollDistance -= (self.view.frame.size.height - value.view.frame.size.height);
    }
    while (value != self) {
      scrollDistance += value.view.frame.origin.y;
      value = value.parent;
    }
    scrollDistance =
        MAX(0, MIN(self.view.contentSize.height - self.view.frame.size.height, scrollDistance));
    [self.view setContentOffset:CGPointMake(0, scrollDistance) animated:isSmooth];
  } else {
    if ([@"nearest" isEqualToString:inlineType]) {
      return;
    }
    if ([@"center" isEqualToString:inlineType]) {
      scrollDistance -= (self.view.frame.size.width - value.view.frame.size.width) / 2;
    } else if ([@"end" isEqualToString:inlineType]) {
      scrollDistance -= (self.view.frame.size.width - value.view.frame.size.width);
    }
    while (value != self) {
      scrollDistance += value.view.frame.origin.x;
      value = value.parent;
    }
    scrollDistance =
        MAX(0, MIN(self.view.contentSize.width - self.view.frame.size.width, scrollDistance));
    [self.view setContentOffset:CGPointMake(scrollDistance, 0) animated:isSmooth];
  }
}

- (void)contentSizeDidChanged {
  NSDictionary *detail = @{
    @"scrollWidth" : @(self.view.contentSize.width),
    @"scrollHeight" : @(self.view.contentSize.height)
  };
  [_scrollEventManager sendScrollEvent:LynxEventContentSizeChange
                            scrollView:self.view
                                detail:detail];
  [self sendScrollEdgeEvent:self.view];
}

- (CGPoint)clampScrollToPosition:(CGFloat)position
                        callback:(LynxUIMethodCallbackBlock)callback
                        isSmooth:(BOOL)isSmooth {
  CGFloat lowerThreshold = 0.;
  CGFloat upperThreshold = CGFLOAT_MAX;
  NSMutableString *callBackInfo = [NSMutableString string];

  if (_enableScrollY) {
    lowerThreshold =
        [self view] == nil ? 0 : self.view.contentSize.height - self.view.bounds.size.height;
    upperThreshold = 0;
  } else {
    lowerThreshold =
        [self view] == nil ? 0 : self.view.contentSize.width - self.view.bounds.size.width;
    upperThreshold = 0;
  }

  // If the final position is over edge, appends extra info.
  if (position > lowerThreshold || position < upperThreshold) {
    CGFloat prevPosition = position;
    position = MIN(lowerThreshold, position);
    position = MAX(upperThreshold, position);
    [callBackInfo
        appendFormat:@"Target scroll position %f is beyond threshold. Clamped to position %f.",
                     prevPosition, position];
  }

  CGPoint targetOffset = _enableScrollY ? CGPointMake(0., position) : CGPointMake(position, 0.);

  // postpone the callback to animation ends.
  if (nil != callback) {
    if (callBackInfo.length > 0) {
      _scrollToCallBack = ^(int code, id _Nullable data) {
        if (code == kScrollToCallBackNonChange && nil == data) {
          callback(
              kUIMethodSuccess,
              [NSString stringWithFormat:@"scrollTo succeed with Warning : %@ Final position: %f",
                                         callBackInfo, position]);
        } else {
          callback(code, data);
        }
      };
    } else {
      _scrollToCallBack = ^(int code, id _Nullable data) {
        if (code == kScrollToCallBackNonChange && nil == data) {
          callback(kUIMethodSuccess,
                   [NSString stringWithFormat:@"scrollTo succeed. Final position: %f", position]);
        } else {
          callback(code, data);
        }
      };
    }
  }

  return targetOffset;
}

- (void)sendNonAnimatedScrollEndEvent:(UIScrollView *)scrollView
                  targetContentOffset:(CGPoint)targetContentOffset {
  [_scrollEventManager sendScrollEvent:LynxEventScrollEnd
                            scrollView:scrollView
                   targetContentOffset:targetContentOffset];
}

LYNX_PROPS_GROUP_DECLARE(LYNX_PROP_DECLARE("ios-block-gesture-class", setIosBlockGestureClass,
                                           NSString *),
                         LYNX_PROP_DECLARE("force-can-scroll", setForceCanScroll, BOOL),
                         LYNX_PROP_DECLARE("ios-recognized-view-tag", setIosRecognizedViewTag,
                                           BOOL))

/**
 * @name: force-can-scroll
 * @description: On iOS, force-can-scroll should be used with ios-block-gesture-class,
 *ios-recognized-view-tag. Can be used alone on Android. scroll-view will consume gesture even when
 *it reaches the bounds, and block all nested scrollable containers, such as pageView component from
 *native、sliding left to return, etc. On iOS, it should also use ios-block-gesture-class to specify
 *the scrollable's className, along with ios-recognized-view-tag to specify the container's tag.
 * @category: different
 * @standardAction: keep
 * @supportVersion: 2.11
 **/
LYNX_PROP_DEFINE("force-can-scroll", setForceCanScroll, BOOL) {
  if (requestReset) {
    value = NO;
  }
  ((LynxScrollView *)self.view).forceCanScroll = value;
}

/**
 * @name: ios-block-gesture-class
 * @description: iOS only. force-can-scroll should be used with
 *ios-block-gesture-class、ios-recognized-view-tag. Specify the class name of scrollable container
 *that should be blocked by force-can-scroll. Given by container's developer.
 * @category: different
 * @standardAction: keep
 * @supportVersion: 2.11
 **/
LYNX_PROP_DEFINE("ios-block-gesture-class", setIosBlockGestureClass, NSString *) {
  if (requestReset) {
    value = [NSString string];
  }
  ((LynxScrollView *)self.view).blockGestureClass = NSClassFromString(value);
}

/**
 * @name: ios-recognized-view-tag
 * @description: iOS only. force-can-scroll should be used with
 *ios-block-gesture-class、ios-recognized-view-tag. Specify scrollable container's tag, the UIView's
 *tag. Set and given by container's developer. to fail its gesture
 * @category: different
 * @standardAction: keep
 * @supportVersion: 2.11
 **/
LYNX_PROP_DEFINE("ios-recognized-view-tag", setIosRecognizedViewTag, NSInteger) {
  if (requestReset) {
    value = 0;
  }
  ((LynxScrollView *)self.view).recognizedViewTag = value;
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

LYNX_UI_METHOD(scrollTo) {
  if (nil != _scrollToCallBack) {
    _scrollToCallBack(
        kUIMethodSuccess,
        @"successDue to the start of a new scrollTo operation, the previous scrollTo has stopped.");
    _scrollToCallBack = nil;
  }
  if (self.children.count == 0) {
    callback(kUIMethodParamInvalid, @"Invoke scrollTo failed due to empty children");
    return;
  }
  NSInteger index = -1;
  CGFloat offset = [LynxUnitUtils toPtFromIDUnitValue:[params objectForKey:@"offset"]
                                        withDefaultPt:0.];
  if ([params objectForKey:@"index"]) {
    index = ((NSNumber *)[params objectForKey:@"index"]).integerValue;
    if (index < 0 || index >= (NSInteger)self.children.count) {
      callback(kUIMethodParamInvalid,
               [NSString stringWithFormat:@"scrollTo index: %ld is out of range[0, %ld].", index,
                                          self.children.count]);
      return;
    }
  }
  BOOL animated = [[params objectForKey:@"smooth"] boolValue];
  UIScrollView *scrollView = [self view];

  if (index >= 0 && (NSUInteger)index < [self.children count]) {
    LynxUI *target = [self.children objectAtIndex:index];
    if (_enableScrollY) {
      offset += target.view.frame.origin.y;
    } else {
      if (self.isRtl) {
        offset = target.view.frame.origin.x + target.view.frame.size.width -
                 self.view.frame.size.width - offset;
      } else {
        offset += target.view.frame.origin.x;
      }
    }
  }
  CGPoint targetOffset = [self clampScrollToPosition:offset callback:callback isSmooth:animated];

  [self.view setContentOffset:targetOffset animated:animated];

  // If animated, triggered after the animation ends.
  // If not animated or targetOffset equals current offset, send callback immediately.
  if (!animated || CGPointEqualToPoint(self.view.contentOffset, targetOffset)) {
    if (nil != _scrollToCallBack) {
      _scrollToCallBack(kScrollToCallBackNonChange, nil);
      _scrollToCallBack = nil;
    }
  }

  if (!animated && !CGPointEqualToPoint(self.view.contentOffset, targetOffset)) {
    [self sendNonAnimatedScrollEndEvent:scrollView targetContentOffset:targetOffset];
  }
}

/**
 * get scroll-view's scroll info
 * @param callback
 * @return scrollX / scrollY - content offset, scrollRange
 */
LYNX_UI_METHOD(getScrollInfo) {
  if (callback) {
    callback(
        kUIMethodSuccess, @{
          @"scrollX" : @(self.view.contentOffset.x),
          @"scrollY" : @(self.view.contentOffset.y),
          @"scrollRange" : _enableScrollY ? @(self.view.contentSize.height)
                                          : @(self.view.contentSize.width)
        });
  }
}

LYNX_UI_METHOD(autoScroll) {
  if ([[params objectForKey:@"start"] boolValue]) {
    [self startAutoScrollWithRate:[[params objectForKey:@"rate"] doubleValue] / 60];
  } else {
    [self stopAutoScroll];
  }
}

- (void)startAutoScrollWithRate:(CGFloat)rate {
  // when there is no 'rate' key in the 'params' dictionary, the value is zero.
  if (rate == 0) {
    return;
  }
  LynxUIScrollerProxy *proxy = [[LynxUIScrollerProxy alloc] initWithScroller:self
                                                                        rate:rate
                                                               enableScrollY:_enableScrollY];

  if (_displayLink) {
    [self stopAutoScroll];
  }
  _displayLink = [CADisplayLink displayLinkWithTarget:proxy selector:@selector(displayLinkAction)];
  _displayLink.paused = NO;
  [_displayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSRunLoopCommonModes];
}

- (void)stopAutoScroll {
  if (_displayLink) {
    _displayLink.paused = YES;
    [_displayLink invalidate];
    _displayLink = nil;
  }
}

- (void)dealloc {
  if (_displayLink) {
    _displayLink.paused = YES;
    [_displayLink invalidate];
    _displayLink = nil;
  }
}

// lynx don't detect horizontal and vertical screen here.
- (void)frameDidChange {
  CGPoint contentOffset = [self.view contentOffset];
  [super frameDidChange];
  [self.view setContentOffset:[self clampScrollToPosition:self.enableScrollY ? contentOffset.y
                                                                             : contentOffset.x
                                                 callback:nil
                                                 isSmooth:NO]];
}

- (float)scrollLeftLimit {
  return [self view] == nil ? 0 : -self.view.contentInset.left;
}

- (float)scrollRightLimit {
  return [self view] == nil ? 0
                            : self.view.contentSize.width - self.view.bounds.size.width +
                                  self.view.contentInset.right;
}

- (float)scrollUpLimit {
  return [self view] == nil ? 0 : -self.view.contentInset.top;
}

- (float)scrollDownLimit {
  return [self view] == nil ? 0
                            : self.view.contentSize.height - self.view.bounds.size.height +
                                  self.view.contentInset.bottom;
}

- (BOOL)canScroll:(ScrollDirection)direction {
  if ([self view] == nil) return NO;
  switch (direction) {
    case SCROLL_LEFT:
      return [self contentOffset].x > [self scrollLeftLimit];
      break;
    case SCROLL_RIGHT:
      return [self contentOffset].x < [self scrollRightLimit];
      break;
    case SCROLL_UP:
      return [self contentOffset].y > [self scrollUpLimit];
      break;
    case SCROLL_DOWN:
      return [self contentOffset].y < [self scrollDownLimit];
      break;
    default:
      break;
  }
  return NO;
};

// private method, for scrollByX and scrollByY
- (void)scroll:(float)delta direction:(ScrollDirection)direction {
  if ([self view] == nil) return;
  CGPoint offset = [self contentOffset];
  if (direction == SCROLL_LEFT || direction == SCROLL_RIGHT) {
    offset.x += delta;
    offset.x = MAX(offset.x, [self scrollLeftLimit]);
    offset.x = MIN(offset.x, [self scrollRightLimit]);
  } else {
    offset.y += delta;
    offset.y = MAX(offset.y, [self scrollUpLimit]);
    offset.y = MIN(offset.y, [self scrollDownLimit]);
  }
  [[self view] setContentOffset:offset];
}

- (void)scrollByX:(float)delta {
  [self scroll:delta direction:SCROLL_LEFT];
}

- (void)scrollByY:(float)delta {
  [self scroll:delta direction:SCROLL_UP];
}

// private method, for flickX and flickY
- (void)flick:(float)velocity direction:(ScrollDirection)direction {
  if (self.view == nil || fabs(velocity) < 1e-3) return;
  bool isHorizontal = direction == SCROLL_LEFT || direction == SCROLL_RIGHT;
  CGPoint offset = [self contentOffset];
  float oppositeBoundary =
      isHorizontal ? [self scrollLeftLimit] - offset.x : [self scrollUpLimit] - offset.y;
  float positiveBoundary =
      isHorizontal ? [self scrollRightLimit] - offset.x : [self scrollDownLimit] - offset.y;
  __strong FlickParameter *flicker =
      [[FlickParameter alloc] initFlick:velocity
                       decelerationRate:self.view.decelerationRate
                              threshold:0.5 / [[UIScreen mainScreen] scale]
                       oppositeBoundary:oppositeBoundary
                       positiveBoundary:positiveBoundary];
  if (isHorizontal) {
    offset.x += [flicker delta];
  } else {
    offset.y += [flicker delta];
  }
  [flicker setContentOffset:self.view destination:offset];
}

- (void)flickX:(float)velocity {
  [self flick:velocity direction:SCROLL_LEFT];
}

- (void)flickY:(float)velocity {
  [self flick:velocity direction:SCROLL_UP];
}

#pragma mark - impression

- (void)lynxImpressionWillManualExposureNotification:(NSNotification *)noti {
  if (![self.context.rootView isKindOfClass:LynxView.class]) {
    return;
  }

  if ([noti.userInfo[LynxImpressionStatusNotificationKey] isEqualToString:@"show"]) {
    _forceImpression = [noti.userInfo[LynxImpressionForceImpressionBoolKey] boolValue];
    [self triggerSubviewsImpression];
  } else if ([noti.userInfo[LynxImpressionStatusNotificationKey] isEqualToString:@"hide"]) {
    [self triggerSubviewsExit];
  }
}

- (void)triggerSubviewsExit {
  _lastScrollPoint = CGPointMake(INFINITY, INFINITY);
  [self.view.subviews enumerateObjectsUsingBlock:^(__kindof LynxInnerImpressionView *_Nonnull obj,
                                                   NSUInteger idx, BOOL *_Nonnull stop) {
    if (![obj isKindOfClass:LynxInnerImpressionView.class]) {
      return;
    }

    [obj exit];
  }];
}

- (void)triggerSubviewsImpression {
  // When _forceImpression is True, check if the rootView is on the screen.
  // When _forceImpression is False, check if the self (aka. the current scrollView) is on the
  // screen.

  CGRect objRect = CGRectZero;
  if (_forceImpression) {
    objRect = [self.context.rootView convertRect:self.context.rootView.bounds toView:nil];
  } else {
    objRect = [self.view convertRect:self.view.bounds toView:nil];
  }

  CGRect intersectionRect = CGRectIntersection(self.view.window.bounds, objRect);

  if ((intersectionRect.size.height * intersectionRect.size.width == 0 || self.view.hidden) &&
      !_forceImpression) {
    return;
  }

  CGPoint contentOffset = self.view.contentOffset;

  if (fabs(_lastScrollPoint.x - contentOffset.x) > _sensitivity ||
      fabs(_lastScrollPoint.y - contentOffset.y) > _sensitivity) {
    _lastScrollPoint = self.view.contentOffset;

    // Perform recursive checks to prevent issues when multiple x-scroll-views are nested.
    // Ensures that x-impression-view can still impression when an outer scroll-view is scrolling.
    [self.children
        enumerateObjectsUsingBlock:^(LynxUI *_Nonnull obj, NSUInteger idx, BOOL *_Nonnull stop) {
          if ([obj isKindOfClass:LynxUIScroller.class]) {
            [(LynxUIScroller *)obj triggerSubviewsImpression];
          }
        }];

    [self.view.subviews enumerateObjectsUsingBlock:^(__kindof LynxInnerImpressionView *_Nonnull obj,
                                                     NSUInteger idx, BOOL *_Nonnull stop) {
      if (![obj isKindOfClass:LynxInnerImpressionView.class]) {
        return;
      }

      CGRect objRect = [self.view convertRect:obj.frame fromView:self.view];
      CGRect intersectionRect = CGRectIntersection(self.view.bounds, objRect);

      CGFloat intersectionArea = intersectionRect.size.height * intersectionRect.size.width;
      if (intersectionArea == 0) {
        [obj exit];
      } else {
        CGFloat impressionArea =
            CGRectGetHeight(obj.bounds) * CGRectGetWidth(obj.bounds) * obj.impressionPercent;
        if (intersectionArea >= impressionArea) {
          [obj impression];
        } else {
          [obj exit];
        }
      }
    }];
  }
}

#pragma mark bounceView

- (void)autoAddBounceView {
  if (!_defaultBounceUI && !_lowerBounceUI && !_upperBounceUI) {
    return;
  }
  if (!_lowerBounceUI && _defaultBounceUI) {
    _lowerBounceUI = _defaultBounceUI;
  }
  if (_lowerBounceUI) {
    [self adjustBounceViewFrame:_lowerBounceUI];
  }

  if (_upperBounceUI) {
    [self adjustBounceViewFrame:_upperBounceUI];
  }

  if (self.isRtl) {
    if (_upperBounceUI || _lowerBounceUI) {
      // In RTL, bounce-view will be the first cell of the scroll-view.
      // We need to update each ChildrenFrame.
      // In short, child.frame.origin.x -= width of bounceView.frame.
      [self adjustChildrenFrameToMaskBounceViewInRtl];
    }
    // In RTL, we need bounce-view info to calculate the content size of the scroll-view.
    // Hence [super layoutDidFinished] should be called after bounce-view logic.
    [super layoutDidFinished];
  }
}

- (void)adjustBounceViewFrame:(LynxBounceView *)bounceUI {
  CGFloat bounceHeight = CGRectGetHeight(bounceUI.view.bounds) ?: CGRectGetHeight(self.view.bounds);
  CGFloat bounceWidth = CGRectGetWidth(bounceUI.view.bounds) ?: CGRectGetWidth(self.view.bounds);

  switch (bounceUI.direction) {
    case LynxBounceViewDirectionTop:
      bounceUI.view.frame = (CGRect){{self.padding.left, -(bounceHeight + _upperBounceUI.space)},
                                     {bounceWidth, bounceHeight}};
      break;
    case LynxBounceViewDirectionLeft:
      if (!self.isRtl) {
        bounceUI.view.frame = (CGRect){{-(bounceWidth + _upperBounceUI.space), self.padding.top},
                                       {bounceWidth, bounceHeight}};
      } else {
        bounceUI.view.frame =
            (CGRect){{MAX([self view].contentSize.width, CGRectGetWidth([self view].bounds)) +
                          bounceUI.space,
                      self.padding.top},
                     {bounceWidth, bounceHeight}};
        bounceUI.direction = LynxBounceViewDirectionRight;
      }
      break;
    case LynxBounceViewDirectionRight:
      if (self.isRtl) {
        bounceUI.view.frame = (CGRect){{-(bounceWidth + _lowerBounceUI.space), self.padding.top},
                                       {bounceWidth, bounceHeight}};
        bounceUI.direction = LynxBounceViewDirectionLeft;
      } else {
        bounceUI.view.frame =
            (CGRect){{MAX([self view].contentSize.width, CGRectGetWidth([self view].bounds)) +
                          bounceUI.space,
                      self.padding.top},
                     {bounceWidth, bounceHeight}};
      }
      break;
    case LynxBounceViewDirectionBottom:
      bounceUI.view.frame = (CGRect){{self.padding.left, MAX([self view].contentSize.height,
                                                             CGRectGetHeight([self view].bounds)) +
                                                             bounceUI.space},
                                     {bounceWidth, bounceHeight}};
      break;
    default:
      break;
  }
  [self.view addSubview:bounceUI.view];
}

- (void)scrollToBounces:(CGFloat)bounceDistance
            inDirection:(NSString *)direction
           withDistance:(CGFloat)eventDistance {
  if (!_lowerBounceUI && !_upperBounceUI) {
    return;
  }

  NSDictionary *detail = @{
    @"direction" : direction ?: @"",
    @"triggerDistance" : [NSNumber numberWithFloat:eventDistance],
    @"bounceDistance" : [NSNumber numberWithFloat:bounceDistance]
  };
  [_scrollEventManager sendScrollEvent:LynxEventScrollToBounce scrollView:self.view detail:detail];
}

- (void)triggerBounceWhileScroll:(UIScrollView *_Nonnull)scrollView {
  // If the scrollView cannot be scrolled, bouncing should also not be triggered.
  if ((_enableScrollY && scrollView.contentSize.height < scrollView.frame.size.height) ||
      (!_enableScrollY && scrollView.contentSize.width < scrollView.frame.size.width)) {
    return;
  }
  __block CGFloat upperBounceDistance = kInvalidBounceDistance;
  __block CGFloat lowerBounceDistance = kInvalidBounceDistance;
  if (_upperBounceUI) {
    if (_upperBounceUI.direction == LynxBounceViewDirectionTop ||
        _upperBounceUI.direction == LynxBounceViewDirectionLeft) {
      upperBounceDistance = _upperBounceUI.triggerBounceEventDistance;
    } else {
      lowerBounceDistance = _upperBounceUI.triggerBounceEventDistance;
    }
  }
  if (_lowerBounceUI) {
    if (_lowerBounceUI.direction == LynxBounceViewDirectionRight ||
        _lowerBounceUI.direction == LynxBounceViewDirectionBottom) {
      lowerBounceDistance = _lowerBounceUI.triggerBounceEventDistance;
    } else {
      upperBounceDistance = _lowerBounceUI.triggerBounceEventDistance;
    }
  }
  // If the contentOffset hit maxW/maxY, it means this scrollView hit the content border and start
  // to scroll bounce-view.
  if (!_enableScrollY) {
    CGFloat maxX = scrollView.contentSize.width - CGRectGetWidth(scrollView.frame);
    if (maxX < 0) {
      maxX = 0;
    }
    // isTransferring and isDragging are used to avoid multiple callbacks when users slide back and
    // forth. isTransferring has to be set to false in endDragging.
    if (_isTransferring || self.view.isDragging) {
      return;
    }
    if (lowerBounceDistance != kInvalidBounceDistance) {
      if (scrollView.contentOffset.x > maxX + lowerBounceDistance) {
        _isTransferring = YES;
        [self scrollToBounces:scrollView.contentOffset.x - maxX
                  inDirection:@"right"
                 withDistance:lowerBounceDistance];
      }
    }
    if (upperBounceDistance != kInvalidBounceDistance) {
      if (scrollView.contentOffset.x < -upperBounceDistance) {
        _isTransferring = YES;
        [self scrollToBounces:scrollView.contentOffset.x
                  inDirection:@"left"
                 withDistance:upperBounceDistance];
      }
    }
  } else {
    if (_isTransferring || self.view.isDragging) {
      return;
    }
    CGFloat maxY = scrollView.contentSize.height - CGRectGetHeight(scrollView.frame);
    if (maxY < 0) {
      maxY = 0;
    }
    if (lowerBounceDistance != kInvalidBounceDistance) {
      if (scrollView.contentOffset.y > maxY + lowerBounceDistance) {
        _isTransferring = YES;
        [self scrollToBounces:scrollView.contentOffset.y - maxY
                  inDirection:@"bottom"
                 withDistance:lowerBounceDistance];
      }
    }
    if (upperBounceDistance != kInvalidBounceDistance) {
      if (scrollView.contentOffset.y < -upperBounceDistance) {
        _isTransferring = YES;
        [self scrollToBounces:scrollView.contentOffset.y
                  inDirection:@"top"
                 withDistance:upperBounceDistance];
      }
    }
  }
}

- (void)adjustChildrenFrameToMaskBounceViewInRtl {
  if (_enableScrollY) {
    float minHeight = INFINITY;
    for (LynxUI *child in self.children) {
      minHeight = MIN(minHeight, child.frame.origin.y);
    }
    // this if is true when the children upper space was occupied by bounceView
    // adjust heights to make sure views will cover bounceView
    if (_enableScrollY && _upperBounceUI != nil && minHeight == _upperBounceUI.frame.size.height) {
      for (LynxUI *child in self.children) {
        [child updateFrame:(CGRect) {
          {child.frame.origin.x, child.frame.origin.y - minHeight}, {
            child.frame.size.width, child.frame.size.height
          }
        }
                    withPadding:child.padding
                         border:child.border
                         margin:child.margin
            withLayoutAnimation:NO];
      }
    }
  } else {
    float minWidth = INFINITY;
    for (LynxUI *child in self.children) {
      minWidth = MIN(minWidth, child.frame.origin.x);
    }
    // the same as height, but left space was occupied, so shift left.
    if (!_enableScrollY && _upperBounceUI != nil && minWidth == _upperBounceUI.frame.size.width) {
      for (LynxUI *child in self.children) {
        [child updateFrame:(CGRect) {
          {child.frame.origin.x - minWidth, child.frame.origin.y}, {
            child.frame.size.width, child.frame.size.height
          }
        }
                    withPadding:child.padding
                         border:child.border
                         margin:child.margin
            withLayoutAnimation:NO];
      }
    }
  }
}

#pragma mark list native storage

- (LynxUI *__nullable)getParentList {
  return [self.context.uiOwner findUIBySign:_listSign];
}

- (NSString *)generateCacheKey:(NSString *)itemKey {
  return [NSString stringWithFormat:@"%@_scrollview_%@", itemKey, self.idSelector];
}

- (BOOL)initialPropsFlushed:(NSString *)initialPropKey {
  LynxUI *storageList = [self getParentList];
  if (!storageList) {
    // Not in list or never recycled before. Return YES and only use self.firstRender to judge.
    return YES;
  }
  return [storageList initialPropsFlushed:initialPropKey
                                 cacheKey:[self generateCacheKey:self.currentItemKey]];
}

- (void)setInitialPropsHasFlushed:(NSString *)initialPropKey {
  LynxUI *storageList = [self getParentList];
  if (!storageList) {
    return;
  }
  [storageList setInitialPropsHasFlushed:initialPropKey
                                cacheKey:[self generateCacheKey:self.currentItemKey]];
}

- (void)onListCellAppear:(NSString *)itemKey withList:(LynxUI *)list {
  [super onListCellAppear:itemKey withList:list];
  // restore contentOffset
  if (itemKey) {
    _listSign = list.sign;
    _currentItemKey = itemKey;
    NSString *cacheKey = [self generateCacheKey:itemKey];
    NSMutableDictionary *nativeStorage = [self getNativeStorageFromList:list];
    if (nativeStorage && nativeStorage[cacheKey]) {
      // no first render and last time this UI shows it has a changed offset
      CGPoint offset = [nativeStorage[cacheKey] CGPointValue];
      [self.nodeReadyBlockArray addObject:^(LynxUI *ui) {
        LynxUIScroller *scroll = (LynxUIScroller *)ui;
        [scroll initialScrollOffsetInner:scroll.enableScrollY ? offset.y : offset.x];
      }];
    } else {
      // first render
      if ([_propMap getValueForKey:LynxScrollViewInitialScrollIndex] &&
          ![self initialPropsFlushed:LynxScrollViewInitialScrollIndex]) {
        // has initial-scroll-index and not flushed in onNodeReadyBlockArray
        // If there is no diff during reuse, the onNodeReady will no trigger
        [self setInitialPropsHasFlushed:LynxScrollViewInitialScrollIndex];
        [self initialScrollToIndexInner:[[_propMap getValueForKey:LynxScrollViewInitialScrollIndex]
                                            intValue]];
      }
      if ([_propMap getValueForKey:LynxScrollViewInitialScrollOffset] &&
          ![self initialPropsFlushed:LynxScrollViewInitialScrollOffset]) {
        // has initial-scroll-offset and not flushed in onNodeReadyBlockArray
        // If there is no diff during reuse, the onNodeReady will no trigger
        [self setInitialPropsHasFlushed:LynxScrollViewInitialScrollOffset];
        [self initialScrollOffsetInner:[[_propMap getValueForKey:LynxScrollViewInitialScrollOffset]
                                           intValue]];
      }
    }
  }
}

- (void)onListCellPrepareForReuse:(NSString *)itemKey withList:(LynxUI *)list {
  // pitfall: in this timing, the self.idSelector has not been updated to reuser
  if (itemKey) {
    _listSign = list.sign;
    _currentItemKey = itemKey;
    [self resetContentOffset];
  }
}

- (void)onListCellDisappear:(NSString *)itemKey exist:(BOOL)isExist withList:(LynxUI *)list {
  [super onListCellDisappear:itemKey exist:isExist withList:list];
  // store current contentOffset
  if (itemKey) {
    _listSign = list.sign;
    _currentItemKey = itemKey;
    NSString *cacheKey = [self generateCacheKey:itemKey];
    if (isExist) {
      [self storeKeyToNativeStorage:list key:cacheKey value:@(self.view.contentOffset)];
    } else {
      [self removeKeyFromNativeStorage:list key:cacheKey];
    }
  }
}

#pragma mark - LynxNewGesture

- (CGFloat)getMemberScrollX {
  return self.view.contentOffset.x;
}

- (CGFloat)getMemberScrollY {
  return self.view.contentOffset.y;
}

- (NSArray<NSNumber *> *)scrollBy:(CGFloat)deltaX deltaY:(CGFloat)deltaY {
  [self onGestureScrollBy:CGPointMake(deltaX, deltaY)];
  return @[ @(self.view.contentOffset.x), @(self.view.contentOffset.y) ];
}

- (void)gestureDidSet {
  if (!self.context.enableNewGesture) {
    return;
  }
  [super gestureDidSet];
  ((LynxScrollView *)self.view).gestureEnabled = YES;
  [self enableIncreaseFrequencyIfNecessary];
  [self ensureGestureConsumer];
}

- (void)ensureGestureConsumer {
  [self.gestureMap
      enumerateKeysAndObjectsUsingBlock:^(
          NSNumber *_Nonnull key, LynxGestureDetectorDarwin *_Nonnull obj, BOOL *_Nonnull stop) {
        if (obj.gestureType == LynxGestureTypeNative) {
          if (!self.view.gestureConsumer) {
            self.view.gestureConsumer = [[LynxGestureConsumer alloc] init];
          }
          self.view.scrollEnabled = YES;
        }
      }];
}

- (void)consumeInternalGesture:(BOOL)consume {
  [self.view.gestureConsumer consumeGesture:consume];
}

- (BOOL)canConsumeGesture:(CGPoint)delta {
  return [self.view consumeDeltaOffset:delta vertical:_enableScrollY];
}

- (BOOL)getGestureBorder:(BOOL)start {
  return ![self.view consumeDeltaOffset:start ? CGPointMake(-0.1, -0.1) : CGPointMake(0.1, 0.1)
                               vertical:_enableScrollY];
}

- (void)onGestureScrollBy:(CGPoint)delta {
  CGPoint point = self.view.contentOffset;
  if (_enableScrollY) {
    point.y += delta.y;
  } else {
    point.x += delta.x;
  }
  ((LynxScrollView *)self.view).duringGestureScroll = YES;
  [self.view updateContentOffset:point vertical:_enableScrollY];
  ((LynxScrollView *)self.view).duringGestureScroll = NO;

  // when scroll, not trigger basic events
  if (fabs(delta.x) > SCROLL_BY_EPSILON || fabs(delta.y) > SCROLL_BY_EPSILON) {
    [self.context onGestureRecognizedByUI:self];
  }
}

@end

@implementation LynxUIScrollerProxy

- (instancetype)initWithScroller:(LynxUIScroller *)scroller
                            rate:(CGFloat)rate
                   enableScrollY:(BOOL)enableScrollY {
  self = [super init];
  if (self) {
    self.scroller = scroller;
    self.rate = rate;
    self.enableScrollY = enableScrollY;
  }
  return self;
}

- (void)displayLinkAction {
  if (self.scroller) {
    if (self.enableScrollY) {
      if (self.scroller.view.bounds.size.height + self.scroller.view.contentOffset.y + self.rate >=
          self.scroller.view.contentSize.height) {
        self.rate = self.scroller.view.contentSize.height - self.scroller.view.contentOffset.y -
                    self.scroller.view.bounds.size.height;
        [self.scroller stopAutoScroll];
      }
      CGRect bounds = CGRectMake(
          self.scroller.view.contentOffset.x, self.scroller.view.contentOffset.y + self.rate,
          self.scroller.view.bounds.size.width, self.scroller.view.bounds.size.height);
      CGFloat prevY = self.scroller.view.bounds.origin.y;
      [self.scroller.view setBounds:bounds];
      if (self.scroller.view.bounds.origin.y != prevY) {
        [self.scroller scrollViewDidScroll:self.scroller.view];
      }
    } else {
      if (self.scroller.isRtl) {
        // Scroll to left and stop at the left edge.
        if (self.scroller.view.contentOffset.x - self.rate < 0) {
          self.rate = 0;
          [self.scroller stopAutoScroll];
        }
        CGRect bounds = CGRectMake(
            self.scroller.view.contentOffset.x - self.rate, self.scroller.view.contentOffset.y,
            self.scroller.view.bounds.size.width, self.scroller.view.bounds.size.height);
        CGFloat prevX = self.scroller.view.bounds.origin.x;
        [self.scroller.view setBounds:bounds];
        if (self.scroller.view.bounds.origin.x != prevX) {
          [self.scroller scrollViewDidScroll:self.scroller.view];
        }
      } else {
        // Scroll to right and stop at the right edge.
        if (self.scroller.view.bounds.size.width + self.scroller.view.contentOffset.x + self.rate >=
            self.scroller.view.contentSize.width) {
          self.rate = self.scroller.view.contentSize.width - self.scroller.view.contentOffset.x -
                      self.scroller.view.bounds.size.width;
          [self.scroller stopAutoScroll];
        }
        CGRect bounds = CGRectMake(
            self.scroller.view.contentOffset.x + self.rate, self.scroller.view.contentOffset.y,
            self.scroller.view.bounds.size.width, self.scroller.view.bounds.size.height);
        CGFloat prevX = self.scroller.view.bounds.origin.x;
        [self.scroller.view setBounds:bounds];
        if (self.scroller.view.bounds.origin.x != prevX) {
          [self.scroller scrollViewDidScroll:self.scroller.view];
        }
      }
    }
  }
}

@end
