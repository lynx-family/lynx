// Copyright 2025 The Lynx Authors
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxGestureExtraBundle.h>
#import <XCTest/XCTest.h>

@interface LynxGestureExtraBundleUnitTest : XCTestCase

@end

@implementation LynxGestureExtraBundleUnitTest

- (void)testInitialState {
  LynxGestureExtraBundle *bundle = [[LynxGestureExtraBundle alloc] init];

  XCTAssertEqual(bundle.gestureDirection, 0, "Initial gestureDirection should be 0");
  XCTAssertEqual(bundle.simultaneousDeltaX, 0, "Initial simultaneousDeltaX should be 0");
  XCTAssertEqual(bundle.simultaneousDeltaY, 0, "Initial simultaneousDeltaY should be 0");
  XCTAssertEqual(bundle.isConsumedGesture, NO, "Initial isConsumedGesture should be NO");
}

- (void)testGestureDirection {
  LynxGestureExtraBundle *bundle = [[LynxGestureExtraBundle alloc] init];

  bundle.gestureDirection = 1;
  XCTAssertEqual(bundle.gestureDirection, 1, "gestureDirection should be set to 1");

  bundle.gestureDirection = -1;
  XCTAssertEqual(bundle.gestureDirection, -1, "gestureDirection should be set to -1");
}

- (void)testSimultaneousDeltaX {
  LynxGestureExtraBundle *bundle = [[LynxGestureExtraBundle alloc] init];

  bundle.simultaneousDeltaX = 10.5;
  XCTAssertEqual(bundle.simultaneousDeltaX, 10.5, "simultaneousDeltaX should be set to 10.5");
}

- (void)testSimultaneousDeltaY {
  LynxGestureExtraBundle *bundle = [[LynxGestureExtraBundle alloc] init];

  bundle.simultaneousDeltaY = 20.5;
  XCTAssertEqual(bundle.simultaneousDeltaY, 20.5, "simultaneousDeltaY should be set to 20.5");
}

- (void)testIsConsumedGesture {
  LynxGestureExtraBundle *bundle = [[LynxGestureExtraBundle alloc] init];

  bundle.isConsumedGesture = YES;
  XCTAssertEqual(bundle.isConsumedGesture, YES, "isConsumedGesture should be set to YES");

  bundle.isConsumedGesture = NO;
  XCTAssertEqual(bundle.isConsumedGesture, NO, "isConsumedGesture should be set to NO");
}

- (void)testReset {
  LynxGestureExtraBundle *bundle = [[LynxGestureExtraBundle alloc] init];

  bundle.gestureDirection = 1;
  bundle.simultaneousDeltaX = 15.0;
  bundle.simultaneousDeltaY = 25.0;
  bundle.isConsumedGesture = YES;

  [bundle reset];

  // Reset should not affect gestureDirection
  XCTAssertEqual(bundle.gestureDirection, 1, "gestureDirection should not be reset");
  XCTAssertEqual(bundle.simultaneousDeltaX, 0, "simultaneousDeltaX should be reset to 0");
  XCTAssertEqual(bundle.simultaneousDeltaY, 0, "simultaneousDeltaY should be reset to 0");
  XCTAssertEqual(bundle.isConsumedGesture, NO, "isConsumedGesture should be reset to NO");
}

@end
