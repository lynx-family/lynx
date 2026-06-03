// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxMemoryUsageResult.h>
#import <XCTest/XCTest.h>

#import "LynxGlobalMemoryUsageCollectionContext.h"

@interface LynxGlobalMemoryUsageCollectionContextUnitTest : XCTestCase

@end

@implementation LynxGlobalMemoryUsageCollectionContextUnitTest

- (void)testContextTracksFetcherResultsAndCompletion {
  __block NSInteger callbackCount = 0;
  LynxGlobalMemoryUsageCollectionContext *context = [[LynxGlobalMemoryUsageCollectionContext alloc]
       initWithCollectionId:7
          collectionStartMs:100
        collectionTimeoutMs:2000
      expectedInstanceCount:2
                   callback:^(LynxGlobalMemoryUsageResult *result) {
                     callbackCount++;
                   }];
  LynxInstanceMemoryUsage *instance = [[LynxInstanceMemoryUsage alloc] init];

  XCTAssertTrue([context matchesCollectionId:7]);
  XCTAssertFalse([context matchesCollectionId:8]);
  XCTAssertEqual(context.collectionId, 7);
  XCTAssertEqual(context.collectionStartMs, 100);
  XCTAssertEqual(context.collectionTimeoutMs, 2000);
  XCTAssertEqual(context.expectedInstanceCount, 2);
  XCTAssertEqual(context.receivedFetchResultCount, 0);
  XCTAssertEqual(context.instances.count, 0);
  XCTAssertFalse([context hasReceivedAllFetchResults]);

  [context recordInstanceResult:nil];
  XCTAssertEqual(context.receivedFetchResultCount, 1);
  XCTAssertEqual(context.instances.count, 0);
  XCTAssertFalse([context hasReceivedAllFetchResults]);

  [context recordInstanceResult:instance];
  XCTAssertEqual(context.receivedFetchResultCount, 2);
  XCTAssertEqual(context.instances.count, 1);
  XCTAssertTrue(context.instances[0] == instance);
  XCTAssertTrue([context hasReceivedAllFetchResults]);

  for (id callbackObject in [context copyCallbacksForCompletion]) {
    LynxGlobalMemoryUsageCallback callback = callbackObject;
    callback([[LynxGlobalMemoryUsageResult alloc] init]);
  }
  XCTAssertEqual(callbackCount, 1);
}

- (void)testContextCoalescesCallbacksIntoCompletionSnapshot {
  __block NSInteger firstCallbackCount = 0;
  __block NSInteger secondCallbackCount = 0;
  __block NSInteger thirdCallbackCount = 0;
  LynxGlobalMemoryUsageCollectionContext *context = [[LynxGlobalMemoryUsageCollectionContext alloc]
       initWithCollectionId:1
          collectionStartMs:10
        collectionTimeoutMs:2000
      expectedInstanceCount:1
                   callback:^(LynxGlobalMemoryUsageResult *result) {
                     firstCallbackCount++;
                   }];
  [context addCallback:^(LynxGlobalMemoryUsageResult *result) {
    secondCallbackCount++;
  }];
  NSArray<id> *callbacks = [context copyCallbacksForCompletion];
  [context addCallback:^(LynxGlobalMemoryUsageResult *result) {
    thirdCallbackCount++;
  }];

  LynxGlobalMemoryUsageResult *result = [[LynxGlobalMemoryUsageResult alloc] init];
  for (id callbackObject in callbacks) {
    LynxGlobalMemoryUsageCallback callback = callbackObject;
    callback(result);
  }

  XCTAssertEqual(callbacks.count, 2);
  XCTAssertEqual(firstCallbackCount, 1);
  XCTAssertEqual(secondCallbackCount, 1);
  XCTAssertEqual(thirdCallbackCount, 0);
  XCTAssertEqual([context copyCallbacksForCompletion].count, 3);
}

@end
