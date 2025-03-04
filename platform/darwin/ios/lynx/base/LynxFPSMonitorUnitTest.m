// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <XCTest/XCTest.h>

#import "LynxFPSMonitor.h"

@interface LynxFPSMonitorUnitTest : XCTestCase

@end

@implementation LynxFPSMonitorUnitTest

- (void)setUp {
}

- (void)tearDown {
}

- (void)testFPSRecordDerivedMetrics {
  LynxFPSRecord* record = [[LynxFPSRecord alloc] init];
  record->_totalMetrics.duration = 10;
  record->_totalMetrics.frames = 500;
  record->_totalMetrics.drop1Count = 20;
  record->_totalMetrics.drop1Duration = 5;
  record->_totalMetrics.drop3Count = 10;
  record->_totalMetrics.drop3Duration = 3;
  record->_totalMetrics.drop7Count = 1;
  record->_totalMetrics.drop7Duration = 1;
  record->_totalMetrics.hitchDuration = 1;
  LynxFPSDerivedMetrics derived = [record derivedMetrics];

  CGFloat delta = 0.000001;
  XCTAssertEqualWithAccuracy(derived.fps, 50, delta);
  XCTAssertEqualWithAccuracy(derived.drop1PerSecond, 2, delta);
  XCTAssertEqualWithAccuracy(derived.drop3PerSecond, 1, delta);
  XCTAssertEqualWithAccuracy(derived.drop7PerSecond, 0.1, delta);
  XCTAssertEqualWithAccuracy(derived.hitchRatio, 0.1, delta);
}

- (void)testFPSMonitor {
  XCTestExpectation* expectation = [self expectationWithDescription:@"return"];
  LynxFPSMonitor* monitor = [[LynxFPSMonitor alloc] init];
  NSString* key = @"specifyKey";
  [monitor beginWithKey:key];

  dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(2 * NSEC_PER_SEC)),
                 dispatch_get_main_queue(), ^{
                   LynxFPSRecord* record = [monitor endWithKey:key];
                   XCTAssertEqual(1, 1);
                   XCTAssertTrue(record.frames > 0);
                   XCTAssertEqualWithAccuracy(record.duration, 2, 0.5);
                   [expectation fulfill];
                 });
  [self waitForExpectationsWithTimeout:10 handler:nil];
}

@end
