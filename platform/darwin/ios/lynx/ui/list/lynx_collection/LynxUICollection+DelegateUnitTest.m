// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <XCTest/XCTest.h>
#import "LynxPropsProcessor.h"
#import "LynxUI+Gesture.h"
#import "LynxUICollection+Delegate.h"
#import "LynxUICollection+Internal.h"
#import "LynxUICollection.h"
#import "LynxUIMethodProcessor.h"
#import "LynxVersion.h"
@interface LynxUICollection (Test)
- (void)autoScroll:(NSDictionary *)params withResult:(LynxUIMethodCallbackBlock)callback;
@end

@interface LynxUICollectionDelegateUnitTest : XCTestCase

@end
@implementation LynxUICollectionDelegateUnitTest

- (void)setUp {
}

- (LynxUICollection *)setUpList {
  LynxUICollection *list = [[LynxUICollection alloc] init];
  list = [[LynxUICollection alloc] init];
  [list updateFrame:UIScreen.mainScreen.bounds
              withPadding:UIEdgeInsetsZero
                   border:UIEdgeInsetsZero
      withLayoutAnimation:NO];
  [list.view setContentSize:CGSizeMake(UIScreen.mainScreen.bounds.size.width,
                                       UIScreen.mainScreen.bounds.size.height * 5)];
  return list;
}

- (void)tearDown {
  // Make all parallel test cases serial, because `auto-scroll` is driven by CADisplayLink, which
  // can not be parallelly excuted.
  sleep(3.2);
}

- (void)testAutoScroll {
  LynxUICollection *list = [self setUpList];
  XCTAssertNotNil(list.view);
  CGFloat originY = list.view.contentOffset.y;
  [list autoScroll:@{@"rate" : @"10px", @"start" : @(YES), @"autoStop" : @(YES)}
        withResult:^(int code, id _Nullable data) {
          XCTAssert(code == 0);
        }];
  XCTestExpectation *expectation =
      [self expectationWithDescription:@"Testing auto scroll works correctly"];
  dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 1 * NSEC_PER_SEC), dispatch_get_main_queue(), ^{
    [expectation fulfill];
  });
  [self waitForExpectations:@[ expectation ] timeout:3];
  XCTAssert(list.view.contentOffset.y > originY);
  [list autoScroll:@{@"rate" : @"10px", @"start" : @(NO), @"autoStop" : @(NO)}
        withResult:^(int code, id _Nullable data) {
          XCTAssert(code == 0);
        }];
}

- (void)testCantAutoScrollToLower {
  LynxUICollection *list = [self setUpList];
  [list.view setContentSize:CGSizeMake(UIScreen.mainScreen.bounds.size.width, 100)];
  XCTAssertNotNil(list.view);
  [list autoScroll:@{@"rate" : @"10px", @"start" : @(YES), @"autoStop" : @(YES)}
        withResult:^(int code, id _Nullable data) {
          XCTAssert(code == 0);
        }];
  XCTestExpectation *expectation =
      [self expectationWithDescription:@"Testing auto scroll works correctly"];
  dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 1.5f * NSEC_PER_SEC), dispatch_get_main_queue(),
                 ^{
                   [expectation fulfill];
                 });
  [self waitForExpectations:@[ expectation ] timeout:3];
  XCTAssert(list.view.contentOffset.y == 0);
}

- (void)testCantAutoScrollToRight {
  LynxUICollection *list = [self setUpList];
  [list.view setContentSize:CGSizeMake(100, UIScreen.mainScreen.bounds.size.height)];
  list.layoutOrientation = LynxListLayoutOrientationHorizontal;
  XCTAssertNotNil(list.view);
  [list autoScroll:@{@"rate" : @"10px", @"start" : @(YES), @"autoStop" : @(YES)}
        withResult:^(int code, id _Nullable data) {
          XCTAssert(code == 0);
        }];
  XCTestExpectation *expectation =
      [self expectationWithDescription:@"Testing auto scroll works correctly"];
  dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 1.5f * NSEC_PER_SEC), dispatch_get_main_queue(),
                 ^{
                   [expectation fulfill];
                 });
  [self waitForExpectations:@[ expectation ] timeout:3];
  XCTAssert(list.view.contentOffset.x == 0);
}

- (void)testAutoScrollAndStop {
  LynxUICollection *list = [self setUpList];
  XCTAssertNotNil(list.view);
  [list autoScroll:@{@"rate" : @"10px", @"start" : @(YES), @"autoStop" : @(NO)}
        withResult:^(int code, id _Nullable data) {
          XCTAssert(code == 0);
        }];
  XCTestExpectation *expectation =
      [self expectationWithDescription:@"Testing auto scroll and stop works correctly"];
  dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 1 * NSEC_PER_SEC), dispatch_get_main_queue(), ^{
    [list autoScroll:@{
      @"rate" : @"10px",
      @"start" : @(NO),
      @"autoStop" : @(NO)
    }
        withResult:^(int code, id _Nullable data) {
          XCTAssert(code == 0);
          dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 1 * NSEC_PER_SEC),
                         dispatch_get_main_queue(), ^{
                           [expectation fulfill];
                         });
        }];
  });
  [self waitForExpectations:@[ expectation ] timeout:3];
  XCTAssert(list.view.contentOffset.y < 20);
}

- (void)testAutoScrollFailure {
  LynxUICollection *list = [self setUpList];
  XCTAssertNotNil(list.view);
  [list autoScroll:@{@"rate" : @"0.1px", @"start" : @(YES), @"autoStop" : @(YES)}
        withResult:^(int code, id _Nullable data) {
          XCTAssert(code > 0);
        }];
}

- (void)testAutoScrollToLower {
  LynxUICollection *list = [self setUpList];
  XCTAssertNotNil(list.view);
  [list.view setContentOffset:CGPointMake(0, 10)];
  [list autoScroll:@{@"rate" : @"-10px", @"start" : @(YES), @"autoStop" : @(YES)}
        withResult:^(int code, id _Nullable data) {
          XCTAssert(code == 0);
        }];
  XCTestExpectation *expectation =
      [self expectationWithDescription:@"Testing auto scroll works correctly"];
  dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 1.5f * NSEC_PER_SEC), dispatch_get_main_queue(),
                 ^{
                   [expectation fulfill];
                 });
  [self waitForExpectations:@[ expectation ] timeout:3];
  XCTAssert(list.view.contentOffset.y == 0);
}

- (void)testScrollBy {
  LynxUICollection *list = [self setUpList];
  XCTAssertNotNil(list.view);
  [list.view setContentSize:CGSizeMake(UIScreen.mainScreen.bounds.size.width * 5,
                                       UIScreen.mainScreen.bounds.size.height * 5)];
  XCTestExpectation *expectation =
      [self expectationWithDescription:@"Testing scroll works correctly"];
  dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 2 * NSEC_PER_SEC), dispatch_get_main_queue(), ^{
    [expectation fulfill];
  });
  [self waitForExpectations:@[ expectation ] timeout:3];
  [list setContentOffset:CGPointMake(25, 30)];
  XCTAssert(list.view.contentOffset.x == 25);
  XCTAssert(list.view.contentOffset.y == 30);
  [list setContentOffset:CGPointMake(-25, -30)];
  XCTAssert(list.view.contentOffset.x == 0);
  XCTAssert(list.view.contentOffset.y == 0);
  [list setContentOffset:CGPointMake(25000, 30000)];
  XCTAssert(list.view.contentOffset.x ==
            UIScreen.mainScreen.bounds.size.width * 5 - list.view.frame.size.width);
  XCTAssert(list.view.contentOffset.y ==
            UIScreen.mainScreen.bounds.size.height * 5 - list.view.frame.size.height);
  XCTestExpectation *expectation2 =
      [self expectationWithDescription:@"Testing scroll works correctly"];
  dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 1 * NSEC_PER_SEC), dispatch_get_main_queue(), ^{
    [expectation2 fulfill];
  });
  [self waitForExpectations:@[ expectation2 ] timeout:3];
}

- (void)testGestureInterfaces {
  LynxUICollection *list = [self setUpList];
  XCTAssertNotNil(list.view);

  XCTAssertFalse([list canConsumeGesture:CGPointMake(10, -1)]);

  [list onGestureScrollBy:CGPointMake(10, 10)];
  XCTAssertTrue(list.view.contentOffset.y == 10);
  XCTAssertTrue(list.view.contentOffset.x == 0);
  XCTAssertTrue([list canConsumeGesture:CGPointMake(10, 10)]);
  XCTAssertTrue([list getMemberScrollX] == 0.0f);
  XCTAssertTrue([list getMemberScrollY] == 10.0f);

  [list onGestureScrollBy:CGPointMake(10, 10000)];
  XCTAssertFalse([list canConsumeGesture:CGPointMake(10, 1)]);
}

- (void)testSnap {
  LynxUICollection *list = [self setUpList];
  XCTAssertNotNil(list.view);
  [LynxPropsProcessor updateProp:@{
    @"factor" : @(0),
    @"offset" : @(20),
  }
                         withKey:@"item-snap"
                           forUI:list];
  XCTAssertTrue(list.pagingAlignFactor == 0);
  XCTAssertTrue(list.pagingAlignOffset == 20);
  [LynxPropsProcessor updateProp:@{
    @"factor" : @(-1),
    @"align" : @(20),
  }
                         withKey:@"item-snap"
                           forUI:list];
}

@end
