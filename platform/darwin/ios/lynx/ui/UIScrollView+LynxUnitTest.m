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

- (UIScrollView *)createVerticalSnapScrollViewWithViewportHeight:(CGFloat)viewportHeight
                                                      itemHeight:(CGFloat)itemHeight
                                                       itemCount:(NSInteger)itemCount
                                                   currentOffset:(CGFloat)currentOffset {
  UIScrollView *scrollView =
      [[UIScrollView alloc] initWithFrame:CGRectMake(0, 0, 300, viewportHeight)];
  scrollView.showsVerticalScrollIndicator = NO;
  scrollView.showsHorizontalScrollIndicator = NO;
  scrollView.contentSize = CGSizeMake(300, itemHeight * itemCount);
  scrollView.contentOffset = CGPointMake(0, currentOffset);

  for (NSInteger i = 0; i < itemCount; i++) {
    UIView *item = [[UIView alloc] initWithFrame:CGRectMake(0, i * itemHeight, 300, itemHeight)];
    [scrollView addSubview:item];
  }
  return scrollView;
}

- (CGPoint)targetSnapOffsetForScrollView:(UIScrollView *)scrollView
                                velocity:(CGPoint)velocity
                            maxSnapCount:(NSInteger)maxSnapCount
                        callbackPosition:(NSInteger *)callbackPosition
                          callbackOffset:(CGPoint *)callbackOffset {
  return [scrollView targetContentOffset:CGPointZero
      withScrollingVelocity:velocity
      withVisibleItems:scrollView.subviews
      getIndexFromView:^NSInteger(UIView *_Nonnull view) {
        return [scrollView.subviews indexOfObject:view];
      }
      getViewRectAtIndex:^CGRect(NSInteger index) {
        return scrollView.subviews[index].frame;
      }
      vertical:YES
      rtl:NO
      factor:0
      offset:0
      maxSnapCount:maxSnapCount
      callback:^(NSInteger position, CGPoint offset) {
        if (callbackPosition) {
          *callbackPosition = position;
        }
        if (callbackOffset) {
          *callbackOffset = offset;
        }
      }];
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
      maxSnapCount:1
      callback:^(NSInteger position, CGPoint offset){

      }];
  XCTAssert(targetOffset.y == 100);
}

- (void)testSnapMaxSnapCountKeepsSingleStepByDefault {
  UIScrollView *scrollView = [self createVerticalSnapScrollViewWithViewportHeight:100
                                                                       itemHeight:100
                                                                        itemCount:10
                                                                    currentOffset:10];
  NSInteger callbackPosition = -1;
  CGPoint callbackOffset = CGPointZero;

  // Verify that maxSnapCount = 1 keeps the original single-step snap behavior.
  // Even with a high velocity, it should snap to the first candidate in the scroll direction.
  CGPoint targetOffset = [self targetSnapOffsetForScrollView:scrollView
                                                    velocity:CGPointMake(0, 500)
                                                maxSnapCount:1
                                            callbackPosition:&callbackPosition
                                              callbackOffset:&callbackOffset];

  XCTAssertEqualWithAccuracy(targetOffset.y, 100, 0.001);
  XCTAssertEqual(callbackPosition, 1);
  XCTAssertEqualWithAccuracy(callbackOffset.y, 100, 0.001);
}

- (void)testSnapMaxSnapCountLimitsMultiStepTarget {
  UIScrollView *scrollView = [self createVerticalSnapScrollViewWithViewportHeight:100
                                                                       itemHeight:100
                                                                        itemCount:10
                                                                    currentOffset:10];
  NSInteger callbackPosition = -1;
  CGPoint callbackOffset = CGPointZero;

  // Verify that maxSnapCount > 1 enables multi-step snapping.
  // A high velocity projects farther, but the final target must stay within maxSnapCount.
  CGPoint targetOffset = [self targetSnapOffsetForScrollView:scrollView
                                                    velocity:CGPointMake(0, 500)
                                                maxSnapCount:3
                                            callbackPosition:&callbackPosition
                                              callbackOffset:&callbackOffset];

  XCTAssertEqualWithAccuracy(targetOffset.y, 300, 0.001);
  XCTAssertEqual(callbackPosition, 3);
  XCTAssertEqualWithAccuracy(callbackOffset.y, 300, 0.001);
}

- (void)testSnapExtraDistanceStartsAboveVelocityThreshold {
  UIScrollView *scrollView = [self createVerticalSnapScrollViewWithViewportHeight:100
                                                                       itemHeight:100
                                                                        itemCount:10
                                                                    currentOffset:10];
  NSInteger callbackPosition = -1;
  CGPoint callbackOffset = CGPointZero;

  CGPoint targetOffset = [self targetSnapOffsetForScrollView:scrollView
                                                    velocity:CGPointMake(0, 2)
                                                maxSnapCount:3
                                            callbackPosition:&callbackPosition
                                              callbackOffset:&callbackOffset];

  XCTAssertEqualWithAccuracy(targetOffset.y, 100, 0.001);
  XCTAssertEqual(callbackPosition, 1);

  targetOffset = [self targetSnapOffsetForScrollView:scrollView
                                            velocity:CGPointMake(0, 3)
                                        maxSnapCount:3
                                    callbackPosition:&callbackPosition
                                      callbackOffset:&callbackOffset];

  XCTAssertEqualWithAccuracy(targetOffset.y, 200, 0.001);
  XCTAssertEqual(callbackPosition, 2);
}

- (void)testSnapMaxSnapCountCollapsesBoundaryCandidates {
  UIScrollView *scrollView = [self createVerticalSnapScrollViewWithViewportHeight:250
                                                                       itemHeight:80
                                                                        itemCount:5
                                                                    currentOffset:50];
  NSInteger callbackPosition = -1;
  CGPoint callbackOffset = CGPointZero;

  // Verify that multiple snap offsets clamped to the same endRange collapse into one candidate.
  // Here contentSize=400 and viewport=250, so endRange=150; positions 2/3/4 clamp to 150.
  // Position 2 has the raw offset nearest to the boundary and should be the representative item.
  CGPoint targetOffset = [self targetSnapOffsetForScrollView:scrollView
                                                    velocity:CGPointMake(0, 500)
                                                maxSnapCount:3
                                            callbackPosition:&callbackPosition
                                              callbackOffset:&callbackOffset];

  XCTAssertEqualWithAccuracy(targetOffset.y, 150, 0.001);
  XCTAssertEqual(callbackPosition, 2);
  XCTAssertEqualWithAccuracy(callbackOffset.y, 150, 0.001);
}

@end
