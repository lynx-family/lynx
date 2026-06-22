// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#import <Lynx/LynxPropsProcessor.h>
#import <Lynx/LynxUIMethodProcessor.h>
#import <Lynx/LynxUIScroller.h>
#import <XCTest/XCTest.h>
#import "LynxUI+Private.h"
#import "LynxUIScrollerUnitTestUtils.h"

@interface LynxUIScroller (TestBasic)
- (float)scrollLeftLimit;
- (float)scrollRightLimit;
- (float)scrollUpLimit;
- (float)scrollDownLimit;
- (BOOL)canScroll:(ScrollDirection)direction;
- (void)flick:(float)velocity direction:(ScrollDirection)direction;
- (BOOL)isScrollContainer;
- (void)scrollByX:(float)delta;
- (void)scrollByY:(float)delta;
@end

@interface LynxUIScrollerBasicUnitTest : XCTestCase
@end

@implementation LynxUIScrollerBasicUnitTest
// Builds a horizontal scroller but defers onNodeReady so tests can control the ready flush order.
- (LynxUIScroller *)mockHorizontalScrollerForInitialScrollTest:(LynxUIMockContext **)context {
  LynxUIMockContext *mockContext =
      [LynxUIUnitTestUtils initUIMockContextWithUI:[[LynxUIScroller alloc] init]];
  LynxUIScroller *scroller = (LynxUIScroller *)mockContext.mockUI;
  [scroller updateFrame:CGRectMake(0, 0, 428.0f, 100.0f)
              withPadding:UIEdgeInsetsZero
                   border:UIEdgeInsetsZero
      withLayoutAnimation:NO];
  [LynxPropsProcessor updateProp:@YES withKey:@"scroll-x" forUI:scroller];
  [scroller propsDidUpdate];
  [LynxUIScrollerUnitTestUtils mockChildren:10
                                    context:mockContext
                                    scrollY:NO
                                       size:CGSizeMake(100.0f, 100.0f)];
  if (context) {
    *context = mockContext;
  }
  return scroller;
}

- (void)testScrollTopNormal {
  LynxUIMockContext *mockContext =
      [LynxUIScrollerUnitTestUtils updateUIMockContext:nil
                                                  sign:0
                                                   tag:@"scroll-view"
                                              eventSet:[NSSet set]
                                         lepusEventSet:[NSSet set]
                                                 props:@{@"scroll-y" : @YES, @"scroll-top" : @100}];
  LynxUIScroller *scroller = (LynxUIScroller *)[mockContext.UIOwner findUIBySign:0];
  XCTAssert(CGPointEqualToPoint(scroller.view.contentOffset, CGPointMake(0, 100)));
}

- (void)testScrollTopExtremelyLargeValue {
  LynxUIMockContext *mockContext = [LynxUIScrollerUnitTestUtils
      updateUIMockContext:nil
                     sign:0
                      tag:@"scroll-view"
                 eventSet:[NSSet set]
            lepusEventSet:[NSSet set]
                    props:@{@"scroll-y" : @YES, @"scroll-top" : @10000}];
  LynxUIScroller *scroller = (LynxUIScroller *)[mockContext.UIOwner findUIBySign:0];
  CGFloat maxScrollDistance =
      scroller.children.count * scroller.children.lastObject.frame.size.height -
      scroller.view.frame.size.height;
  XCTAssert(CGPointEqualToPoint(scroller.view.contentOffset, CGPointMake(0, maxScrollDistance)));
}

- (void)testScrollTopNegativeValue {
  LynxUIMockContext *mockContext = [LynxUIScrollerUnitTestUtils
      updateUIMockContext:nil
                     sign:0
                      tag:@"scroll-view"
                 eventSet:[NSSet set]
            lepusEventSet:[NSSet set]
                    props:@{@"scroll-y" : @YES, @"scroll-top" : @-100}];
  LynxUIScroller *scroller = (LynxUIScroller *)[mockContext.UIOwner findUIBySign:0];
  XCTAssert(CGPointEqualToPoint(scroller.view.contentOffset, CGPointZero));
}

- (void)testScrollLeftNormal {
  LynxUIMockContext *mockContext = [LynxUIScrollerUnitTestUtils
      updateUIMockContext:nil
                     sign:0
                      tag:@"scroll-view"
                 eventSet:[NSSet set]
            lepusEventSet:[NSSet set]
                    props:@{@"scroll-x" : @YES, @"scroll-left" : @100}];
  LynxUIScroller *scroller = (LynxUIScroller *)[mockContext.UIOwner findUIBySign:0];
  XCTAssert(CGPointEqualToPoint(scroller.view.contentOffset, CGPointMake(100, 0)));
}

- (void)testScrollLeftExtremelyLargeValue {
  LynxUIMockContext *mockContext = [LynxUIScrollerUnitTestUtils
      updateUIMockContext:nil
                     sign:0
                      tag:@"scroll-view"
                 eventSet:[NSSet set]
            lepusEventSet:[NSSet set]
                    props:@{@"scroll-x" : @YES, @"scroll-left" : @10000}];
  LynxUIScroller *scroller = (LynxUIScroller *)[mockContext.UIOwner findUIBySign:0];
  CGFloat maxScrollDistance =
      scroller.children.count * scroller.children.lastObject.frame.size.width -
      scroller.view.frame.size.width;
  XCTAssert(CGPointEqualToPoint(scroller.view.contentOffset, CGPointMake(maxScrollDistance, 0)));
}

- (void)testScrollLeftNegativeValue {
  LynxUIMockContext *mockContext = [LynxUIScrollerUnitTestUtils
      updateUIMockContext:nil
                     sign:0
                      tag:@"scroll-view"
                 eventSet:[NSSet set]
            lepusEventSet:[NSSet set]
                    props:@{@"scroll-x" : @YES, @"scroll-left" : @-100}];
  LynxUIScroller *scroller = (LynxUIScroller *)[mockContext.UIOwner findUIBySign:0];
  XCTAssert(CGPointEqualToPoint(scroller.view.contentOffset, CGPointZero));
}

- (void)testScrollByX {
  LynxUIMockContext *mockContext =
      [LynxUIScrollerUnitTestUtils updateUIMockContext:nil
                                                  sign:0
                                                   tag:@"scroll-view"
                                              eventSet:[NSSet set]
                                         lepusEventSet:[NSSet set]
                                                 props:@{@"scroll-x" : @YES}];
  LynxUIScroller *scroller = (LynxUIScroller *)[mockContext.UIOwner findUIBySign:0];
  XCTAssertFalse([scroller canScroll:SCROLL_LEFT]);
  [scroller scrollByX:300];
  XCTAssertTrue(CGPointEqualToPoint(scroller.view.contentOffset, CGPointMake(300, 0)));
  XCTAssertTrue([scroller canScroll:SCROLL_RIGHT]);
  [scroller scrollByX:1000];
  CGFloat childWidth = scroller.children.firstObject.frame.size.width;
  NSInteger childCount = scroller.children.count;
  XCTAssertTrue(CGPointEqualToPoint(
      scroller.view.contentOffset,
      CGPointMake(childCount * childWidth - scroller.view.frame.size.width, 0)));
  XCTAssertFalse([scroller canScroll:SCROLL_RIGHT]);
}

- (void)testScrollByY {
  LynxUIMockContext *mockContext =
      [LynxUIScrollerUnitTestUtils updateUIMockContext:nil
                                                  sign:0
                                                   tag:@"scroll-view"
                                              eventSet:[NSSet set]
                                         lepusEventSet:[NSSet set]
                                                 props:@{@"scroll-y" : @YES}];
  LynxUIScroller *scroller = (LynxUIScroller *)[mockContext.UIOwner findUIBySign:0];
  XCTAssertFalse([scroller canScroll:SCROLL_UP]);
  [scroller scrollByY:300];
  XCTAssertTrue(CGPointEqualToPoint(scroller.view.contentOffset, CGPointMake(0, 300)));
  XCTAssertTrue([scroller canScroll:SCROLL_DOWN]);
  [scroller scrollByY:1000];
  CGFloat childHeight = scroller.children.firstObject.frame.size.height;
  NSInteger childCount = scroller.children.count;
  XCTAssertTrue(CGPointEqualToPoint(
      scroller.view.contentOffset,
      CGPointMake(0, childCount * childHeight - scroller.view.frame.size.height)));
  XCTAssertFalse([scroller canScroll:SCROLL_DOWN]);
}

- (void)testAutoScroll {
  LynxUIMockContext *mockContext =
      [LynxUIScrollerUnitTestUtils updateUIMockContext:nil
                                                  sign:0
                                                   tag:@"scroll-view"
                                              eventSet:[NSSet set]
                                         lepusEventSet:[NSSet set]
                                                 props:@{@"scroll-x" : @YES}];
  LynxUIScroller *scroller = (LynxUIScroller *)[mockContext.UIOwner findUIBySign:0];
  LynxUIMethodCallbackBlock callBack = ^(int code, id _Nullable data) {
  };

  [LynxUIMethodProcessor invokeMethod:@"autoScroll"
                           withParams:@{@"start" : @YES, @"rate" : @0}
                           withResult:callBack
                                forUI:scroller];
  XCTestExpectation *expectationNotScroll =
      [self expectationWithDescription:@"Testing auto scroll works correctly"];
  dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 1 * NSEC_PER_SEC), dispatch_get_main_queue(), ^{
    [expectationNotScroll fulfill];
  });
  [self waitForExpectations:@[ expectationNotScroll ] timeout:3];
  XCTAssert(scroller.view.contentOffset.x == 0);

  [LynxUIMethodProcessor invokeMethod:@"autoScroll"
                           withParams:@{@"start" : @YES, @"rate" : @60}
                           withResult:callBack
                                forUI:scroller];
  XCTestExpectation *expectation =
      [self expectationWithDescription:@"Testing auto scroll works correctly"];
  dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 1 * NSEC_PER_SEC), dispatch_get_main_queue(), ^{
    [expectation fulfill];
  });
  [self waitForExpectations:@[ expectation ] timeout:3];
  XCTAssert(scroller.view.contentOffset.x > 0);

  [LynxUIMethodProcessor invokeMethod:@"autoScroll"
                           withParams:@{@"start" : @YES, @"rate" : @2000}
                           withResult:callBack
                                forUI:scroller];
  XCTestExpectation *expectationReachEnd =
      [self expectationWithDescription:@"Testing auto scroll hit the border and stop"];
  dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 2 * NSEC_PER_SEC), dispatch_get_main_queue(), ^{
    [expectationReachEnd fulfill];
  });
  [self waitForExpectations:@[ expectationReachEnd ] timeout:3];
  CGFloat childWidth = scroller.children.lastObject.frame.size.width;
  NSInteger childCount = scroller.children.count;
  XCTAssert(scroller.view.contentOffset.x ==
            childWidth * childCount - scroller.view.frame.size.width);
}

- (void)testAutoScrollScrollY {
  LynxUIMockContext *mockContext =
      [LynxUIScrollerUnitTestUtils updateUIMockContext:nil
                                                  sign:0
                                                   tag:@"scroll-view"
                                              eventSet:[NSSet set]
                                         lepusEventSet:[NSSet set]
                                                 props:@{@"scroll-y" : @YES}];
  LynxUIScroller *scroller = (LynxUIScroller *)[mockContext.UIOwner findUIBySign:0];
  LynxUIMethodCallbackBlock callBack = ^(int code, id _Nullable data) {
  };
  [LynxUIMethodProcessor invokeMethod:@"autoScroll"
                           withParams:@{@"start" : @YES, @"rate" : @0}
                           withResult:callBack
                                forUI:scroller];
  XCTestExpectation *expectationNotScroll =
      [self expectationWithDescription:@"Testing auto scroll with 0 rate failed"];
  dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 1 * NSEC_PER_SEC), dispatch_get_main_queue(), ^{
    [expectationNotScroll fulfill];
  });
  [self waitForExpectations:@[ expectationNotScroll ] timeout:3];
  XCTAssert(scroller.view.contentOffset.y == 0);

  [LynxUIMethodProcessor invokeMethod:@"autoScroll"
                           withParams:@{@"start" : @YES, @"rate" : @60}
                           withResult:callBack
                                forUI:scroller];
  XCTestExpectation *expectation =
      [self expectationWithDescription:@"Testing auto scroll works correctly"];
  dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 1 * NSEC_PER_SEC), dispatch_get_main_queue(), ^{
    [expectation fulfill];
  });
  [self waitForExpectations:@[ expectation ] timeout:3];
  XCTAssert(scroller.view.contentOffset.y > 0);

  [LynxUIMethodProcessor invokeMethod:@"autoScroll"
                           withParams:@{@"start" : @YES, @"rate" : @2000}
                           withResult:callBack
                                forUI:scroller];
  XCTestExpectation *expectationReachEnd =
      [self expectationWithDescription:@"Testing auto scroll hit the border and stop"];
  dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 2 * NSEC_PER_SEC), dispatch_get_main_queue(), ^{
    [expectationReachEnd fulfill];
  });
  [self waitForExpectations:@[ expectationReachEnd ] timeout:3];
  CGFloat childHeight = scroller.children.lastObject.frame.size.height;
  NSInteger childCount = scroller.children.count;
  XCTAssert(scroller.view.contentOffset.y ==
            childHeight * childCount - scroller.view.frame.size.height);
}

- (void)testAutoScrollScrollXRTL {
  LynxUIMockContext *mockContext =
      [LynxUIScrollerUnitTestUtils updateUIMockContext:nil
                                                  sign:0
                                                   tag:@"scroll-view"
                                              eventSet:[NSSet set]
                                         lepusEventSet:[NSSet set]
                                                 props:@{@"scroll-x" : @YES}];
  LynxUIScroller *scroller = (LynxUIScroller *)[mockContext.UIOwner findUIBySign:0];
  [LynxPropsProcessor updateProp:@2 withKey:@"direction" forUI:scroller];
  [scroller propsDidUpdate];
  [scroller onNodeReadyForUIOwner];
  NSInteger childCount = scroller.children.count;
  NSInteger childWidth = scroller.children.lastObject.frame.size.width;
  CGFloat maxScrollDistance = childCount * childWidth - scroller.view.frame.size.width;
  XCTAssertTrue(
      CGPointEqualToPoint(scroller.view.contentOffset, CGPointMake(maxScrollDistance, 0)));
  LynxUIMethodCallbackBlock callBack = ^(int code, id _Nullable data) {
  };

  [LynxUIMethodProcessor invokeMethod:@"autoScroll"
                           withParams:@{@"start" : @YES, @"rate" : @0}
                           withResult:callBack
                                forUI:scroller];
  XCTestExpectation *expectationNotScroll =
      [self expectationWithDescription:@"Testing auto scroll works correctly"];
  dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 1 * NSEC_PER_SEC), dispatch_get_main_queue(), ^{
    [expectationNotScroll fulfill];
  });
  [self waitForExpectations:@[ expectationNotScroll ] timeout:3];
  XCTAssert(scroller.view.contentOffset.x == maxScrollDistance);

  [LynxUIMethodProcessor invokeMethod:@"autoScroll"
                           withParams:@{@"start" : @YES, @"rate" : @60}
                           withResult:callBack
                                forUI:scroller];
  XCTestExpectation *expectation =
      [self expectationWithDescription:@"Testing auto scroll works correctly"];
  dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 1 * NSEC_PER_SEC), dispatch_get_main_queue(), ^{
    [expectation fulfill];
  });
  [self waitForExpectations:@[ expectation ] timeout:3];
  XCTAssert(scroller.view.contentOffset.x <
            childCount * childWidth - scroller.view.frame.size.width);
}

- (void)testFlick {
  LynxUIMockContext *mockContext =
      [LynxUIScrollerUnitTestUtils updateUIMockContext:nil
                                                  sign:0
                                                   tag:@"scroll-view"
                                              eventSet:[NSSet set]
                                         lepusEventSet:[NSSet set]
                                                 props:@{@"scroll-x" : @YES}];
  LynxUIScroller *scroller = (LynxUIScroller *)[mockContext.UIOwner findUIBySign:0];
  [scroller flick:10 direction:SCROLL_RIGHT];
  XCTestExpectation *expectation =
      [self expectationWithDescription:@"Testing flick works correctly"];
  dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 1 * NSEC_PER_SEC), dispatch_get_main_queue(), ^{
    [expectation fulfill];
  });
  [self waitForExpectations:@[ expectation ] timeout:3];
  XCTAssert(scroller.view.contentOffset.x > 0);
}

- (void)testRTL {
  LynxUIMockContext *mockContext =
      [LynxUIScrollerUnitTestUtils updateUIMockContext:nil
                                                  sign:0
                                                   tag:@"scroll-view"
                                              eventSet:[NSSet set]
                                         lepusEventSet:[NSSet set]
                                                 props:@{@"scroll-x" : @YES}];
  LynxUIScroller *scroller = (LynxUIScroller *)[mockContext.UIOwner findUIBySign:0];
  scroller.view.contentOffset = CGPointMake(100, 0);
  XCTAssert(CGPointEqualToPoint(scroller.view.contentOffset, CGPointMake(100, 0)));
  // Set it dynamically after initialization
  [LynxPropsProcessor updateProp:@2 withKey:@"direction" forUI:scroller];
  [scroller propsDidUpdate];
  [scroller onNodeReadyForUIOwner];
  // children's total width - scrollView's width
  XCTAssert(CGPointEqualToPoint(scroller.view.contentOffset, CGPointMake(10 * 100 - 428, 0)));
}

// Validates that keyed ready slots make final initial-scroll-to-index independent of prop order.
- (void)testInitialScrollToIndexWinsWhenSetBeforeDirection {
  LynxUIMockContext *mockContext;
  LynxUIScroller *scroller = [self mockHorizontalScrollerForInitialScrollTest:&mockContext];

  [LynxPropsProcessor updateProp:@3 withKey:@"initial-scroll-to-index" forUI:scroller];
  [LynxPropsProcessor updateProp:@2 withKey:@"direction" forUI:scroller];
  [scroller propsDidUpdate];
  [scroller onNodeReadyForUIOwner];

  XCTAssertTrue(CGPointEqualToPoint(scroller.view.contentOffset, CGPointMake(300.0f, 0.0f)));
  XCTAssertTrue(((UIScrollView *)scroller.view).isRTL);
  XCTAssertEqual(mockContext.mockUI, scroller);
}

// Covers the inverse prop order to prove onNodeReady consumes the slots in fixed priority order.
- (void)testInitialScrollToIndexWinsWhenSetAfterDirection {
  LynxUIMockContext *mockContext;
  LynxUIScroller *scroller = [self mockHorizontalScrollerForInitialScrollTest:&mockContext];

  [LynxPropsProcessor updateProp:@2 withKey:@"direction" forUI:scroller];
  [LynxPropsProcessor updateProp:@3 withKey:@"initial-scroll-to-index" forUI:scroller];
  [scroller propsDidUpdate];
  [scroller onNodeReadyForUIOwner];

  XCTAssertTrue(CGPointEqualToPoint(scroller.view.contentOffset, CGPointMake(300.0f, 0.0f)));
  XCTAssertTrue(((UIScrollView *)scroller.view).isRTL);
  XCTAssertEqual(mockContext.mockUI, scroller);
}

// Confirms initial-scroll-to-index has higher priority than initial-scroll-offset at ready time.
- (void)testInitialScrollToIndexOverridesInitialScrollOffset {
  LynxUIMockContext *mockContext;
  LynxUIScroller *scroller = [self mockHorizontalScrollerForInitialScrollTest:&mockContext];

  [LynxPropsProcessor updateProp:@4 withKey:@"initial-scroll-to-index" forUI:scroller];
  [LynxPropsProcessor updateProp:@200 withKey:@"initial-scroll-offset" forUI:scroller];
  [scroller propsDidUpdate];
  [scroller onNodeReadyForUIOwner];

  XCTAssertTrue(CGPointEqualToPoint(scroller.view.contentOffset, CGPointMake(400.0f, 0.0f)));
  XCTAssertEqual(mockContext.mockUI, scroller);
}

// Ensures resetting direction removes the stale ready block so an earlier rtl write does not leak.
- (void)testDirectionResetClearsStaleReadyBlock {
  LynxUIMockContext *mockContext;
  LynxUIScroller *scroller = [self mockHorizontalScrollerForInitialScrollTest:&mockContext];

  [LynxPropsProcessor updateProp:@2 withKey:@"direction" forUI:scroller];
  [LynxPropsProcessor updateProp:[NSNull null] withKey:@"direction" forUI:scroller];
  [scroller propsDidUpdate];
  [scroller onNodeReadyForUIOwner];

  XCTAssertTrue(CGPointEqualToPoint(scroller.view.contentOffset, CGPointZero));
  XCTAssertFalse(((UIScrollView *)scroller.view).isRTL);
  XCTAssertEqual(mockContext.mockUI, scroller);
}

#pragma mark props setting or judge functions
- (void)testSetScrollBarEnable {
  LynxUIScroller *scroller = [[LynxUIScroller alloc] init];
  [LynxPropsProcessor updateProp:@1 withKey:@"scroll-bar-enable" forUI:scroller];
  [scroller propsDidUpdate];
  XCTAssertTrue(scroller.view.showsVerticalScrollIndicator);
  XCTAssertTrue(scroller.view.showsHorizontalScrollIndicator);
}

- (void)testIsContainer {
  LynxUIScroller *scroller = [[LynxUIScroller alloc] init];
  XCTAssertTrue([scroller isScrollContainer]);
}

- (void)testBounces {
  LynxUIScroller *scroller = [[LynxUIScroller alloc] init];
  [LynxPropsProcessor updateProp:@1 withKey:@"bounces" forUI:scroller];
  [scroller propsDidUpdate];
  XCTAssertTrue(scroller.view.bounces);
}

- (void)testScrollEnabled {
  LynxUIScroller *scroller = [[LynxUIScroller alloc] init];
  [LynxPropsProcessor updateProp:@1 withKey:@"enable-scroll" forUI:scroller];
  [scroller propsDidUpdate];
  XCTAssertTrue(scroller.view.scrollEnabled);
}

@end
