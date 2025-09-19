// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <XCTest/XCTest.h>
#import "LynxCustomGestureRecognizer.h"

@interface LynxCustomGestureRecognizerUnitTest : XCTestCase

@end

@implementation LynxCustomGestureRecognizerUnitTest

- (void)setUp {
  [super setUp];
  // Put setup code here. This method is called before the invocation of each test method in the
  // class.
}

- (void)tearDown {
  // Put teardown code here. This method is called after the invocation of each test method in the
  // class.
  [super tearDown];
}

- (void)testPossibleGesture {
  LynxCustomGestureRecognizer *gestureRecognizer = [[LynxCustomGestureRecognizer alloc] init];
  [gestureRecognizer possibleGesture];
  XCTAssertEqual(gestureRecognizer.state, UIGestureRecognizerStatePossible,
                 @"Gesture state should be Possible");
}

- (void)testBeginGesture {
  LynxCustomGestureRecognizer *gestureRecognizer = [[LynxCustomGestureRecognizer alloc] init];
  [gestureRecognizer beginGesture];
  XCTAssertEqual(gestureRecognizer.state, UIGestureRecognizerStateBegan,
                 @"Gesture state should be Began");
}

- (void)testChangeGesture {
  LynxCustomGestureRecognizer *gestureRecognizer = [[LynxCustomGestureRecognizer alloc] init];
  [gestureRecognizer beginGesture];
  [gestureRecognizer changeGesture];
  XCTAssertEqual(gestureRecognizer.state, UIGestureRecognizerStateChanged,
                 @"Gesture state should be Changed");
}

- (void)testEndGesture {
  LynxCustomGestureRecognizer *gestureRecognizer = [[LynxCustomGestureRecognizer alloc] init];
  [gestureRecognizer endGesture];
  XCTAssertEqual(gestureRecognizer.state, UIGestureRecognizerStateEnded,
                 @"Gesture state should be Ended");
}

- (void)testFailGesture {
  LynxCustomGestureRecognizer *gestureRecognizer = [[LynxCustomGestureRecognizer alloc] init];
  [gestureRecognizer failGesture];
  XCTAssertEqual(gestureRecognizer.state, UIGestureRecognizerStateFailed,
                 @"Gesture state should be Failed");
}

@end
