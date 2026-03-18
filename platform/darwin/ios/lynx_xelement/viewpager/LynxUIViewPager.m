// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxGlobalObserver.h>
#import <Lynx/LynxPropsProcessor.h>
#import <Lynx/LynxUIMethodProcessor.h>
#import <Lynx/UIView+Lynx.h>
#import <XElement/LynxUIViewPager.h>
#import <XElement/LynxUIViewPagerItem.h>

static const CGFloat kLynxViewPagerChangeEpsilonThreshold = 1.0f;
static const NSInteger kInvalidViewPagerIndex = -1;

@protocol LynxUIViewPagerProtocol <NSObject>

- (void)didLayoutSubviews:(UIScrollView *)scrollView;
- (UIView *)lynxView;
- (Class)recognizedGestureClass;
- (NSInteger)recognizedGestureViewTag;
- (BOOL)disableImplicitAnimation;
- (BOOL)isRtl;

@end

@interface LynxViewPagerCell : UICollectionViewCell

@end

@implementation LynxViewPagerCell

@end

@interface LynxViewPager : UICollectionView
@property(nonatomic, weak) id<LynxUIViewPagerProtocol> uiDelegate;
@property(nonatomic, assign) BOOL attachScroll;
@property(nonatomic, assign) CGFloat gestureBeginOffset;
@end

@implementation LynxViewPager

- (instancetype)initWithFrame:(CGRect)frame collectionViewLayout:(UICollectionViewLayout *)layout {
  if (self = [super initWithFrame:frame collectionViewLayout:layout]) {
    self.backgroundColor = [UIColor clearColor];
    self.showsHorizontalScrollIndicator = NO;
    self.showsVerticalScrollIndicator = NO;
    self.scrollsToTop = NO;
    self.pagingEnabled = YES;
    // We disable UICollectionView's allowsSelection, because it will stop anim by default.
    // And Lynx has its own events system, we don't need UICollectionView's selection behavior here.
    self.allowsSelection = NO;
    [self registerClass:LynxViewPagerCell.class
        forCellWithReuseIdentifier:NSStringFromClass(LynxViewPagerCell.class)];
    if (@available(iOS 10.0, *)) {
      self.prefetchingEnabled = NO;
    }
    if (@available(iOS 11.0, *)) {
      if ([self respondsToSelector:@selector(setContentInsetAdjustmentBehavior:)]) {
        self.contentInsetAdjustmentBehavior = UIScrollViewContentInsetAdjustmentNever;
      }
    }
  }
  return self;
}

- (void)layoutSubviews {
  if (self.uiDelegate.disableImplicitAnimation) {
    [CATransaction begin];
    [CATransaction setDisableActions:YES];
  }
  [super layoutSubviews];
  [self.uiDelegate didLayoutSubviews:self];
  if (self.uiDelegate.disableImplicitAnimation) {
    [CATransaction commit];
  }
  // Notify layout did finish.
  [((LynxUIViewPager *)_uiDelegate).context.observer notifyScroll:nil];
}

- (BOOL)gestureRecognizerShouldBegin:(UIPanGestureRecognizer *)panGestureRecognizer {
  // `threshold` represent the margin between the touch point and the scrolling offset from left.
  // And in RTL, the scrolling offset should be caculated from right.
  CGFloat threshold = ([panGestureRecognizer locationInView:self].x - self.contentOffset.x);

  if ([self.uiDelegate isRtl]) {
    threshold = self.bounds.size.width - threshold;
  }

  if (self.gestureBeginOffset != 0 &&
      [panGestureRecognizer isKindOfClass:[UIPanGestureRecognizer class]] &&
      threshold < self.gestureBeginOffset) {
    return NO;
  }

  return YES;
}

- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer
    shouldRecognizeSimultaneouslyWithGestureRecognizer:
        (UIGestureRecognizer *)otherGestureRecognizer {
  // Allow another gesture to work together with viewpager.
  if ([otherGestureRecognizer isKindOfClass:[self.uiDelegate recognizedGestureClass]] &&
      otherGestureRecognizer.view.tag == [self.uiDelegate recognizedGestureViewTag]) {
    if (otherGestureRecognizer.state == UIGestureRecognizerStateBegan ||
        otherGestureRecognizer.state == UIGestureRecognizerStatePossible) {
      // force to cast to UIPanGesture, because we only recognize this kind of gesture.
      CGPoint velocity = [(UIPanGestureRecognizer *)otherGestureRecognizer velocityInView:self];
      if (velocity.x > 0 && self.contentOffset.x <= 0) {
        return YES;
      } else if (velocity.x < 0 &&
                 self.contentOffset.x >= self.contentSize.width - self.bounds.size.width) {
        return YES;
      }
    }
    return NO;
  }

  if ([otherGestureRecognizer.view isKindOfClass:NSClassFromString(@"UILayoutContainerView")]) {
    // Adapt to the `Swipe to go bace` gesture on iOS.
    // Allow the gesture only if the ViewPager is at right-most position, in RTL.
    BOOL respondsToGoBackGesture =
        [self.uiDelegate isRtl]
            ? self.contentOffset.x >= (self.contentSize.width - self.bounds.size.width)
            : self.contentOffset.x <= 0;

    if ((otherGestureRecognizer.state == UIGestureRecognizerStateBegan ||
         otherGestureRecognizer.state == UIGestureRecognizerStatePossible) &&
        respondsToGoBackGesture && !self.bounces) {
      return YES;
    }
  }

  if (self.attachScroll && [otherGestureRecognizer.view isKindOfClass:UIScrollView.class] &&
      [otherGestureRecognizer isKindOfClass:UIPanGestureRecognizer.class] &&
      [self.uiDelegate.lynxView isDescendantOfView:otherGestureRecognizer.view] &&
      (otherGestureRecognizer.state == UIGestureRecognizerStateBegan ||
       otherGestureRecognizer.state == UIGestureRecognizerStatePossible) &&
      ((UIScrollView *)(otherGestureRecognizer.view)).contentSize.width >
          otherGestureRecognizer.view.bounds.size.width) {
    CGPoint velocity = [(UIPanGestureRecognizer *)otherGestureRecognizer velocityInView:self];
    if (velocity.x > 0 && self.contentOffset.x <= 0) {
      return YES;
    } else if (velocity.x < 0 &&
               self.contentOffset.x >= self.contentSize.width - self.bounds.size.width) {
      return YES;
    }
  }

  return NO;
}

@end

@protocol LynxUIViewPagerContainerDelegate <NSObject>
- (void)containerDidLayoutSubviews:(UIView *)container;
@end

@interface LynxViewPagerContainer : UIView
@property(nonatomic, weak) id<LynxUIViewPagerContainerDelegate> delegate;
@end

@implementation LynxViewPagerContainer
- (void)layoutSubviews {
  [super layoutSubviews];
  [self.delegate containerDidLayoutSubviews:self];
}
@end

@interface LynxUIViewPager () <UICollectionViewDataSource,
                               UICollectionViewDelegate,
                               LynxUIViewPagerProtocol,
                               LynxUIViewPagerContainerDelegate>
@property(nonatomic, assign) BOOL ignoreScrollEvent;
@property(nonatomic, assign) NSNumber *disablePageChangeAnim;
@property(nonatomic, assign) BOOL shouldReloadData;
@property(nonatomic, assign) BOOL shouldInvalidateLayout;
@property(nonatomic, strong) LynxViewPager *viewpager;
@property(nonatomic, assign) BOOL hasInnerTabbar;
@property(nonatomic, assign) NSUInteger currentIndex;
@property(nonatomic, assign) NSInteger frontEndCurrentIndex;
@property(nonatomic, assign) BOOL singlePageTouch;
@property(nonatomic, assign) BOOL scrollEnable;
@property(nonatomic, assign) NSInteger indexAfterReload;
@property(nonatomic, assign) NSInteger initialSelectIndex;
@property(nonatomic, copy) LynxUIMethodCallbackBlock selectTabCallback;
@property(nonatomic, assign) BOOL keepItemView;
@property(nonatomic, assign) BOOL tabWasSelectedBeforeTouch;
@property(nonatomic, assign) BOOL notReloadItemsAfterLayout;
@property(nonatomic, assign) NSInteger recognizedGestureViewTag;
@property(nonatomic, strong) Class recognizedGestureClass;
@property(nonatomic, assign) NSUInteger emitTargetChangesOnlyDuringScroll;
@property(nonatomic, assign) CGFloat pagerChangeEpsilon;
@property(nonatomic, assign) NSInteger selectTabCounter;
@property(nonatomic, assign) NSInteger viewpagerWidthChangedWithMarkedIndex;

@end

@implementation LynxUIViewPager

#pragma mark - LynxUI

- (instancetype)init {
  if (self = [super init]) {
    self.scrollEnable = YES;
    // Reset this value when reloadTemplate gets an explicit lifecycle callback.
    self.initialSelectIndex = kInvalidViewPagerIndex;
    self.firstRender = YES;
    self.disablePageChangeAnim = @(NO);
    self.emitTargetChangesOnlyDuringScroll = -1;
    self.pagerChangeEpsilon = 1.0f / UIScreen.mainScreen.scale;
    self.viewpagerWidthChangedWithMarkedIndex = -1;
    self.frontEndCurrentIndex = -1;
  }
  return self;
}

- (UIView *)createView {
  LynxViewPagerContainer *container = [[LynxViewPagerContainer alloc] init];
  container.delegate = self;
  return container;
}

- (void)updateFrame:(CGRect)frame
            withPadding:(UIEdgeInsets)padding
                 border:(UIEdgeInsets)border
                 margin:(UIEdgeInsets)margin
    withLayoutAnimation:(BOOL)with {
  if (self.frame.size.width != frame.size.width) {
    // width is changed, scroll to currentIndex
    self.viewpagerWidthChangedWithMarkedIndex = self.emitTargetChangesOnlyDuringScroll != -1
                                                    ? self.emitTargetChangesOnlyDuringScroll
                                                    : self.frontEndCurrentIndex;
  }
  [super updateFrame:frame
              withPadding:padding
                   border:border
                   margin:margin
      withLayoutAnimation:with];
  self.shouldInvalidateLayout = YES;
}

- (BOOL)notifyParent {
  return YES;
}

- (void)onNodeReload {
  [super onNodeReload];
  [self setCurrent:0 animated:NO];
}

- (void)layoutDidFinished {
  [super layoutDidFinished];
}

- (void)finishLayoutOperation {
  [super finishLayoutOperation];
  if (self.disableImplicitAnimation) {
    [CATransaction begin];
    [CATransaction setDisableActions:YES];
  }

  self.viewpager.semanticContentAttribute = [self isRtl]
                                                ? UISemanticContentAttributeForceRightToLeft
                                                : UISemanticContentAttributeForceLeftToRight;

  BOOL shouldReloadData = self.shouldReloadData;
  if (self.shouldReloadData) {
    self.shouldReloadData = NO;
    if (self.frontEndCurrentIndex == -1) {
      self.frontEndCurrentIndex = 0;
    }
    [self.viewpager reloadData];
  }

  self.viewpager.clipsToBounds = self.overflow == LynxOverflowHidden;

  if (self.shouldInvalidateLayout) {
    self.shouldInvalidateLayout = NO;
    if (self.view.subviews.lastObject != self.viewpager) {
      [self.view addSubview:self.viewpager];
    }

    // Notice: invoke setFrame on a UICollectionView may change its contentOffset.
    // It is most likely a UIKit's bug. To solve this problem, we record the original value and
    // reset it after setFrame.
    CGPoint preOffset = self.viewpager.contentOffset;
    self.ignoreScrollEvent = YES;
    self.viewpager.frame = self.view.bounds;
    self.viewpager.contentOffset = preOffset;
    self.ignoreScrollEvent = NO;

    [self.viewpager.collectionViewLayout invalidateLayout];
    // reload visible indexPath, because viewpager item's frame may be changed by layout
    NSMutableArray<NSIndexPath *> *reloadPaths = [[NSMutableArray alloc] init];
    for (NSIndexPath *path in [self.viewpager indexPathsForVisibleItems]) {
      if (path.item < [self itemCount]) {
        [reloadPaths addObject:path];
      }
    }

    if (self.notReloadItemsAfterLayout) {
      // `reloadItemsAtIndexPaths` will close the keyboard by default. Call `cellForItem`
      // and `willDisplayCell` manually when layout-only updates should keep the keyboard.
      [reloadPaths enumerateObjectsUsingBlock:^(NSIndexPath *_Nonnull indexPath, NSUInteger idx,
                                                BOOL *_Nonnull stop) {
        LynxViewPagerCell *cell =
            (LynxViewPagerCell *)[self.viewpager cellForItemAtIndexPath:indexPath];
        if (cell) {
          cell.frame = CGRectMake(indexPath.item * CGRectGetWidth(self.viewpager.bounds), 0,
                                  CGRectGetWidth(self.viewpager.bounds),
                                  CGRectGetHeight(self.viewpager.bounds));
          cell.contentView.bounds = cell.bounds;
          if (self.children.count > indexPath.item) {
            UIView *view = ((LynxUI *)(self.children[indexPath.item])).view;
            view.frame = cell.contentView.bounds;
            if (cell.contentView.subviews.firstObject != view) {
              [cell.contentView.subviews.firstObject removeFromSuperview];
            }
            [cell.contentView addSubview:view];
          } else {
            [cell.contentView.subviews.firstObject removeFromSuperview];
          }
        }
      }];
      [reloadPaths enumerateObjectsUsingBlock:^(NSIndexPath *_Nonnull obj, NSUInteger idx,
                                                BOOL *_Nonnull stop) {
        [self collectionView:self.viewpager
               willDisplayCell:[self.viewpager cellForItemAtIndexPath:obj]
            forItemAtIndexPath:obj];
      }];
    } else {
      [self.viewpager reloadItemsAtIndexPaths:reloadPaths];
    }
  }

  if (self.disableImplicitAnimation) {
    [CATransaction commit];
  }

  NSInteger targetIndex = self.viewpagerWidthChangedWithMarkedIndex;

  // This value should be consumed only once, reset it.
  self.viewpagerWidthChangedWithMarkedIndex = -1;

  if (shouldReloadData) {
    targetIndex = MIN(self.indexAfterReload, [self itemCount] - 1);
  }

  // set initial-select-index
  if (self.firstRender && self.initialSelectIndex != kInvalidViewPagerIndex) {
    targetIndex = self.initialSelectIndex;
  }
  self.firstRender = NO;

  if (targetIndex != -1) {
    [self setCurrent:targetIndex animated:NO];
  }
}

- (void)insertChild:(id)child atIndex:(NSInteger)index {
  [super insertChild:child atIndex:index];
  if ([child isKindOfClass:LynxUIViewPagerItem.class]) {
    [[child view] removeFromSuperview];
    self.shouldReloadData = YES;
  }
}

- (void)removeChild:(id)child atIndex:(NSInteger)index {
  [super removeChild:child atIndex:index];
  if ([child isKindOfClass:LynxUIViewPagerItem.class]) {
    self.shouldReloadData = YES;
  }
}

- (BOOL)isScrollContainer {
  return YES;
}

- (LynxUI *)customHitTest:(CGPoint)point withEvent:(UIEvent *)event {
  UIView *view = [self.view hitTest:point withEvent:event];
  if (view == self.view || !view) {
    return nil;
  }

  UIView *targetViewWithUI = view;
  while (view.superview != self.view) {
    view = view.superview;
    if (view.lynxSign) {
      targetViewWithUI = view;
    }
  }
  for (LynxUI *child in self.children) {
    if (child.view == targetViewWithUI) {
      return child;
    }
  }
  return nil;
}

- (id<LynxEventTarget>)hitTest:(CGPoint)point withEvent:(UIEvent *)event {
  LynxUI *guard = nil;

  guard = [self customHitTest:point withEvent:event];
  point = [self.view convertPoint:point toView:guard.view];

  if (!guard) {
    return self;
  }
  return [guard hitTest:point withEvent:event];
}

#pragma mark - UICollectionViewDataSource

- (NSInteger)collectionView:(UICollectionView *)collectionView
     numberOfItemsInSection:(NSInteger)section {
  return [self itemCount];
}

- (__kindof UICollectionViewCell *)collectionView:(UICollectionView *)collectionView
                           cellForItemAtIndexPath:(NSIndexPath *)indexPath {
  LynxViewPagerCell *cell = (LynxViewPagerCell *)[collectionView
      dequeueReusableCellWithReuseIdentifier:NSStringFromClass(LynxViewPagerCell.class)
                                forIndexPath:indexPath];
  cell.contentView.bounds = CGRectMake(0, 0, cell.bounds.size.width, cell.bounds.size.height);
  if (self.children.count > indexPath.item) {
    UIView *view = ((LynxUI *)(self.children[indexPath.item])).view;
    view.frame = cell.contentView.bounds;
    if (cell.contentView.subviews.firstObject != view) {
      [cell.contentView.subviews.firstObject removeFromSuperview];
    }
    [cell.contentView addSubview:view];
  } else {
    [cell.contentView.subviews.firstObject removeFromSuperview];
  }

  return cell;
}

#pragma mark - UICollectionViewDelegateFlowLayout

- (CGSize)collectionView:(UICollectionView *)collectionView
                    layout:(UICollectionViewLayout *)collectionViewLayout
    sizeForItemAtIndexPath:(NSIndexPath *)indexPath {
  return self.viewpager.bounds.size;
}

#pragma mark - UICollectionViewDelegate

- (void)collectionView:(UICollectionView *)collectionView
       willDisplayCell:(UICollectionViewCell *)cell
    forItemAtIndexPath:(NSIndexPath *)indexPath {
  [[self.children objectAtIndex:indexPath.item] resetAnimation];
  [[self.children objectAtIndex:indexPath.item] restartAnimation];
}

- (void)collectionView:(UICollectionView *)collectionView
    didEndDisplayingCell:(UICollectionViewCell *)cell
      forItemAtIndexPath:(NSIndexPath *)indexPath {
  if (self.keepItemView) {
    // attach view to window, to make global exposure work
    UIView *endDisplayingView = cell.contentView.subviews.firstObject;
    [collectionView addSubview:endDisplayingView];
    endDisplayingView.frame = cell.frame;
  }
}

- (void)scrollViewDidEndScrollingAnimation:(UIScrollView *)scrollView {
  if (self.scrollEnable) {
    scrollView.scrollEnabled = YES;
  }
  self.emitTargetChangesOnlyDuringScroll = -1;
  if (self.selectTabCallback) {
    self.selectTabCallback(kUIMethodSuccess, nil);
    self.selectTabCallback = nil;
  }
}

- (void)scrollViewWillBeginDragging:(UIScrollView *)scrollView {
  self.tabWasSelectedBeforeTouch = NO;
  self.emitTargetChangesOnlyDuringScroll = -1;
  if (self.selectTabCallback) {
    self.selectTabCallback(kUIMethodUnknown, @{@"err" : @"selectTab break by touch"});
    self.selectTabCallback = nil;
  }
}

- (void)scrollViewDidEndDragging:(UIScrollView *)scrollView willDecelerate:(BOOL)decelerate {
  if (decelerate && self.singlePageTouch) {
    scrollView.scrollEnabled = NO;
  }
}

- (void)scrollViewDidEndDecelerating:(UIScrollView *)scrollView {
  if (self.singlePageTouch && self.scrollEnable) {
    scrollView.scrollEnabled = YES;
  }
}

- (void)scrollViewWillEndDragging:(UIScrollView *)scrollView
                     withVelocity:(CGPoint)velocity
              targetContentOffset:(inout CGPoint *)targetContentOffset {
  NSInteger willChangeIndex = _currentIndex;

  if (scrollView.bounds.size.width != 0) {
    CGFloat progress = (targetContentOffset->x / scrollView.bounds.size.width);
    NSInteger nextIndex = floor(progress);
    NSInteger ceilNextIndex = ceil(progress);

    if ((progress - nextIndex) * scrollView.bounds.size.width < self.pagerChangeEpsilon) {
      willChangeIndex = nextIndex;
    } else {
      willChangeIndex = ceilNextIndex;
    }
  }

  [self.context.eventEmitter
      sendCustomEvent:[[LynxDetailEvent alloc]
                          initWithName:@"willchange"
                            targetSign:[self sign]
                                detail:@{
                                  @"isDragged" : @(YES),
                                  @"index" :
                                      @([self isRtl] ? ([self itemCount] - 1 - willChangeIndex)
                                                     : willChangeIndex),
                                }]];
}

- (void)scrollViewDidScroll:(UIScrollView *)scrollView {
  // Notify viewpager did scroll.
  [self.context.observer notifyScroll:nil];
  if (self.ignoreScrollEvent) {
    return;
  }
  NSInteger nextIndex = scrollView.bounds.size.width
                            ? (scrollView.contentOffset.x / scrollView.bounds.size.width)
                            : 0;

  if (self.pagerChangeEpsilon <= kLynxViewPagerChangeEpsilonThreshold) {
    if (nextIndex < self.currentIndex) {
      // While scrolling from right to left, `self.currentIndex` will only be changed when the
      // current page is fully displayed. So, we should ceil the `progress`, to get the appropriate
      // value.
      CGFloat progress = (scrollView.contentOffset.x / scrollView.bounds.size.width);
      NSInteger ceilNextIndex = ceil(progress);

      // However, the frame may be a `float` value in Lynx Context, if the FE use `rpx` unit.
      // And we have to deal with the float precision by EPSILON.
      if ((progress - nextIndex) * scrollView.bounds.size.width < self.pagerChangeEpsilon) {
        // If the diff between float value and integer value is small enough, the integer value
        // could be used. Which means we have been at the next page (the left one).
      } else {
        // Otherwise, the ceil value should be the appropriate one. Which means we are still at the
        // previous page (the right one).
        nextIndex = ceilNextIndex;
      }

    } else {
      // Scrolling from left to right, the logic is as same as above, but reversed.
      CGFloat progress = (scrollView.contentOffset.x / scrollView.bounds.size.width);
      NSInteger ceilNextIndex = ceil(progress);
      if ((ceilNextIndex - progress) * scrollView.bounds.size.width < self.pagerChangeEpsilon) {
        // If the diff is small enough, we believe that we have been at the next page (the right
        // one).
        nextIndex = ceilNextIndex;
      } else {
        // Otherwise, we believe that the pager is at the previous page (the left one).
      }
    }
  } else {
    if (nextIndex < self.currentIndex) {
      if (scrollView.contentOffset.x > (self.currentIndex - 1) * scrollView.bounds.size.width) {
        nextIndex = self.currentIndex;
      }
    }
  }

  [self updatePagerEvent:nextIndex
                progress:scrollView.contentOffset.x / scrollView.frame.size.width];
}

#pragma mark - LynxUIViewPagerContainerDelegate
- (void)containerDidLayoutSubviews:(UIView *)container {
}

#pragma mark - LynxUIViewPagerProtocol

- (void)didLayoutSubviews:(UIScrollView *)scrollView {
  if (self.keepItemView) {
    for (NSUInteger i = 0; i < self.children.count; i++) {
      LynxUI *item = self.children[i];
      // attach view to window, to make global exposure work
      if (![item.view.superview.superview isKindOfClass:LynxViewPagerCell.class]) {
        [self.viewpager addSubview:item.view];
        UICollectionViewLayoutAttributes *attr = [self.viewpager
            layoutAttributesForItemAtIndexPath:[NSIndexPath indexPathForItem:i inSection:0]];
        item.view.frame = attr.frame;
      }
    }
  }
}

- (UIView *)lynxView {
  return self.context.rootView;
}

- (BOOL)disableImplicitAnimation {
  return !self.backgroundManager.implicitAnimation;
}

#pragma mark - internal

- (LynxViewPager *)viewpager {
  if (!_viewpager) {
    UICollectionViewFlowLayout *layout = [[UICollectionViewFlowLayout alloc] init];
    layout.minimumLineSpacing = 0;
    layout.minimumInteritemSpacing = 0;
    layout.scrollDirection = UICollectionViewScrollDirectionHorizontal;
    _viewpager = [[LynxViewPager alloc] initWithFrame:CGRectZero collectionViewLayout:layout];
    _viewpager.delegate = self;
    _viewpager.dataSource = self;
    _viewpager.uiDelegate = self;
    [self didCreateViewPager:_viewpager];
  }
  return _viewpager;
}

- (void)didCreateViewPager:(UICollectionView *)viewpager {
}

- (NSUInteger)itemCount {  // todo
  return self.children.count;
}

- (void)setCurrent:(NSUInteger)index animated:(BOOL)animated {
  if (index >= 0 && index < [self itemCount]) {
    self.tabWasSelectedBeforeTouch = YES;
    NSIndexPath *target = [NSIndexPath indexPathForItem:index inSection:0];
    NSInteger realIndex = [self isRtl] ? ([self itemCount] - 1 - index) : index;
    if (ABS(self.viewpager.bounds.size.width * realIndex - self.viewpager.contentOffset.x) <
        self.pagerChangeEpsilon) {
      // @selector(scrollDidScroll:) will not triggered automatically
      [self updatePagerEvent:realIndex progress:index];
      if (animated) {
        [self scrollViewDidEndScrollingAnimation:self.viewpager];
      }
    } else {
      [self.viewpager scrollToItemAtIndexPath:target
                             atScrollPosition:UICollectionViewScrollPositionNone
                                     animated:animated];
    }
  }
}

- (void)updatePagerEvent:(NSUInteger)nextIndex progress:(CGFloat)precent {
  if (nextIndex >= 0 && nextIndex < [self itemCount] && nextIndex != self.currentIndex) {
    // `contentOffset` is a property of UIScrollView, but index and scrolling offset is defined by
    // Lynx. In Lynx's RTL, the `contentOffset` is still all about UI (from left to right), but the
    // index and offset, which are calculated from `contentOffset`, will be reversed.

    self.currentIndex = nextIndex;

    // While in RTL, the concept of `index` of UICollection is reversed, but the `nextIndex` we
    // caculated from `contentOffset` is not. So, it is needed to take the RTL into account here, to
    // merge these two concepts.
    NSInteger emitTargetChangesOnlyDuringScroll =
        [self isRtl] ? ([self itemCount] - 1 - self.emitTargetChangesOnlyDuringScroll)
                     : self.emitTargetChangesOnlyDuringScroll;

    if (self.emitTargetChangesOnlyDuringScroll != -1 &&
        nextIndex != emitTargetChangesOnlyDuringScroll) {
      // Do not emit change event here
    } else {
      NSInteger nextFrontEndCurrentIndex =
          [self isRtl] ? ([self itemCount] - 1 - self.currentIndex) : self.currentIndex;
      if (self.frontEndCurrentIndex != nextFrontEndCurrentIndex) {
        self.frontEndCurrentIndex = nextFrontEndCurrentIndex;
        [self.context.eventEmitter
            sendCustomEvent:[[LynxDetailEvent alloc]
                                initWithName:@"change"
                                  targetSign:[self sign]
                                      detail:@{
                                        @"index" : @(self.frontEndCurrentIndex),
                                        @"isDragged" : @(!self.tabWasSelectedBeforeTouch),
                                      }]];
      }
    }
    [self.pagerDelegate didIndexChanged:nextIndex];
  }

  [self.context.eventEmitter
      sendCustomEvent:[[LynxDetailEvent alloc]
                          initWithName:@"offsetchange"
                            targetSign:[self sign]
                                detail:@{
                                  @"offset" : [NSString
                                      stringWithFormat:@"%.2f", [self isRtl] ? ([self itemCount] -
                                                                                1.0 - precent)
                                                                             : precent]
                                }]];
}

- (NSInteger)centerIndex {
  if (self.viewpager.indexPathsForVisibleItems.count == 1) {
    return self.viewpager.indexPathsForVisibleItems.firstObject.item;
  } else {
    return -1;
  }
}

#pragma mark - LYNX_PROPS

LYNX_PROP_SETTER("bounces", setBounces, BOOL) { self.viewpager.bounces = value; }

LYNX_PROP_SETTER("ios-single-page-touch", setSinglePageTouch, BOOL) {
  self.singlePageTouch = value;
}

LYNX_PROP_SETTER("single-page-touch", setSinglePageTouchProp, BOOL) {
  self.singlePageTouch = value;
}

LYNX_PROP_SETTER("select-index", setSelectIndex, int) { self.indexAfterReload = value; }

LYNX_PROP_SETTER("initial-select-index", setInitialSelectIndex, NSInteger) {
  self.initialSelectIndex = value;
}

LYNX_PROP_SETTER("allow-horizontal-gesture", setHorizontalGesture, BOOL) {
  self.scrollEnable = value;
  self.viewpager.scrollEnabled = self.scrollEnable;
}

LYNX_PROP_SETTER("enable-scroll", setScrollEnable, BOOL) {
  self.scrollEnable = value;
  self.viewpager.scrollEnabled = self.scrollEnable;
  if (!value) {
    self.viewpager.panGestureRecognizer.state = UIGestureRecognizerStateCancelled;
  }
}

LYNX_PROP_SETTER("page-change-animation", setPageChangeAnim, BOOL) {
  self.disablePageChangeAnim = @(value);
}

LYNX_PROP_SETTER("ios-gesture-direction", gestureDirection, int) {
  self.viewpager.attachScroll = value;
}

LYNX_PROP_SETTER("ios-gesture-offset", gestureOffset, CGFloat) {
  self.viewpager.gestureBeginOffset = value;
}

LYNX_PROP_SETTER("keep-item-view", markKeepItemView, BOOL) { self.keepItemView = value; }

#pragma mark - LYNX_UI_METHOD

LYNX_UI_METHOD(selectTab) {
  // update counter
  NSInteger selectTabCounter = self.selectTabCounter;
  self.selectTabCounter = ++selectTabCounter;
  if (self.selectTabCallback) {
    // previous select tab not finished.
    self.emitTargetChangesOnlyDuringScroll = -1;
    self.selectTabCallback(kUIMethodUnknown, @{@"err" : @"break by next selectTab"});
    self.selectTabCallback = nil;
  }
  NSString *err = nil;
  BOOL callbackSuccessNow = NO;

  if ([params objectForKey:@"index"]) {
    NSInteger index = [[params objectForKey:@"index"] intValue];
    BOOL smooth = YES;
    if ([params objectForKey:@"smooth"]) {
      smooth = [[params objectForKey:@"smooth"] boolValue];
    }
    // emit target changes only when it will scroll animated. If it is not, we just keep the
    // `emitTargetChangesOnlyDuringScroll` to be a illegale value.
    if (![[params objectForKey:@"allowTempChanges"] boolValue] && smooth) {
      self.emitTargetChangesOnlyDuringScroll = index;
    } else {
      self.emitTargetChangesOnlyDuringScroll = -1;
    }

    if (index < 0 || index >= [self itemCount]) {
      err = [NSString stringWithFormat:@"index out of range [0, %@)", @([self itemCount])];
      self.emitTargetChangesOnlyDuringScroll = -1;
    } else {
      if (!smooth || ([self centerIndex] == index)) {
        callbackSuccessNow = YES;
        self.emitTargetChangesOnlyDuringScroll = -1;
      } else {
        // set selectTabCallback because XCTest will ignore the animation and call back immediately,
        self.selectTabCallback = callback;

        __weak typeof(self) weakSelf = self;

        // If the system callback not invoked (it is possible when the LynxView is detached from the
        // window), and no other `selectTab` happened, reset `emitTargetChangesOnlyDuringScroll`
        // manually. 500ms is the magic number cause that our scroll should be finished in 300ms.
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(500 * NSEC_PER_MSEC)),
                       dispatch_get_main_queue(), ^{
                         if (weakSelf.selectTabCounter == selectTabCounter) {
                           weakSelf.emitTargetChangesOnlyDuringScroll = -1;
                         }
                       });
      }
      [self.context.eventEmitter
          sendCustomEvent:[[LynxDetailEvent alloc] initWithName:@"willchange"
                                                     targetSign:[self sign]
                                                         detail:@{
                                                           @"isDragged" : @(NO),
                                                           @"index" : @(index),
                                                         }]];
      [self setCurrent:index animated:smooth];
    }
  } else {
    err = @"index is not assigned";
  }

  if (callback) {
    if (err) {
      callback(kUIMethodUnknown, @{@"err" : err});
    } else {
      if (callbackSuccessNow) {
        callback(kUIMethodSuccess, nil);
      }
    }
  }
}

LYNX_UI_METHOD(setDragGesture) {
  if ([params objectForKey:@"canDrag"]) {
    BOOL canDrag = [params[@"canDrag"] boolValue];
    self.viewpager.scrollEnabled = canDrag;
  }
  if (callback) {
    callback(kUIMethodSuccess, nil);
  }
}

LYNX_PROPS_GROUP_DECLARE(
    LYNX_PROP_DECLARE("experimental-pager-change-epsilon", pagerChangeEpsilon, NSNumber *),
    LYNX_PROP_DECLARE("ios-recognized-view-tag", setIosRecognizedViewTag, NSInteger),
    LYNX_PROP_DECLARE("ios-recognized-gesture-class", setIosRecognizedGestureClass, NSString *),
    LYNX_PROP_DECLARE("ios-not-reload-items", setIosNotReloadItems, BOOL))

/**
 * @name: ios-not-reload-items
 * @description: This is customized for containers whose frame changes with the
 * keyboard. If `reloadItemsAtIndexPaths` runs after layout, UIKit will close
 * the keyboard by default. This attribute prevents that behavior.
 * @category: different
 * @standardAction: keep
 * @supportVersion: 2.6.3
 **/
LYNX_PROP_DEFINE("ios-not-reload-items", setIosNotReloadItems, BOOL) {
  self.notReloadItemsAfterLayout = value;
}

/**
 * @name: ios-recognized-gesture-class
 * @description: UIGestureRecognizer's class name, used to identify the
 * UIGestureRecognizer that may be recognized simultaneously with viewpager.
 * This property is designed to let a UIPanGesture work together with viewpager.
 * @category: different
 * @standardAction: keep
 * @supportVersion: 2.9
 **/
LYNX_PROP_DEFINE("ios-recognized-gesture-class", setIosRecognizedGestureClass, NSString *) {
  self.recognizedGestureClass = NSClassFromString(value);
}

/**
 * @name: ios-recognized-view-tag
 * @description: UIView's tag, used to identify the UIView of the
 * UIGestureRecognizer identified by `ios-recognized-gesture-class`.
 * This property is designed to let a UIPanGesture work together with viewpager.
 * @category: different
 * @standardAction: keep
 * @supportVersion: 2.9
 **/
LYNX_PROP_DEFINE("ios-recognized-view-tag", setIosRecognizedViewTag, NSInteger) {
  self.recognizedGestureViewTag = value;
}

/**
 * @name: experimental-pager-change-epsilon
 * @description: On iOS, if we are using 'rpx', their will be some accuracy problem about the
 *comparison of float value, during the calculation of 'bindchange'. This value could adjust the
 *float epsilon to make sure the `bindchange` can be triggered correctly. This experimentation is
 *opened by default, since LynxSDK 2.10.2. The default value is designed to cover all the bad cases.
 *But if it is not, you can set this value to `1.0` to turn off this experimentation or customize
 *your own epsilon. If it is stable, will be offline in the future.
 * @category: different
 * @standardAction: offline
 * @supportVersion: 2.10.2
 * @resolveVersion: 2.12
 **/
LYNX_PROP_DEFINE("experimental-pager-change-epsilon", pagerChangeEpsilon, NSNumber *) {
  self.pagerChangeEpsilon = [value doubleValue];
}

@end
