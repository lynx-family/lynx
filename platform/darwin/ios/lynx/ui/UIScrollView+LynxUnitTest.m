// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#import <Lynx/UIScrollView+Lynx.h>
#import <XCTest/XCTest.h>

@interface UIScrollView_LynxUnitTest : XCTestCase

@end

@implementation UIScrollView_LynxUnitTest

- (void)setUp {
  // Put setup code here. This method is called before the invocation of each test method in the
  // class.
}

- (void)tearDown {
  // Put teardown code here. This method is called after the invocation of each test method in the
  // class.
  sleep(0.8);
}

- (void)testScrollToTargetContentOffsetForbid {
  UIScrollView *scrollView = [[UIScrollView alloc] initWithFrame:CGRectMake(0, 0, 300, 100)];
  [scrollView setContentSize:CGSizeMake(300 * 3, 100)];
  [scrollView scrollToTargetContentOffset:CGPointMake(200, 0)
                                 behavior:LynxScrollViewTouchBehaviorForbid
                                 duration:0.3
                                 interval:0
                                 complete:^BOOL(BOOL scrollEnabledAtStart, BOOL completed) {
                                   XCTAssert(scrollView.contentOffset.x == 200);
                                   return scrollEnabledAtStart;
                                 }];
  XCTAssert(!scrollView.scrollEnabled);
  XCTestExpectation *expectation = [self expectationWithDescription:@"Testing scroll ends"];
  dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 0.5 * NSEC_PER_SEC), dispatch_get_main_queue(), ^{
    [expectation fulfill];
  });
  [self waitForExpectations:@[ expectation ] timeout:3];
  XCTAssert(scrollView.scrollEnabled);
}

- (void)testScrollToTargetContentOffsetStop {
  UIScrollView *scrollView = [[UIScrollView alloc] initWithFrame:CGRectMake(0, 0, 300, 100)];
  [scrollView setContentSize:CGSizeMake(300 * 3, 100)];
  [scrollView scrollToTargetContentOffset:CGPointMake(200, 0)
                                 behavior:LynxScrollViewTouchBehaviorStop
                                 duration:0.3
                                 interval:0
                                 complete:^BOOL(BOOL scrollEnabledAtStart, BOOL completed) {
                                   XCTAssert(scrollView.contentOffset.x == 200);
                                   return scrollEnabledAtStart;
                                 }];
  XCTAssert(scrollView.scrollEnabled);
  XCTestExpectation *expectation = [self expectationWithDescription:@"Testing scroll ends"];
  dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 0.5 * NSEC_PER_SEC), dispatch_get_main_queue(), ^{
    [expectation fulfill];
  });
  [self waitForExpectations:@[ expectation ] timeout:3];
  XCTAssert(scrollView.scrollEnabled);
}

- (void)testSetContentOffsetForbid {
  UIScrollView *scrollView = [[UIScrollView alloc] initWithFrame:CGRectMake(0, 0, 300, 100)];
  [scrollView setContentSize:CGSizeMake(300 * 3, 100)];
  [scrollView setContentOffset:CGPointMake(200, 0)
      behavior:LynxScrollViewTouchBehaviorForbid
      duration:0.3
      interval:0
      progress:^CGPoint(double timeProgress, double distProgress, CGPoint contentOffset) {
        return contentOffset;
      }
      complete:^BOOL(BOOL scrollEnabledAtStart, BOOL completed) {
        XCTAssert(scrollView.contentOffset.x == 200);
        return scrollEnabledAtStart;
      }];
  XCTAssert(!scrollView.scrollEnabled);
  XCTestExpectation *expectation = [self expectationWithDescription:@"Testing scroll ends"];
  dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 0.5 * NSEC_PER_SEC), dispatch_get_main_queue(), ^{
    [expectation fulfill];
  });
  [self waitForExpectations:@[ expectation ] timeout:3];
  XCTAssert(scrollView.scrollEnabled);
}

- (void)testSetContentOffsetStop {
  UIScrollView *scrollView = [[UIScrollView alloc] initWithFrame:CGRectMake(0, 0, 300, 100)];
  [scrollView setContentSize:CGSizeMake(300 * 3, 100)];
  [scrollView setContentOffset:CGPointMake(200, 0)
      behavior:LynxScrollViewTouchBehaviorStop
      duration:0.3
      interval:0
      progress:^CGPoint(double timeProgress, double distProgress, CGPoint contentOffset) {
        return contentOffset;
      }
      complete:^BOOL(BOOL scrollEnabledAtStart, BOOL completed) {
        XCTAssert(scrollView.contentOffset.x == 200);
        return scrollEnabledAtStart;
      }];
  XCTAssert(scrollView.scrollEnabled);
  XCTestExpectation *expectation = [self expectationWithDescription:@"Testing scroll ends"];
  dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 0.5 * NSEC_PER_SEC), dispatch_get_main_queue(), ^{
    [expectation fulfill];
  });
  [self waitForExpectations:@[ expectation ] timeout:3];
  XCTAssert(scrollView.scrollEnabled);
}

- (void)testSnap {
  UIScrollView *scrollView = [[UIScrollView alloc] initWithFrame:CGRectMake(0, 0, 300, 100)];
  scrollView.showsVerticalScrollIndicator = NO;
  scrollView.showsHorizontalScrollIndicator = NO;

  [scrollView setContentSize:CGSizeMake(300, 1000)];
  [scrollView setContentOffset:CGPointMake(0, 10)];

  for (int i = 0; i < 10; i++) {
    [scrollView addSubview:[[UIView alloc] initWithFrame:CGRectMake(0, i * 100, 300, 100)]];
  }
  CGPoint targetOffset = [scrollView targetContentOffset:CGPointMake(0, 0)
      withScrollingVelocity:CGPointMake(0, 500)
      withVisibleItems:@[ scrollView.subviews[0], scrollView.subviews[1] ]
      getIndexFromView:^NSInteger(UIView *_Nonnull view) {
        return [scrollView.subviews indexOfObject:(UIView *)view];
      }
      getViewRectAtIndex:^CGRect(NSInteger index) {
        return scrollView.subviews[index].frame;
      }
      vertical:YES
      rtl:NO
      factor:0
      offset:0
      callback:^(NSInteger position, CGPoint offset){

      }];
  XCTAssert(targetOffset.y == 100);
}

@end
