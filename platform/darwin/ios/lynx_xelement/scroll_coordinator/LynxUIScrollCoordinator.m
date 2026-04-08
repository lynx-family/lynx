// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxBaseGestureHandler.h>
#import <Lynx/LynxGlobalObserver.h>
#import <Lynx/LynxPropsProcessor.h>
#import <Lynx/LynxUIMethodProcessor.h>
#import <Lynx/LynxUnitUtils.h>
#import <Lynx/UIScrollView+Lynx.h>
#import <Lynx/UIScrollView+LynxGesture.h>
#import <Lynx/UIView+Lynx.h>
#import <XElement/LynxUIScrollCoordinator.h>
#import <XElement/LynxUIScrollCoordinatorHeader.h>
#import <XElement/LynxUIScrollCoordinatorSlot.h>
#import <XElement/LynxUIScrollCoordinatorToolbar.h>
#import <XElement/LynxUIViewPager.h>
#import <XElement/UIScrollView+ScrollCoordinator.h>
#import <objc/runtime.h>

static const CGFloat kScrollByEpsilon = 0.1f;

static Class LynxScrollCoordinatorLookupClass(const char *name) { return objc_lookUpClass(name); }

@protocol LynxUIScrollCoordinatorInternalDelegate <NSObject>

- (void)scrollCoordinatorDidScroll:(UIScrollView *)scrollView;
- (BOOL)shouldIgnoreNestedScrollView:(UIScrollView *)scrollView;

@end

// todo pixel to 1dp
@interface LynxScrollCoordinatorView () <UIScrollViewDelegate>
@property(nonatomic, assign) CGFloat threshold;
@property(nonatomic, weak) id<LynxUIScrollCoordinatorInternalDelegate> uiDelegate;
@property(nonatomic, weak) LynxUI *potentialRootScrollableLynxUI;
@property(nonatomic, weak) UIScrollView *nestedScrollView;
@property(nonatomic, assign) BOOL allowNestScrollViewBounces;
@property(nonatomic, weak) UIView *tabbarView;
@property(nonatomic, weak) UIView *slotDragView;
@property(nonatomic, assign) CGFloat expandHeight;
@property(nonatomic, assign) BOOL duringKVO;
@property(nonatomic, assign) BOOL duringDidScroll;
@property(nonatomic, assign) CGFloat limitedContentOffsetY;
@property(nonatomic, assign) CGFloat nestLimitedContentOffsetY;
@property(nonatomic, assign) BOOL scrollAttached;
@property(nonatomic, assign) BOOL forceScrollDetach;
@property(nonatomic, assign) BOOL forceScrollDetachWhileIdling;
@property(nonatomic, assign) CGFloat scrollViewFilter;
@property(nonatomic, strong) NSArray<NSString *> *excludeScrollViewNames;
@property(nonatomic, strong) LynxGestureConsumer *gestureConsumer;
@end

@implementation LynxScrollCoordinatorView

- (instancetype)init {
  if (self = [super init]) {
    self.alwaysBounceVertical = YES;
    self.alwaysBounceHorizontal = NO;
    self.showsVerticalScrollIndicator = NO;
    self.showsHorizontalScrollIndicator = NO;
    self.delegate = self;
    self.scrollsToTop = NO;
    if (@available(iOS 11.0, *)) {
      self.contentInsetAdjustmentBehavior = UIScrollViewContentInsetAdjustmentNever;
    }
    self.threshold = 1.0 / UIScreen.mainScreen.scale;
    self.lynxEnableTapGestureSimultaneously = YES;
    self.forceScrollDetachWhileIdling = YES;
  }
  return self;
}

- (BOOL)checkNestedScrollView:(UIScrollView *)scrollview {
  if ([self checkBlockList:scrollview]) {
    return NO;
  }
  if ([self checkAllowList:scrollview]) {
    return YES;
  }
  return [self checkVisibleVerticalScrollView:scrollview];
}

- (BOOL)checkVisibleVerticalScrollView:(UIScrollView *)scrollview {
  if (![scrollview isKindOfClass:UIScrollView.class]) {
    return NO;
  }
  if (scrollview.alwaysBounceHorizontal ||
      scrollview.contentSize.width > scrollview.bounds.size.width) {
    return NO;
  }

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wundeclared-selector"
  NSString *name = [scrollview respondsToSelector:@selector(name)]
                       ? [scrollview performSelector:@selector(name)]
                       : nil;
#pragma clang diagnostic pop
  if (name) {
    for (NSString *exclude in self.excludeScrollViewNames) {
      if ([name isEqualToString:exclude]) {
        return NO;
      }
    }
  }

  BOOL scrollViewVisibleInCoordinator = NO;
  CGFloat centerX =
      [scrollview convertPoint:CGPointMake(scrollview.bounds.size.width / 2.0, 0) toView:self].x;
  if (centerX >= 0 && centerX <= self.bounds.size.width) {
    scrollViewVisibleInCoordinator = YES;
  }

  if (scrollViewVisibleInCoordinator &&
      scrollview.frame.size.height >=
          (self.contentSize.height - self.expandHeight) * self.scrollViewFilter) {
    return YES;
  }
  return NO;
}

- (BOOL)checkAllowList:(UIScrollView *)scrollview {
  for (Class cls in @[
         LynxScrollCoordinatorLookupClass("LynxUICollectionView"),
         LynxScrollCoordinatorLookupClass("LynxScrollView")
       ]) {
    if ([scrollview isKindOfClass:cls] && [self checkVisibleVerticalScrollView:scrollview]) {
      return YES;
    }
  }
  return NO;
}

- (BOOL)checkBlockList:(UIScrollView *)scrollview {
  if ([self.uiDelegate shouldIgnoreNestedScrollView:scrollview]) {
    return YES;
  }
  for (Class cls in @[ LynxScrollCoordinatorLookupClass("LynxViewPager") ]) {
    if ([scrollview isKindOfClass:cls]) {
      return YES;
    }
  }
  return NO;
}

- (UIScrollView *)nestedScrollView {
  if (!_nestedScrollView) {
    UIScrollView *potentialScrollableView = (UIScrollView *)self.potentialRootScrollableLynxUI.view;
    if ([self checkNestedScrollView:potentialScrollableView]) {
      _nestedScrollView = potentialScrollableView;
    } else if ([self.potentialRootScrollableLynxUI isKindOfClass:LynxUIViewPager.class]) {
      _nestedScrollView =
          [self findNestedScrollView:potentialScrollableView.subviews.firstObject.subviews];
    } else {
      _nestedScrollView = [self findNestedScrollView:potentialScrollableView.subviews];
    }
    __weak typeof(self) weakSelf = self;
    [_nestedScrollView
        scrollCoordinator_addObserverBlockForKeyPath:@"contentOffset"
                                               block:^(__weak id obj, id oldVal, id newVal) {
                                                 __strong __typeof(weakSelf) strongSelf = weakSelf;
                                                 [strongSelf observeValue:newVal ofObject:obj];
                                               }];
    self.limitedContentOffsetY = self.contentOffset.y;
    self.nestLimitedContentOffsetY = _nestedScrollView.contentOffset.y;
    if (!_nestedScrollView) {
      self.scrollAttached = YES;
    } else if (!self.scrollAttached) {
      self.scrollAttached = [self checkAttached];
    }
  }
  return _nestedScrollView;
}

- (BOOL)isFolded {
  return ABS(self.contentOffset.y - self.expandHeight) < self.threshold;
}

- (BOOL)isFullyExpanded {
  return self.contentOffset.y <= 0;
}

- (BOOL)checkAttached {
  return self.nestedScrollView.contentOffset.y <= self.threshold || [self isFolded];
}

#pragma mark - KVO

- (void)observeValue:(id)value ofObject:(id)object {
  if (self.forceScrollDetach) {
    return;
  }

  if (!self.duringKVO && object == _nestedScrollView) {
    if (self.forceScrollDetachWhileIdling) {
      BOOL coordinatorIdling = !(self.dragging || self.tracking || self.decelerating);
      BOOL nestedScrollViewIdling =
          self.nestedScrollView == nil ||
          !(self.nestedScrollView.dragging || self.nestedScrollView.tracking ||
            self.nestedScrollView.decelerating);
      if (coordinatorIdling && nestedScrollViewIdling) {
        [self clearNestedScrollView];
        [self nestedScrollView];
        return;
      }
    }
    self.duringKVO = YES;
    CGPoint contentOffset = [value CGPointValue];
    CGFloat originY = contentOffset.y;
    if (!self.scrollAttached) {
      self.scrollAttached = [self checkAttached];
    }

    CGFloat topMost = _nestedScrollView ? -_nestedScrollView.contentInset.top : 0;

    if (self.scrollAttached) {
      if (![self isFolded]) {
        if ([self isFullyExpanded]) {
          contentOffset.y =
              MIN(topMost, self.allowNestScrollViewBounces ? contentOffset.y : topMost);
        } else {
          contentOffset.y =
              self.allowNestScrollViewBounces ? -self.nestedScrollView.contentInset.top : topMost;
        }
        if (contentOffset.y != originY) {
          [self.nestedScrollView setContentOffset:contentOffset];
        }
      }
    }

    if (![self isFullyExpanded] && contentOffset.y < topMost) {
      contentOffset.y = topMost;
      [self.nestedScrollView setContentOffset:contentOffset];
    }

    if (!self.scrollAttached) {
      if (contentOffset.y > self.nestLimitedContentOffsetY) {
        contentOffset.y = self.nestLimitedContentOffsetY;
        [self.nestedScrollView setContentOffset:contentOffset];
      } else {
        self.nestLimitedContentOffsetY = contentOffset.y;
      }
    }

    self.duringKVO = NO;
  }
}

- (void)clearNestedScrollView {
  [_nestedScrollView scrollCoordinator_removeObserverBlocksForKeyPath:@"contentOffset"];
  _nestedScrollView = nil;
  self.nestLimitedContentOffsetY = 0;
  self.scrollAttached = NO;
}

- (UIScrollView *)findNestedScrollView:(NSArray<__kindof UIView *> *)subviews {
  NSArray<UIView *> *reverseSubview = [[subviews reverseObjectEnumerator] allObjects];

  for (UIView *child in reverseSubview) {
    if ([self checkNestedScrollView:(UIScrollView *)child]) {
      return (UIScrollView *)child;
    }
  }

  for (UIView *child in reverseSubview) {
    UIScrollView *ret = [self findNestedScrollView:child.subviews];
    if (ret) {
      return ret;
    }
  }

  return nil;
}

- (void)adjustContentOffset {
  BOOL isDetached = self.forceScrollDetach;
  if (!isDetached) {
    self.forceScrollDetach = YES;
  }

  if (self.contentOffset.y > self.expandHeight) {
    [self setContentOffset:CGPointMake(self.contentOffset.x, self.expandHeight)];
    self.limitedContentOffsetY = self.expandHeight;
  }

  if (!isDetached) {
    self.forceScrollDetach = NO;
  }
}

- (void)dealloc {
  [self clearNestedScrollView];
}

- (void)layoutSubviews {
  [super layoutSubviews];
  [self nestedScrollView];
  [((LynxUIScrollCoordinator *)_uiDelegate).context.observer notifyScroll:nil];
}

- (void)setExpandHeight:(CGFloat)expandHeight {
  _expandHeight = expandHeight;
  [self setContentOffset:self.contentOffset];
}

- (void)scrollViewWillEndDragging:(UIScrollView *)scrollView
                     withVelocity:(CGPoint)velocity
              targetContentOffset:(inout CGPoint *)targetContentOffset {
  if ([self stopDeceleratingIfNecessaryWithTargetContentOffset:targetContentOffset]) {
    return;
  }
}

- (void)scrollViewDidScroll:(UIScrollView *)scrollView {
  [((LynxUIScrollCoordinator *)_uiDelegate).context.observer notifyScroll:nil];
  if (self.duringDidScroll) {
    return;
  }
  self.duringDidScroll = YES;
  CGPoint contentOffset = scrollView.contentOffset;

  if (contentOffset.y > self.expandHeight) {
    contentOffset = CGPointMake(contentOffset.x, self.expandHeight);
    [self setContentOffset:contentOffset];
  }

  if (self.forceScrollDetach) {
    [self.uiDelegate scrollCoordinatorDidScroll:scrollView];
    self.duringDidScroll = NO;
    return;
  }

  if (self.scrollAttached) {
    if (self.nestedScrollView && self.nestedScrollView.contentOffset.y > self.threshold) {
      contentOffset = CGPointMake(contentOffset.x, self.expandHeight);
      [self setContentOffset:contentOffset];
    }
  } else {
    BOOL nestedScrollViewIdling =
        self.nestedScrollView == nil ||
        !(self.nestedScrollView.dragging || self.nestedScrollView.tracking ||
          self.nestedScrollView.decelerating);
    if (contentOffset.y < self.limitedContentOffsetY && !nestedScrollViewIdling) {
      contentOffset = CGPointMake(contentOffset.x, self.limitedContentOffsetY);
      [self setContentOffset:contentOffset];
    } else {
      self.limitedContentOffsetY = contentOffset.y;
    }
  }

  [self.uiDelegate scrollCoordinatorDidScroll:scrollView];
  self.duringDidScroll = NO;
}

- (BOOL)scrollViewShouldScrollToTop:(UIScrollView *)scrollView {
  if (self.scrollsToTop) {
    self.forceScrollDetach = YES;
    BOOL userInteractionEnabled = self.userInteractionEnabled;
    self.userInteractionEnabled = NO;

    [self setContentOffset:CGPointZero animated:YES];
    [self.nestedScrollView setContentOffset:CGPointZero animated:YES];

    __weak typeof(self) weakSelf = self;
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(200 * NSEC_PER_MSEC)),
                   dispatch_get_main_queue(), ^{
                     weakSelf.userInteractionEnabled = userInteractionEnabled;
                     weakSelf.forceScrollDetach = NO;
                   });
  }
  return NO;
}

- (BOOL)gestureRecognizerShouldBegin:(UIGestureRecognizer *)gestureRecognizer {
  if (self.tabbarView && CGRectContainsPoint(self.tabbarView.bounds,
                                             [gestureRecognizer locationInView:self.tabbarView])) {
    return NO;
  }
  if (self.slotDragView &&
      CGRectContainsPoint(self.slotDragView.bounds,
                          [gestureRecognizer locationInView:self.slotDragView])) {
    return NO;
  }

  if ([gestureRecognizer isKindOfClass:UIPanGestureRecognizer.class] &&
      [(UIPanGestureRecognizer *)gestureRecognizer velocityInView:self].y > 0 &&
      self.allowNestScrollViewBounces && [self isFullyExpanded] && !self.bounces) {
    return NO;
  }

  return YES;
}

- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer
    shouldRecognizeSimultaneouslyWithGestureRecognizer:
        (UIGestureRecognizer *)otherGestureRecognizer {
  [self disableGesturesRecursivelyIfNecessary:self.gestureConsumer];

  UIView *currentScrollingList = self.nestedScrollView;
  return [gestureRecognizer isKindOfClass:UIPanGestureRecognizer.class] &&
         [otherGestureRecognizer isKindOfClass:UIPanGestureRecognizer.class] &&
         otherGestureRecognizer.view == currentScrollingList;
}

- (UIView *)hitTest:(CGPoint)point withEvent:(UIEvent *)event {
  if (![self checkNestedScrollView:_nestedScrollView]) {
    [self clearNestedScrollView];
    [self nestedScrollView];
  }
  return [super hitTest:point withEvent:event];
}

@end

typedef NS_ENUM(NSInteger, LynxUIScrollCoordinatorHeaderOverSlot) {
  LynxUIScrollCoordinatorHeaderOverSlotHeaderAtTop = -1,
  LynxUIScrollCoordinatorHeaderOverSlotUndefined = 0,
  LynxUIScrollCoordinatorHeaderOverSlotSlotAtTop = 1,
};

@interface LynxUIScrollCoordinator () <LynxUIScrollCoordinatorInternalDelegate,
                                       LynxUIViewPagerDelegate>
@property(nonatomic, assign) BOOL bounceWithToolbar;
@property(nonatomic, assign) CGFloat scrollViewFilter;
@property(nonatomic, assign) BOOL shouldInvalidateLayout;
@property(nonatomic, strong) LynxScrollCoordinatorView *scrollCoordinator;
@property(nonatomic, weak) LynxUIScrollCoordinatorToolbar *toolbar;
@property(nonatomic, weak) LynxUIScrollCoordinatorHeader *header;
@property(nonatomic, weak) LynxUIScrollCoordinatorSlot *slot;
@property(nonatomic, assign) CGFloat preOffset;
@property(nonatomic, assign) CGFloat granularity;
@property(nonatomic, assign) BOOL forceDetachScroll;
@property(nonatomic, strong) NSArray<NSString *> *excludeLynxUINames;
@property(nonatomic, assign) CGFloat topPaddingForNative;
@property(nonatomic, assign) LynxUIScrollCoordinatorHeaderOverSlot headerOverSlot;
@property(nonatomic, assign) BOOL headerSlotOverflowHitTest;
@end

@implementation LynxUIScrollCoordinator

- (instancetype)init {
  if (self = [super init]) {
    _scrollViewFilter = 0.5;
    _granularity = 0.01;
    _headerSlotOverflowHitTest = YES;
  }
  return self;
}

#pragma mark - Hooks

- (LynxScrollCoordinatorView *)createScrollCoordinatorView {
  return [[LynxScrollCoordinatorView alloc] init];
}

- (void)prepareForRootScrollableUIResolution {
}

- (void)didResolveRootScrollableUI:(LynxUI *)rootScrollableUI {
}

- (BOOL)shouldIgnoreNestedScrollView:(UIScrollView *)scrollView {
  return NO;
}

- (NSString *)coordinatorExceptionName {
  return @"scroll-coordinator";
}

- (NSString *)missingRequiredChildReason {
  return @"scroll-coordinator-header and scroll-coordinator-slot must be set";
}

- (NSString *)unsupportedChildReason {
  return @"scroll-coordinator only supports scroll-coordinator-header, scroll-coordinator-slot, or "
         @"scroll-coordinator-toolbar as its children";
}

#pragma mark - LynxUI

- (UIView *)createView {
  self.scrollCoordinator = [self createScrollCoordinatorView];
  self.scrollCoordinator.uiDelegate = self;
  UIView *view = [[UIView alloc] init];
  [view addSubview:self.scrollCoordinator];
  return view;
}

- (void)updateFrame:(CGRect)frame
            withPadding:(UIEdgeInsets)padding
                 border:(UIEdgeInsets)border
                 margin:(UIEdgeInsets)margin
    withLayoutAnimation:(BOOL)with {
  [super updateFrame:frame
              withPadding:padding
                   border:border
                   margin:margin
      withLayoutAnimation:with];
  [self.scrollCoordinator setFrame:self.view.bounds];
  self.shouldInvalidateLayout = YES;
}

- (void)layoutDidFinished {
  [super layoutDidFinished];
}

- (void)finishLayoutOperation {
  [super finishLayoutOperation];

  if (!self.header || !self.slot) {
    return;
  }

  NSUInteger headerIndex = [self.children indexOfObject:self.header];
  NSUInteger slotIndex = [self.children indexOfObject:self.slot];
  NSUInteger headerViewIndex = [[self.scrollCoordinator subviews] indexOfObject:self.header.view];
  NSUInteger slotViewIndex = [[self.scrollCoordinator subviews] indexOfObject:self.slot.view];

  if (self.headerOverSlot == LynxUIScrollCoordinatorHeaderOverSlotHeaderAtTop) {
    headerIndex = 1;
    slotIndex = 0;
  } else if (self.headerOverSlot == LynxUIScrollCoordinatorHeaderOverSlotSlotAtTop) {
    headerIndex = 0;
    slotIndex = 1;
  }

  if (((NSInteger)headerIndex - (NSInteger)slotIndex) *
          ((NSInteger)headerViewIndex - (NSInteger)slotViewIndex) <=
      0) {
    [self.header.view removeFromSuperview];
    [self.slot.view removeFromSuperview];
    if (headerIndex < slotIndex) {
      [self.scrollCoordinator addSubview:self.header.view];
      [self.scrollCoordinator addSubview:self.slot.view];
    } else {
      [self.scrollCoordinator addSubview:self.slot.view];
      [self.scrollCoordinator addSubview:self.header.view];
    }
    self.shouldInvalidateLayout = YES;
  }

  [self prepareForRootScrollableUIResolution];
  [self clearResolvedRootScrollableState];
  self.scrollCoordinator.scrollViewFilter = self.scrollViewFilter;

  NSMutableArray<LynxComponent *> *reverseChildren =
      [[[self.slot.children reverseObjectEnumerator] allObjects] mutableCopy];
  self.scrollCoordinator.potentialRootScrollableLynxUI =
      [self findPotentialRootScrollableLynxUI:reverseChildren];
  if ([self isViewPagerUI:self.scrollCoordinator.potentialRootScrollableLynxUI]) {
    [self bindViewPagerRecursively:self.scrollCoordinator.potentialRootScrollableLynxUI depth:3];
  }
  [self didResolveRootScrollableUI:self.scrollCoordinator.potentialRootScrollableLynxUI];

  [self setupSlotDrag];

  if (self.header.headerHeightChanged || self.toolbar.toolbarHeightChanged ||
      self.slot.slotHeightChanged) {
    self.header.headerHeightChanged = NO;
    self.toolbar.toolbarHeightChanged = NO;
    self.slot.slotHeightChanged = NO;
    self.shouldInvalidateLayout = YES;
  }

  if (self.shouldInvalidateLayout) {
    self.shouldInvalidateLayout = NO;
    self.header.view.frame =
        CGRectMake(CGRectGetMinX(self.header.view.frame), 0, CGRectGetWidth(self.header.view.frame),
                   CGRectGetHeight(self.header.view.frame));
    self.slot.view.frame =
        CGRectMake(CGRectGetMinX(self.slot.view.frame), CGRectGetHeight(self.header.view.frame),
                   CGRectGetWidth(self.slot.view.frame), CGRectGetHeight(self.slot.view.frame));
    self.toolbar.view.frame = CGRectMake(CGRectGetMinX(self.toolbar.view.frame), 0,
                                         CGRectGetWidth(self.toolbar.view.frame),
                                         CGRectGetHeight(self.toolbar.view.frame));

    self.scrollCoordinator.expandHeight =
        MAX(0, self.header.view.bounds.size.height - self.toolbar.view.bounds.size.height);
    self.scrollCoordinator.contentSize =
        CGSizeMake(self.view.bounds.size.width,
                   self.header.view.bounds.size.height + self.slot.view.bounds.size.height);

    [self.scrollCoordinator adjustContentOffset];
  }

  [self.scrollCoordinator nestedScrollView];
  if (self.scrollCoordinator.contentOffset.y > self.scrollCoordinator.expandHeight) {
    [self.scrollCoordinator setContentOffset:CGPointMake(self.scrollCoordinator.contentOffset.x,
                                                         self.scrollCoordinator.expandHeight)];
  }

  UIEdgeInsets inset = self.scrollCoordinator.contentInset;
  if (inset.top != self.topPaddingForNative) {
    inset.top = self.topPaddingForNative;
    self.scrollCoordinator.contentInset = inset;
    self.scrollCoordinator.contentOffset = CGPointMake(0, -self.topPaddingForNative);
  }

  [self.slot updateManagerRelated];
}

- (void)onNodeReady {
  [super onNodeReady];
  if (!self.header || !self.slot) {
    @throw [NSException exceptionWithName:[self coordinatorExceptionName]
                                   reason:[self missingRequiredChildReason]
                                 userInfo:nil];
  }
}

- (void)insertChild:(LynxUI *)child atIndex:(NSInteger)index {
  [super insertChild:child atIndex:index];
  if ([child isKindOfClass:LynxUIScrollCoordinatorHeader.class]) {
    self.header = (LynxUIScrollCoordinatorHeader *)child;
    [self.header.view removeFromSuperview];
    [self.scrollCoordinator addSubview:self.header.view];
    self.shouldInvalidateLayout = YES;
  } else if ([child isKindOfClass:LynxUIScrollCoordinatorSlot.class]) {
    self.slot = (LynxUIScrollCoordinatorSlot *)child;
    [self.slot.view removeFromSuperview];
    [self.scrollCoordinator addSubview:self.slot.view];
    self.shouldInvalidateLayout = YES;
  } else if ([child isKindOfClass:LynxUIScrollCoordinatorToolbar.class]) {
    self.toolbar = (LynxUIScrollCoordinatorToolbar *)child;
    [self.view bringSubviewToFront:self.toolbar.view];
    self.shouldInvalidateLayout = YES;
  } else {
    @throw [NSException exceptionWithName:[self coordinatorExceptionName]
                                   reason:[self unsupportedChildReason]
                                 userInfo:nil];
  }
}

- (void)removeChild:(LynxUI *)child atIndex:(NSInteger)index {
  [super removeChild:child atIndex:index];
  if ([child isKindOfClass:LynxUIScrollCoordinatorHeader.class]) {
    self.header = nil;
    self.shouldInvalidateLayout = YES;
  } else if ([child isKindOfClass:LynxUIScrollCoordinatorSlot.class]) {
    self.slot = nil;
    self.shouldInvalidateLayout = YES;
  }
}

- (id<LynxEventTarget>)hitTest:(CGPoint)oriPoint withEvent:(UIEvent *)event {
  [self resetInterceptGesture];
  LynxUI *guard = nil;
  NSMutableArray<LynxUI *> *array = [[NSMutableArray alloc] init];

  if (self.toolbar) {
    [array addObject:self.toolbar];
  }

  if (self.headerSlotOverflowHitTest) {
    if (self.headerOverSlot == LynxUIScrollCoordinatorHeaderOverSlotHeaderAtTop) {
      [array addObject:self.header];
      [array addObject:self.slot];
    } else if (self.headerOverSlot == LynxUIScrollCoordinatorHeaderOverSlotSlotAtTop) {
      [array addObject:self.slot];
      [array addObject:self.header];
    } else {
      [array addObject:self.header];
      [array addObject:self.slot];
    }
  } else {
    [array addObject:self.header];
    [array addObject:self.slot];
  }

  for (LynxUI *child in array) {
    CGPoint point = [self.view convertPoint:oriPoint toView:child.view];
    if (![child shouldHitTest:point withEvent:event] || child.view.hidden) {
      continue;
    }
    BOOL contain = NO;
    if (self.headerSlotOverflowHitTest) {
      contain = self.context.enableEventRefactor ? [child containsPoint:point]
                                                 : [child containsPoint:oriPoint];
    } else {
      contain = CGRectContainsPoint(child.view.bounds, point);
    }
    if (contain) {
      if (child.isOnResponseChain) {
        guard = child;
        break;
      }
      if (guard == nil || guard.getTransationZ < child.getTransationZ) {
        guard = child;
      }
    }
  }

  if (guard == nil) {
    return self;
  }
  oriPoint = [self.view convertPoint:oriPoint toView:guard.view];
  return [guard hitTest:oriPoint withEvent:event];
}

- (BOOL)isScrollContainer {
  return YES;
}

#pragma mark - LYNX_PROPS

LYNX_PROP_SETTER("bounces", setBounces, BOOL) { self.scrollCoordinator.bounces = value; }

LYNX_PROP_SETTER("allow-vertical-bounce", allowVerticalBounce, BOOL) {
  [self setBounces:value requestReset:requestReset];
}

LYNX_PROP_SETTER("granularity", granularity, CGFloat) { self.granularity = value; }

LYNX_PROP_SETTER("scroll-bar-enable", scrollBarEnable, BOOL) {
  self.scrollCoordinator.showsVerticalScrollIndicator = value;
}

LYNX_PROP_SETTER("refresh-mode", refreshMode, NSString *) {
  self.scrollCoordinator.allowNestScrollViewBounces = [value isEqualToString:@"page"];
  self.bounceWithToolbar = [value isEqualToString:@"fold"];
}

LYNX_PROP_SETTER("scroll-enable", scrollEnable, BOOL) {
  self.scrollCoordinator.scrollEnabled = value;
  if (!value) {
    self.scrollCoordinator.panGestureRecognizer.state = UIGestureRecognizerStateCancelled;
  }
}

LYNX_PROP_SETTER("ios-force-scroll-detach", setForceDetach, BOOL) {
  self.scrollCoordinator.forceScrollDetachWhileIdling = NO;
  self.forceDetachScroll = value;
  self.scrollCoordinator.forceScrollDetach = value;
}

LYNX_PROP_SETTER("ios-scroll-view-filter", setScrollFilter, NSNumber *) {
  self.scrollViewFilter = [value floatValue];
}

LYNX_PROP_SETTER("ios-scroll-exclude", setScrollExclude, NSString *) {
  self.excludeLynxUINames = [value componentsSeparatedByString:@","];
  self.scrollCoordinator.excludeScrollViewNames = self.excludeLynxUINames;
}

/**
 * @name: ios-top-padding-for-native
 * @description: We need to leave some space for the native container at the top of the scroll
 *coordinator under some circumstances. We cannot use css padding because it would change the layout
 *inside the coordinator, so we adjust the native `contentInset` instead.
 * @category: different
 * @standardAction: keep
 * @supportVersion: 2.8
 **/
LYNX_PROP_SETTER("ios-top-padding-for-native", setTopPadding, NSString *) {
  self.topPaddingForNative = [self toPtWithUnitValue:value fontSize:0];
}

#pragma mark - LYNX_UI_METHOD

LYNX_UI_METHOD(setFoldExpanded) {
  CGFloat offset = [LynxUnitUtils toPtFromUnitValue:[params objectForKey:@"offset"]];
  BOOL smooth = [params objectForKey:@"smooth"] ? [[params objectForKey:@"smooth"] boolValue] : YES;
  offset = MAX(MIN(offset, self.scrollCoordinator.expandHeight), 0);

  self.scrollCoordinator.forceScrollDetach = YES;
  [self.scrollCoordinator
      setContentOffset:CGPointMake(self.scrollCoordinator.contentOffset.x, offset)
              animated:smooth];

  if (callback) {
    if (!smooth) {
      self.scrollCoordinator.forceScrollDetach = self.forceDetachScroll;
      [self.scrollCoordinator clearNestedScrollView];
      [self.scrollCoordinator nestedScrollView];
      callback(kUIMethodSuccess, nil);
    } else {
      __weak typeof(self) weakSelf = self;
      dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(300 * NSEC_PER_MSEC)),
                     dispatch_get_main_queue(), ^{
                       weakSelf.scrollCoordinator.forceScrollDetach = weakSelf.forceDetachScroll;
                       [weakSelf.scrollCoordinator clearNestedScrollView];
                       [weakSelf.scrollCoordinator nestedScrollView];
                       callback(kUIMethodSuccess, nil);
                     });
    }
  } else {
    self.scrollCoordinator.forceScrollDetach = self.forceDetachScroll;
    [self.scrollCoordinator clearNestedScrollView];
    [self.scrollCoordinator nestedScrollView];
  }
}

LYNX_UI_METHOD(getScrollInfo) {
  if (callback) {
    callback(
        kUIMethodSuccess, @{
          @"scrollX" : @(0),
          @"scrollY" : @(self.scrollCoordinator.contentOffset.y),
          @"scrollRange" : @(self.scrollCoordinator.expandHeight),
        });
  }
}

LYNX_UI_METHOD(scrollBy) {
  if (![params objectForKey:@"offset"]) {
    if (callback) {
      callback(kUIMethodParamInvalid,
               @{@"msg" : @"Invoke scrollBy failed due to index param is null"});
    }
    return;
  }

  CGFloat offset = ((NSNumber *)[params objectForKey:@"offset"]).floatValue;
  CGPoint preOffset = self.scrollCoordinator.contentOffset;
  NSArray<NSNumber *> *res = [self scrollBy:0 deltaY:offset];
  CGPoint postOffset = CGPointMake(res.firstObject.floatValue, res.lastObject.floatValue);

  int consumedX = postOffset.x - preOffset.x;
  int consumedY = postOffset.y - preOffset.y;
  int unconsumedX = offset - consumedX;
  int unconsumedY = offset - consumedY;

  NSDictionary *scrollResults = @{
    @"consumedX" : @(consumedX),
    @"consumedY" : @(consumedY),
    @"unconsumedX" : @(unconsumedX),
    @"unconsumedY" : @(unconsumedY)
  };
  if (callback) {
    callback(kUIMethodSuccess, scrollResults);
  }
}

#pragma mark - LynxUIScrollCoordinatorInternalDelegate

- (void)scrollCoordinatorDidScroll:(UIScrollView *)scrollView {
  if (scrollView == self.scrollCoordinator &&
      ![self.scrollCoordinator
          respondToScrollViewDidScroll:self.scrollCoordinator.gestureConsumer]) {
    return;
  }

  CGFloat offset = scrollView.contentOffset.y;
  if (self.bounceWithToolbar) {
    CGRect toolbarFrame =
        CGRectMake(0, offset < 0 ? -offset : 0, self.toolbar.view.frame.size.width,
                   self.toolbar.view.frame.size.height);
    [self.toolbar.view setFrame:toolbarFrame];
  }
  CGFloat height = self.scrollCoordinator.expandHeight;
  if (ABS(offset - self.preOffset) >= height * self.granularity ||
      (offset != self.preOffset && (offset == 0 || ABS(offset - height) < DBL_EPSILON))) {
    [self.context.eventEmitter
        sendCustomEvent:[[LynxDetailEvent alloc]
                            initWithName:@"offset"
                              targetSign:[self sign]
                                  detail:@{@"offset" : @(offset), @"height" : @(height)}]];
    self.preOffset = offset;
  }
}

#pragma mark - LynxUIViewPagerDelegate

- (void)didIndexChanged:(NSUInteger)index {
  [self.scrollCoordinator clearNestedScrollView];
  [self.scrollCoordinator nestedScrollView];
}

#pragma mark - Internal

- (void)bindViewPagerRecursively:(LynxUI *)currentUI depth:(NSInteger)depth {
  if ([currentUI isKindOfClass:LynxUIViewPager.class]) {
    ((LynxUIViewPager *)currentUI).pagerDelegate = self;
  }
  if (depth == 0) {
    return;
  }
  for (LynxUI *child in currentUI.children) {
    [self bindViewPagerRecursively:child depth:depth - 1];
  }
}

- (void)setupSlotDrag {
  self.scrollCoordinator.slotDragView =
      self.slot.slotDrag.forbidMovable ? self.slot.slotDrag.view : nil;
}

- (void)clearResolvedRootScrollableState {
  self.scrollCoordinator.tabbarView = nil;
  [self.scrollCoordinator clearNestedScrollView];
}

- (LynxUI *)findPotentialRootScrollableLynxUI:(NSMutableArray<LynxComponent *> *)children {
  for (LynxComponent *child in children) {
    if ([self checkScrollable:child]) {
      return (LynxUI *)child;
    }
  }

  for (LynxComponent *child in children) {
    LynxUI *ret = [self findPotentialRootScrollableLynxUI:child.children];
    if (ret) {
      return ret;
    }
  }

  return nil;
}

- (BOOL)checkScrollable:(LynxComponent *)component {
  LynxUI *lynxUI = (LynxUI *)component;

  for (NSString *exclude in self.excludeLynxUINames) {
    if ([exclude isEqualToString:lynxUI.name]) {
      return NO;
    }
  }

  if ([lynxUI isKindOfClass:LynxUI.class]) {
    if ([self isViewPagerUI:lynxUI]) {
      return YES;
    }
    if ([lynxUI isKindOfClass:LynxScrollCoordinatorLookupClass("LynxUICollection")] ||
        [lynxUI isKindOfClass:LynxScrollCoordinatorLookupClass("AbsLynxUIScroller")] ||
        [lynxUI.view isKindOfClass:UIScrollView.class]) {
      if (lynxUI.view.frame.size.height >=
          self.slot.view.frame.size.height * self.scrollViewFilter) {
        return YES;
      }
    }
  }
  return NO;
}

- (BOOL)isViewPagerUI:(LynxUI *)lynxUI {
  return [lynxUI isKindOfClass:LynxUIViewPager.class];
}

LYNX_PROPS_GROUP_DECLARE(LYNX_PROP_DECLARE("ios-scrolls-to-top", iosScrollsToTop, BOOL),
                         LYNX_PROP_DECLARE("header-over-slot", headerOverSlot, BOOL),
                         LYNX_PROP_DECLARE("experimental-header-slot-overflow-hit-test",
                                           headerSlotOverflowHitTest, BOOL))

/**
 * @name: header-over-slot
 * @description: This property controls whether the header hierarchy is higher than the slot. When
 *the header overflows, it is displayed on top of the slot; otherwise it is displayed below the
 *slot. It is better to set this property explicitly instead of relying on the default.
 * @category: experimental
 * @standardAction: keep
 * @supportVersion: 2.11
 **/
LYNX_PROP_DEFINE("header-over-slot", headerOverSlot, BOOL) {
  self.headerOverSlot = value ? LynxUIScrollCoordinatorHeaderOverSlotHeaderAtTop
                              : LynxUIScrollCoordinatorHeaderOverSlotSlotAtTop;
}

/**
 * @name: experimental-header-slot-overflow-hit-test
 * @description: Controls whether the header/slot overflow hit-testing is enabled. When true,
 *hit-testing follows the current stacking order between header and slot; when false, it always
 *checks header before slot.
 * @category: experimental
 * @standardAction: offline
 * @supportVersion: 3.6
 **/
LYNX_PROP_DEFINE("experimental-header-slot-overflow-hit-test", headerSlotOverflowHitTest, BOOL) {
  self.headerSlotOverflowHitTest = value;
}

/**
 * @name: ios-scrolls-to-top
 * @description: When the user taps the status bar, the scroll view beneath the touch that is
 *closest to the status bar scrolls to top. iOS feature only.
 * @category: different
 * @standardAction: keep
 * @supportVersion: 2.12
 **/
LYNX_PROP_DEFINE("ios-scrolls-to-top", iosScrollsToTop, BOOL) {
  self.scrollCoordinator.scrollsToTop = value;
}

#pragma mark - LynxNewGesture

- (void)gestureDidSet {
  if (!self.context.enableNewGesture) {
    return;
  }
  [super gestureDidSet];
  [self ensureGestureConsumer];
}

- (void)ensureGestureConsumer {
  if (!self.scrollCoordinator.gestureConsumer) {
    [self.gestureMap
        enumerateKeysAndObjectsUsingBlock:^(
            NSNumber *_Nonnull key, LynxGestureDetectorDarwin *_Nonnull obj, BOOL *_Nonnull stop) {
          if (obj.gestureType == LynxGestureTypeNative) {
            self.scrollCoordinator.gestureConsumer = [[LynxGestureConsumer alloc] init];
            self.scrollCoordinator.scrollEnabled = YES;
          }
        }];
  }
}

- (void)consumeInternalGesture:(BOOL)consume {
  [self.scrollCoordinator.gestureConsumer consumeGesture:consume];
}

- (void)interceptGesture:(BOOL)intercept {
  [self.scrollCoordinator.gestureConsumer interceptGesture:intercept];
}

- (void)resetInterceptGesture {
  self.scrollCoordinator.gestureConsumer.interceptGestureStatus = LynxInterceptGestureStateUnset;
}

- (BOOL)canConsumeGesture:(CGPoint)delta {
  return [self.scrollCoordinator consumeDeltaOffset:delta vertical:YES];
}

- (CGFloat)getMemberScrollX {
  return self.scrollCoordinator.contentOffset.x;
}

- (CGFloat)getMemberScrollY {
  return self.scrollCoordinator.contentOffset.y;
}

- (int)getScrollContainerDirection {
  return DIRECTION_VERTICAL;
}

- (NSArray<NSNumber *> *)scrollBy:(CGFloat)deltaX deltaY:(CGFloat)deltaY {
  [self onGestureScrollBy:CGPointMake(deltaX, deltaY)];
  return @[ @(self.scrollCoordinator.contentOffset.x), @(self.scrollCoordinator.contentOffset.y) ];
}

- (void)onGestureScrollBy:(CGPoint)delta {
  CGPoint point = self.scrollCoordinator.contentOffset;
  point.y += delta.y;
  [self.scrollCoordinator setContentOffset:point];

  if (fabs(delta.x) > kScrollByEpsilon || fabs(delta.y) > kScrollByEpsilon) {
    [self.context onGestureRecognizedByUI:self];
  }
}

- (BOOL)getGestureBorder:(BOOL)start {
  if (start) {
    return ![self.scrollCoordinator consumeDeltaOffset:CGPointMake(-0.1, -0.1) vertical:YES];
  }
  if (self.scrollCoordinator.nestedScrollView) {
    return ![self.scrollCoordinator.nestedScrollView consumeDeltaOffset:CGPointMake(0.1, 0.1)
                                                               vertical:YES];
  }
  return ![self.scrollCoordinator consumeDeltaOffset:CGPointMake(0.1, 0.1) vertical:YES];
}

@end
