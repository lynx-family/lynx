// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxMemoryUsageQuery.h>
#import <XCTest/XCTest.h>

@interface LynxMemoryUsageCollectorUnitTest : XCTestCase

@end

@implementation LynxMemoryUsageCollectorUnitTest

- (void)testQueryReadsGlobalMemoryMonitorAsynchronously {
  XCTestExpectation *expectation = [self expectationWithDescription:@"global memory usage"];
  __block BOOL callbackCanRunSynchronously = YES;

  [[LynxMemoryUsageQuery sharedInstance]
      queryLynxGlobalMemoryUsageAsync:^(LynxGlobalMemoryUsageResult *result) {
        XCTAssertFalse(callbackCanRunSynchronously);
        XCTAssertNotNil(result);
        XCTAssertTrue(result.collectionStatus == LynxMemoryCollectionStatusCompleted ||
                      result.collectionStatus == LynxMemoryCollectionStatusError);
        if (result.collectionStatus == LynxMemoryCollectionStatusError) {
          XCTAssertGreaterThan(result.errorMessage.length, 0);
          XCTAssertEqual(result.instances.count, 0);
          XCTAssertEqual(result.destroyedInstances.count, 0);
        }
        XCTAssertEqual(result.expectedInstanceCount, result.completedInstanceCount);
        XCTAssertEqual(result.totalBytes, result.elementBytes + result.viewBytes +
                                              result.mainThreadRuntimeBytes +
                                              result.backgroundThreadRuntimeBytes);
        XCTAssertEqual(result.ratioToApp,
                       result.appBytes > 0 ? (double)result.totalBytes / result.appBytes : 0.0);
        [expectation fulfill];
      }
                            timeoutMs:20];

  callbackCanRunSynchronously = NO;
  [self waitForExpectations:@[ expectation ] timeout:1.0];
}

@end
