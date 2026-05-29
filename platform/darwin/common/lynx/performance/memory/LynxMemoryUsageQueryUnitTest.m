// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxMemoryUsageQuery.h>
#import <Lynx/LynxMemoryUsageResult.h>
#import <XCTest/XCTest.h>

@interface LynxMemoryUsageQueryUnitTest : XCTestCase

@end

@implementation LynxMemoryUsageQueryUnitTest

- (void)testQueryAcceptsNilCallback {
  XCTAssertNoThrow([[LynxMemoryUsageQuery sharedInstance] queryLynxGlobalMemoryUsageAsync:nil]);
}

- (void)testQueryReturnsEmptyCompletedStubResultAsynchronously {
  XCTestExpectation *expectation = [self expectationWithDescription:@"memory usage stub callback"];
  __block BOOL callbackCanRunSynchronously = YES;

  [[LynxMemoryUsageQuery sharedInstance]
      queryLynxGlobalMemoryUsageAsync:^(LynxGlobalMemoryUsageResult *result) {
        XCTAssertFalse(callbackCanRunSynchronously);
        XCTAssertNotNil(result);
        XCTAssertGreaterThan(result.collectionStartMs, 0);
        XCTAssertEqual(result.collectionStatus, LynxMemoryCollectionStatusCompleted);
        XCTAssertEqual(result.collectionDurationMs, 0);
        XCTAssertEqual(result.collectionTimeoutMs, 0);
        XCTAssertEqual(result.expectedInstanceCount, 0);
        XCTAssertEqual(result.completedInstanceCount, 0);
        XCTAssertEqual(result.totalBytes, 0);
        XCTAssertEqual(result.appBytes, 0);
        XCTAssertEqual(result.ratioToApp, 0);
        XCTAssertEqual(result.elementBytes, 0);
        XCTAssertEqual(result.elementNodeCount, 0);
        XCTAssertEqual(result.viewBytes, 0);
        XCTAssertEqual(result.mainThreadRuntimeBytes, 0);
        XCTAssertEqual(result.backgroundThreadRuntimeBytes, 0);
        XCTAssertEqual(result.instances.count, 0);
        [expectation fulfill];
      }];

  callbackCanRunSynchronously = NO;
  [self waitForExpectations:@[ expectation ] timeout:1.0];
}

@end
