// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <XCTest/XCTest.h>
#import "LynxGestureFlingTrigger.h"

@interface LynxGestureFlingTrigger (UnitTest)

@property(readonly, nonatomic, strong) CADisplayLink *displayLink;

- (void)tick:(CADisplayLink *)displayLink;

@end

@interface LynxGestureFlingTriggerUnitTest : XCTestCase

@property(nonatomic, strong) LynxGestureFlingTrigger *trigger;

@end

@implementation LynxGestureFlingTriggerUnitTest

- (void)setUp {
  // Put setup code here. This method is called before the invocation of each test method in the
  // class.
  _trigger = [[LynxGestureFlingTrigger alloc] initWithTarget:self action:@selector(handleFling:)];
}

- (void)tearDown {
  // Put teardown code here. This method is called after the invocation of each test method in the
  // class.
}

- (void)testFlingState {
  XCTAssertFalse([_trigger startWithVelocity:CGPointMake(0, 0)]);
  XCTAssertFalse([_trigger startWithVelocity:CGPointMake(200, 0)]);
  XCTAssertFalse([_trigger startWithVelocity:CGPointMake(200, 200)]);

  XCTAssertTrue([_trigger startWithVelocity:CGPointMake(301, 200)]);

  XCTAssertTrue(_trigger.state == LynxGestureFlingTriggerStateStart);

  [_trigger tick:_trigger.displayLink];

  XCTAssertTrue(_trigger.state == LynxGestureFlingTriggerStateUpdate);

  [_trigger stop];

  [_trigger reset];
  XCTAssertTrue(_trigger.state == LynxGestureFlingTriggerStateEnd);
}

- (void)handleFling:(LynxGestureFlingTrigger *)sender {
}

@end
