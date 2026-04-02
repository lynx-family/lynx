// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxBaseScrollView+Internal.h>
#import <Lynx/LynxBaseScrollView+Nested.h>
#import <Lynx/LynxBaseScrollView+Public.h>
#import <Lynx/UIScrollView+Lynx.h>
#import <XCTest/XCTest.h>

@interface MockLynxBaseScrollViewDelegate : NSObject <LynxBaseScrollViewDelegate>
@property(nonatomic, strong) NSArray<UIScrollView *> *hitTestChain;
@end

@implementation MockLynxBaseScrollViewDelegate
- (NSArray<UIScrollView *> *)getHitTestChainForNestedScrollViews {
  return self.hitTestChain;
}
- (void)scrollStateChangedFrom:(LynxBaseScrollViewScrollState)from
                            to:(LynxBaseScrollViewScrollState)to {
}
- (void)scrollViewDidScroll:(UIScrollView *)scrollView {
}
@end

@interface LynxBaseScrollView (Testing)

+ (BOOL)handleNestedScroll:(UIScrollView<LynxNestedScrollProtocol> *)scrollView
                      with:(NSArray<UIScrollView<LynxNestedScrollProtocol> *> *)hittestChain;
- (void)scrollViewWillEndDragging:(UIScrollView *)scrollView
                     withVelocity:(CGPoint)velocity
              targetContentOffset:(inout CGPoint *)targetContentOffset;

@end

@interface TrackingLynxBaseScrollView : LynxBaseScrollView
@property(nonatomic, assign) BOOL stopScrollCalled;
@end

@implementation TrackingLynxBaseScrollView

- (void)stopScroll {
  self.stopScrollCalled = YES;
  [super stopScroll];
}

@end

@interface LynxBaseScrollViewNestedUnitTest : XCTestCase
@property(nonatomic, strong) TrackingLynxBaseScrollView *parentScrollView;
@property(nonatomic, strong) TrackingLynxBaseScrollView *childScrollView;
@property(nonatomic, strong) MockLynxBaseScrollViewDelegate *parentDelegate;
@property(nonatomic, strong) MockLynxBaseScrollViewDelegate *childDelegate;
@end

@implementation LynxBaseScrollViewNestedUnitTest

- (void)setUp {
  _parentScrollView = [[TrackingLynxBaseScrollView alloc] initWithFrame:CGRectMake(0, 0, 200, 200)];
  _parentScrollView.contentSize = CGSizeMake(200, 400);
  _parentScrollView.vertical = YES;

  _childScrollView = [[TrackingLynxBaseScrollView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)];
  _childScrollView.contentSize = CGSizeMake(100, 200);
  _childScrollView.vertical = YES;

  [_parentScrollView addSubview:_childScrollView];

  _parentDelegate = [[MockLynxBaseScrollViewDelegate alloc] init];
  _childDelegate = [[MockLynxBaseScrollViewDelegate alloc] init];

  _parentScrollView.scrollDelegate = _parentDelegate;
  _childScrollView.scrollDelegate = _childDelegate;

  NSArray *chain = @[ _childScrollView, _parentScrollView ];
  _childDelegate.hitTestChain = chain;
  _parentDelegate.hitTestChain = chain;
}

- (void)testGenerateNestedScrollChain {
  UIView *root = [[UIView alloc] initWithFrame:CGRectMake(0, 0, 300, 300)];
  LynxBaseScrollView *parent =
      [[LynxBaseScrollView alloc] initWithFrame:CGRectMake(0, 0, 200, 200)];
  [root addSubview:parent];
  LynxBaseScrollView *child = [[LynxBaseScrollView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)];
  [parent addSubview:child];

  NSArray *chain = [LynxBaseScrollView generateNestedScrollChainWithHitTestTarget:child];
  XCTAssertEqual(chain.count, 2);
  XCTAssertEqual(chain[0], child);
  XCTAssertEqual(chain[1], parent);
}

- (void)testNestedScrollSelfOnly {
  _childScrollView.forwardsNestedScrollMode = NestedScrollModeSelfOnly;
  _childScrollView.backwardsNestedScrollMode = NestedScrollModeSelfOnly;
  _parentScrollView.forwardsNestedScrollMode = NestedScrollModeSelfOnly;
  _parentScrollView.backwardsNestedScrollMode = NestedScrollModeSelfOnly;

  _childScrollView.previousScrollOffset = CGPointMake(0, 0);
  _parentScrollView.previousScrollOffset = CGPointMake(0, 0);

  _childScrollView.contentOffset = CGPointMake(0, 50);
  [_childScrollView tryToHandleNestedScroll];

  XCTAssertEqual(_childScrollView.contentOffset.y, 50);
  XCTAssertEqual(_parentScrollView.contentOffset.y, 0);

  _childScrollView.contentOffset = CGPointMake(0, 100);
  _parentScrollView.contentOffset = CGPointMake(0, 100);
  [_childScrollView tryToHandleNestedScroll];

  XCTAssertEqualWithAccuracy(_childScrollView.contentOffset.y, 100, 0.001);
  XCTAssertEqual(_parentScrollView.contentOffset.y, 0);
}

- (void)testNestedScrollParentFirst {
  _childScrollView.forwardsNestedScrollMode = NestedScrollModeParentFirst;
  _childScrollView.backwardsNestedScrollMode = NestedScrollModeParentFirst;
  _parentScrollView.forwardsNestedScrollMode = NestedScrollModeParentFirst;
  _parentScrollView.backwardsNestedScrollMode = NestedScrollModeParentFirst;

  _childScrollView.previousScrollOffset = CGPointMake(0, 0);
  _parentScrollView.previousScrollOffset = CGPointMake(0, 0);

  _childScrollView.contentOffset = CGPointMake(0, 50);
  _parentScrollView.contentOffset = CGPointMake(0, 50);

  [_childScrollView tryToHandleNestedScroll];

  XCTAssertEqual(_childScrollView.contentOffset.y, 0);
  XCTAssertEqual(_parentScrollView.contentOffset.y, 50);

  _parentScrollView.contentOffset = CGPointMake(0, 200);
  _childScrollView.contentOffset = CGPointMake(0, 50);
  _childScrollView.previousScrollOffset = CGPointMake(0, 0);
  _parentScrollView.previousScrollOffset = CGPointMake(0, 200);

  [_childScrollView tryToHandleNestedScroll];

  XCTAssertEqualWithAccuracy(_parentScrollView.contentOffset.y, 200, 0.001);
  XCTAssertEqualWithAccuracy(_childScrollView.contentOffset.y, 50, 0.001);
}

- (void)testNestedScrollSelfFirst {
  _childScrollView.forwardsNestedScrollMode = NestedScrollModeSelfFirst;
  _childScrollView.backwardsNestedScrollMode = NestedScrollModeSelfFirst;
  _parentScrollView.forwardsNestedScrollMode = NestedScrollModeSelfFirst;
  _parentScrollView.backwardsNestedScrollMode = NestedScrollModeSelfFirst;

  _childScrollView.previousScrollOffset = CGPointMake(0, 0);
  _parentScrollView.previousScrollOffset = CGPointMake(0, 0);

  _childScrollView.contentOffset = CGPointMake(0, 150);
  _parentScrollView.contentOffset = CGPointMake(0, 150);
  [_childScrollView tryToHandleNestedScroll];

  XCTAssertEqualWithAccuracy(_childScrollView.contentOffset.y, 150, 0.001);
  XCTAssertEqualWithAccuracy(_parentScrollView.contentOffset.y, 0, 0.001);
}

- (void)testNestedScrollParallel {
  _childScrollView.forwardsNestedScrollMode = NestedScrollModeParallel;
  _childScrollView.backwardsNestedScrollMode = NestedScrollModeParallel;
  _parentScrollView.forwardsNestedScrollMode = NestedScrollModeParallel;
  _parentScrollView.backwardsNestedScrollMode = NestedScrollModeParallel;

  _childScrollView.previousScrollOffset = CGPointMake(0, 0);
  _parentScrollView.previousScrollOffset = CGPointMake(0, 0);

  _childScrollView.contentOffset = CGPointMake(0, 50);
  [_childScrollView tryToHandleNestedScroll];

  XCTAssertEqual(_childScrollView.contentOffset.y, 50);
  XCTAssertEqual(_parentScrollView.contentOffset.y, 0);

  _childScrollView.contentOffset = CGPointMake(0, 150);
  _parentScrollView.contentOffset = CGPointMake(0, 150);

  [_childScrollView tryToHandleNestedScroll];

  XCTAssertEqualWithAccuracy(_childScrollView.contentOffset.y, 150, 0.001);
  XCTAssertEqualWithAccuracy(_parentScrollView.contentOffset.y, 150, 0.001);
}

- (void)testStopSameDirectionParentFlingForSelfOnlyForward {
  _childScrollView.forwardsNestedScrollMode = NestedScrollModeSelfOnly;
  _childScrollView.backwardsNestedScrollMode = NestedScrollModeParallel;
  _parentScrollView.scrollState = LynxBaseScrollViewScrollStateFling;
  _parentScrollView.previousScrollOffset = CGPointMake(0, 20);
  _parentScrollView.contentOffset = CGPointMake(0, 80);

  _childScrollView.contentOffset = CGPointMake(0, 10);
  CGPoint targetContentOffset = CGPointMake(0, 60);
  [_childScrollView scrollViewWillEndDragging:_childScrollView
                                 withVelocity:CGPointZero
                          targetContentOffset:&targetContentOffset];

  XCTAssertTrue(_parentScrollView.stopScrollCalled);
  XCTAssertEqual([_parentScrollView currentScrollState], LynxBaseScrollViewScrollStateIdle);
}

- (void)testStopOppositeDirectionParentFling {
  _childScrollView.forwardsNestedScrollMode = NestedScrollModeSelfOnly;
  _childScrollView.backwardsNestedScrollMode = NestedScrollModeParallel;
  _parentScrollView.scrollState = LynxBaseScrollViewScrollStateFling;
  _parentScrollView.previousScrollOffset = CGPointMake(0, 80);
  _parentScrollView.contentOffset = CGPointMake(0, 20);

  _childScrollView.contentOffset = CGPointMake(0, 10);
  CGPoint targetContentOffset = CGPointMake(0, 60);
  [_childScrollView scrollViewWillEndDragging:_childScrollView
                                 withVelocity:CGPointZero
                          targetContentOffset:&targetContentOffset];

  XCTAssertTrue(_parentScrollView.stopScrollCalled);
  XCTAssertEqual([_parentScrollView currentScrollState], LynxBaseScrollViewScrollStateIdle);
}

- (void)testStopSameDirectionParentFlingForSelfOnlyWithBounceUsesVelocity {
  _childScrollView.forwardsNestedScrollMode = NestedScrollModeSelfOnly;
  _childScrollView.backwardsNestedScrollMode = NestedScrollModeParallel;
  _parentScrollView.scrollState = LynxBaseScrollViewScrollStateFling;
  _parentScrollView.previousScrollOffset = CGPointMake(0, 20);
  _parentScrollView.contentOffset = CGPointMake(0, 80);

  _childScrollView.contentOffset = CGPointMake(0, 130);
  CGPoint targetContentOffset = CGPointMake(0, 100);
  [_childScrollView scrollViewWillEndDragging:_childScrollView
                                 withVelocity:CGPointMake(0, -500)
                          targetContentOffset:&targetContentOffset];

  XCTAssertTrue(_parentScrollView.stopScrollCalled);
  XCTAssertEqual([_parentScrollView currentScrollState], LynxBaseScrollViewScrollStateIdle);
}

@end
