// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxEventReporter.h>
#import <Lynx/LynxMemoryRecord.h>
#import <Lynx/LynxMemoryUsageQuery.h>
#import <Lynx/LynxMemoryUsageResult.h>
#import <XCTest/XCTest.h>

#import "LynxMemoryUsageFetcher.h"

@interface LynxMemoryUsageCollector : NSObject

+ (instancetype)sharedCollector;
- (void)queryGlobalMemoryUsageAsync:(nullable LynxGlobalMemoryUsageCallback)callback;

@end

typedef void (^LynxMemoryUsageTestFetcherHandler)(LynxMemoryUsageFetcherCallback callback);

@interface LynxMemoryUsageTestFetcher : NSObject <LynxMemoryUsageFetcher>

@property(nonatomic, copy) LynxMemoryUsageTestFetcherHandler handler;

- (instancetype)initWithHandler:(LynxMemoryUsageTestFetcherHandler)handler;

@end

@implementation LynxMemoryUsageTestFetcher

- (instancetype)initWithHandler:(LynxMemoryUsageTestFetcherHandler)handler {
  self = [super init];
  if (self) {
    _handler = [handler copy];
  }
  return self;
}

- (void)queryMemoryUsageAsync:(LynxMemoryUsageFetcherCallback)callback {
  if (_handler) {
    _handler(callback);
  }
}

@end

static LynxInstanceMemoryUsage *LynxCreateMemoryUsageTestInstance(
    int32_t instanceId, int64_t elementBytes, int32_t elementNodeCount, int64_t viewBytes,
    int64_t mainThreadRuntimeBytes, int64_t backgroundThreadRuntimeBytes,
    NSString *btsRuntimeGroupId) {
  LynxInstanceMemoryUsage *instance = [[LynxInstanceMemoryUsage alloc] init];
  instance.instanceId = instanceId;
  instance.pageId = [NSString stringWithFormat:@"page-%d", instanceId];
  instance.url = [NSString stringWithFormat:@"https://example.com/%d", instanceId];
  instance.elementBytes = elementBytes;
  instance.elementNodeCount = elementNodeCount;
  instance.viewBytes = viewBytes;
  instance.mainThreadRuntimeBytes = mainThreadRuntimeBytes;
  instance.backgroundThreadRuntimeBytes = backgroundThreadRuntimeBytes;
  instance.totalBytes =
      elementBytes + viewBytes + mainThreadRuntimeBytes + backgroundThreadRuntimeBytes;
  LynxMemoryRecord *viewRecord = [[LynxMemoryRecord alloc] initWithCategory:@"view"
                                                                  sizeBytes:viewBytes
                                                                     detail:nil];
  instance.viewDetail = @{@"view" : viewRecord};
  instance.btsRuntimeGroupId = btsRuntimeGroupId ?: @"";
  return instance;
}

@interface LynxMemoryUsageCollectorUnitTest : XCTestCase

@end

@implementation LynxMemoryUsageCollectorUnitTest

- (void)setUp {
  [super setUp];
  [self unregisterAllMemoryUsageFetchers];
}

- (void)tearDown {
  [self unregisterAllMemoryUsageFetchers];
  [super tearDown];
}

- (void)unregisterAllMemoryUsageFetchers {
  for (id<LynxMemoryUsageFetcher> fetcher in [LynxMemoryUsageFetchers fetchersForCurrentQuery]) {
    [LynxMemoryUsageFetchers unregisterFetcher:fetcher];
  }
}

- (void)testCollectorReturnsEmptyCompletedResultAsynchronouslyWithoutFetchers {
  XCTestExpectation *expectation = [self expectationWithDescription:@"empty memory usage callback"];
  __block BOOL callbackCanRunSynchronously = YES;

  [[LynxMemoryUsageCollector sharedCollector]
      queryGlobalMemoryUsageAsync:^(LynxGlobalMemoryUsageResult *result) {
        XCTAssertFalse(callbackCanRunSynchronously);
        XCTAssertNotNil(result);
        XCTAssertGreaterThan(result.collectionStartMs, 0);
        XCTAssertEqual(result.collectionStatus, LynxMemoryCollectionStatusCompleted);
        XCTAssertGreaterThanOrEqual(result.collectionDurationMs, 0);
        XCTAssertGreaterThan(result.collectionTimeoutMs, 0);
        XCTAssertEqual(result.expectedInstanceCount, 0);
        XCTAssertEqual(result.completedInstanceCount, 0);
        XCTAssertEqual(result.totalBytes, 0);
        XCTAssertGreaterThanOrEqual(result.appBytes, 0);
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

- (void)testCollectorAggregatesCompletedFetcherResults {
  LynxInstanceMemoryUsage *smallerInstance =
      LynxCreateMemoryUsageTestInstance(1, 30, 3, 20, 10, 40, @"");
  LynxInstanceMemoryUsage *largerInstance =
      LynxCreateMemoryUsageTestInstance(2, 70, 7, 60, 50, 20, @"");
  LynxMemoryUsageTestFetcher *smallerFetcher = [[LynxMemoryUsageTestFetcher alloc]
      initWithHandler:^(LynxMemoryUsageFetcherCallback callback) {
        callback(smallerInstance);
      }];
  LynxMemoryUsageTestFetcher *largerFetcher = [[LynxMemoryUsageTestFetcher alloc]
      initWithHandler:^(LynxMemoryUsageFetcherCallback callback) {
        callback(largerInstance);
      }];
  XCTAssertTrue([LynxMemoryUsageFetchers registerFetcher:smallerFetcher]);
  XCTAssertTrue([LynxMemoryUsageFetchers registerFetcher:largerFetcher]);

  XCTestExpectation *expectation = [self expectationWithDescription:@"aggregated memory usage"];
  [[LynxMemoryUsageCollector sharedCollector]
      queryGlobalMemoryUsageAsync:^(LynxGlobalMemoryUsageResult *result) {
        XCTAssertEqual(result.collectionStatus, LynxMemoryCollectionStatusCompleted);
        XCTAssertEqual(result.expectedInstanceCount, 2);
        XCTAssertEqual(result.completedInstanceCount, 2);
        XCTAssertEqual(result.elementBytes, 100);
        XCTAssertEqual(result.elementNodeCount, 10);
        XCTAssertEqual(result.viewBytes, 80);
        XCTAssertEqual(result.mainThreadRuntimeBytes, 60);
        XCTAssertEqual(result.backgroundThreadRuntimeBytes, 60);
        XCTAssertEqual(result.totalBytes, 300);
        XCTAssertEqual(result.instances.count, 2);
        XCTAssertEqual(result.instances[0].instanceId, 2);
        XCTAssertEqual(result.instances[1].instanceId, 1);
        XCTAssertGreaterThanOrEqual(result.instances[0].totalBytes, result.instances[1].totalBytes);
        [expectation fulfill];
      }];

  [self waitForExpectations:@[ expectation ] timeout:1.0];
  XCTAssertTrue([LynxMemoryUsageFetchers unregisterFetcher:smallerFetcher]);
  XCTAssertTrue([LynxMemoryUsageFetchers unregisterFetcher:largerFetcher]);
}

- (void)testCollectorCoalescesCallbacksIntoOneActiveCollection {
  LynxInstanceMemoryUsage *instance = LynxCreateMemoryUsageTestInstance(1, 10, 1, 20, 30, 40, @"");
  XCTestExpectation *fetcherInvoked = [self expectationWithDescription:@"fetcher invoked once"];
  XCTestExpectation *queriesReachReportThread =
      [self expectationWithDescription:@"queries reach report thread"];
  XCTestExpectation *firstCallback = [self expectationWithDescription:@"first callback"];
  XCTestExpectation *secondCallback = [self expectationWithDescription:@"second callback"];
  __block LynxMemoryUsageFetcherCallback pendingFetcherCallback = nil;
  __block NSInteger fetcherInvocationCount = 0;
  __block LynxGlobalMemoryUsageResult *firstResult = nil;
  __block LynxGlobalMemoryUsageResult *secondResult = nil;
  LynxMemoryUsageTestFetcher *fetcher = [[LynxMemoryUsageTestFetcher alloc]
      initWithHandler:^(LynxMemoryUsageFetcherCallback callback) {
        fetcherInvocationCount++;
        pendingFetcherCallback = [callback copy];
        [fetcherInvoked fulfill];
      }];
  XCTAssertTrue([LynxMemoryUsageFetchers registerFetcher:fetcher]);

  [[LynxMemoryUsageCollector sharedCollector]
      queryGlobalMemoryUsageAsync:^(LynxGlobalMemoryUsageResult *result) {
        firstResult = result;
        XCTAssertEqual(result.collectionStatus, LynxMemoryCollectionStatusCompleted);
        XCTAssertEqual(result.expectedInstanceCount, 1);
        XCTAssertEqual(result.completedInstanceCount, 1);
        XCTAssertEqual(result.totalBytes, 100);
        [firstCallback fulfill];
      }];
  [[LynxMemoryUsageCollector sharedCollector]
      queryGlobalMemoryUsageAsync:^(LynxGlobalMemoryUsageResult *result) {
        secondResult = result;
        XCTAssertEqual(result.collectionStatus, LynxMemoryCollectionStatusCompleted);
        XCTAssertEqual(result.expectedInstanceCount, 1);
        XCTAssertEqual(result.completedInstanceCount, 1);
        XCTAssertEqual(result.totalBytes, 100);
        [secondCallback fulfill];
      }];
  [LynxEventReporter
      delayRunOnReportThread:^{
        [queriesReachReportThread fulfill];
      }
                     delayMs:0];

  [self waitForExpectations:@[ fetcherInvoked, queriesReachReportThread ] timeout:1.0];
  XCTAssertEqual(fetcherInvocationCount, 1);
  XCTAssertNotNil(pendingFetcherCallback);
  pendingFetcherCallback(instance);

  [self waitForExpectations:@[ firstCallback, secondCallback ] timeout:1.0];
  XCTAssertEqual(firstResult, secondResult);
  XCTAssertTrue([LynxMemoryUsageFetchers unregisterFetcher:fetcher]);
}

- (void)testCollectorIgnoresDuplicateFetcherCallback {
  LynxInstanceMemoryUsage *firstInstance =
      LynxCreateMemoryUsageTestInstance(1, 10, 1, 0, 0, 0, @"");
  LynxInstanceMemoryUsage *duplicateInstance =
      LynxCreateMemoryUsageTestInstance(2, 1000, 1, 0, 0, 0, @"");
  LynxInstanceMemoryUsage *secondInstance =
      LynxCreateMemoryUsageTestInstance(3, 20, 1, 0, 0, 0, @"");
  XCTestExpectation *firstFetcherInvoked = [self expectationWithDescription:@"first fetcher"];
  XCTestExpectation *secondFetcherInvoked = [self expectationWithDescription:@"second fetcher"];
  __block LynxMemoryUsageFetcherCallback firstFetcherCallback = nil;
  __block LynxMemoryUsageFetcherCallback secondFetcherCallback = nil;
  LynxMemoryUsageTestFetcher *firstFetcher = [[LynxMemoryUsageTestFetcher alloc]
      initWithHandler:^(LynxMemoryUsageFetcherCallback callback) {
        firstFetcherCallback = [callback copy];
        [firstFetcherInvoked fulfill];
      }];
  LynxMemoryUsageTestFetcher *secondFetcher = [[LynxMemoryUsageTestFetcher alloc]
      initWithHandler:^(LynxMemoryUsageFetcherCallback callback) {
        secondFetcherCallback = [callback copy];
        [secondFetcherInvoked fulfill];
      }];
  XCTAssertTrue([LynxMemoryUsageFetchers registerFetcher:firstFetcher]);
  XCTAssertTrue([LynxMemoryUsageFetchers registerFetcher:secondFetcher]);

  XCTestExpectation *expectation = [self expectationWithDescription:@"single-shot fetcher result"];
  [[LynxMemoryUsageCollector sharedCollector]
      queryGlobalMemoryUsageAsync:^(LynxGlobalMemoryUsageResult *result) {
        XCTAssertEqual(result.collectionStatus, LynxMemoryCollectionStatusCompleted);
        XCTAssertEqual(result.expectedInstanceCount, 2);
        XCTAssertEqual(result.completedInstanceCount, 2);
        XCTAssertEqual(result.elementBytes, 30);
        XCTAssertEqual(result.instances.count, 2);
        XCTAssertEqual(result.instances[0].instanceId, 3);
        XCTAssertEqual(result.instances[1].instanceId, 1);
        [expectation fulfill];
      }];

  [self waitForExpectations:@[ firstFetcherInvoked, secondFetcherInvoked ] timeout:1.0];
  XCTAssertNotNil(firstFetcherCallback);
  XCTAssertNotNil(secondFetcherCallback);
  firstFetcherCallback(firstInstance);
  firstFetcherCallback(duplicateInstance);
  secondFetcherCallback(secondInstance);

  [self waitForExpectations:@[ expectation ] timeout:1.0];
  XCTAssertTrue([LynxMemoryUsageFetchers unregisterFetcher:firstFetcher]);
  XCTAssertTrue([LynxMemoryUsageFetchers unregisterFetcher:secondFetcher]);
}

- (void)testCollectorContinuesInvokingCallbacksAfterCallbackException {
  LynxInstanceMemoryUsage *instance = LynxCreateMemoryUsageTestInstance(1, 10, 1, 0, 0, 0, @"");
  XCTestExpectation *fetcherInvoked = [self expectationWithDescription:@"fetcher invoked"];
  XCTestExpectation *queriesReachReportThread =
      [self expectationWithDescription:@"queries reach report thread"];
  XCTestExpectation *secondCallback = [self expectationWithDescription:@"second callback"];
  __block LynxMemoryUsageFetcherCallback pendingFetcherCallback = nil;
  LynxMemoryUsageTestFetcher *fetcher = [[LynxMemoryUsageTestFetcher alloc]
      initWithHandler:^(LynxMemoryUsageFetcherCallback callback) {
        pendingFetcherCallback = [callback copy];
        [fetcherInvoked fulfill];
      }];
  XCTAssertTrue([LynxMemoryUsageFetchers registerFetcher:fetcher]);

  [[LynxMemoryUsageCollector sharedCollector]
      queryGlobalMemoryUsageAsync:^(LynxGlobalMemoryUsageResult *result) {
        [NSException raise:NSGenericException format:@"first callback failed"];
      }];
  [[LynxMemoryUsageCollector sharedCollector]
      queryGlobalMemoryUsageAsync:^(LynxGlobalMemoryUsageResult *result) {
        XCTAssertEqual(result.collectionStatus, LynxMemoryCollectionStatusCompleted);
        XCTAssertEqual(result.expectedInstanceCount, 1);
        XCTAssertEqual(result.completedInstanceCount, 1);
        XCTAssertEqual(result.totalBytes, 10);
        [secondCallback fulfill];
      }];
  [LynxEventReporter
      delayRunOnReportThread:^{
        [queriesReachReportThread fulfill];
      }
                     delayMs:0];

  [self waitForExpectations:@[ fetcherInvoked, queriesReachReportThread ] timeout:1.0];
  XCTAssertNotNil(pendingFetcherCallback);
  pendingFetcherCallback(instance);

  [self waitForExpectations:@[ secondCallback ] timeout:1.0];
  XCTAssertTrue([LynxMemoryUsageFetchers unregisterFetcher:fetcher]);
}

- (void)testCollectorDeduplicatesOnlySharedBackgroundRuntimeGroups {
  NSArray<LynxInstanceMemoryUsage *> *instances = @[
    LynxCreateMemoryUsageTestInstance(1, 0, 0, 0, 0, 40, @"-1"),
    LynxCreateMemoryUsageTestInstance(2, 0, 0, 0, 0, 60, @"-1"),
    LynxCreateMemoryUsageTestInstance(3, 0, 0, 0, 0, 50, @"shared-runtime"),
    LynxCreateMemoryUsageTestInstance(4, 0, 0, 0, 0, 90, @"shared-runtime"),
  ];
  NSMutableArray<LynxMemoryUsageTestFetcher *> *fetchers = [NSMutableArray array];
  for (LynxInstanceMemoryUsage *instance in instances) {
    LynxMemoryUsageTestFetcher *fetcher = [[LynxMemoryUsageTestFetcher alloc]
        initWithHandler:^(LynxMemoryUsageFetcherCallback callback) {
          callback(instance);
        }];
    [fetchers addObject:fetcher];
    XCTAssertTrue([LynxMemoryUsageFetchers registerFetcher:fetcher]);
  }

  XCTestExpectation *expectation =
      [self expectationWithDescription:@"deduplicated background runtime usage"];
  [[LynxMemoryUsageCollector sharedCollector]
      queryGlobalMemoryUsageAsync:^(LynxGlobalMemoryUsageResult *result) {
        XCTAssertEqual(result.collectionStatus, LynxMemoryCollectionStatusCompleted);
        XCTAssertEqual(result.expectedInstanceCount, 4);
        XCTAssertEqual(result.completedInstanceCount, 4);
        XCTAssertEqual(result.backgroundThreadRuntimeBytes, 190);
        XCTAssertEqual(result.totalBytes, 190);
        [expectation fulfill];
      }];

  [self waitForExpectations:@[ expectation ] timeout:1.0];
  for (LynxMemoryUsageTestFetcher *fetcher in fetchers) {
    XCTAssertTrue([LynxMemoryUsageFetchers unregisterFetcher:fetcher]);
  }
}

- (void)testCollectorTreatsNilFetcherResultAsCompletedButExcluded {
  LynxMemoryUsageTestFetcher *fetcher = [[LynxMemoryUsageTestFetcher alloc]
      initWithHandler:^(LynxMemoryUsageFetcherCallback callback) {
        callback(nil);
      }];
  XCTAssertTrue([LynxMemoryUsageFetchers registerFetcher:fetcher]);

  XCTestExpectation *expectation = [self expectationWithDescription:@"nil fetcher result"];
  [[LynxMemoryUsageCollector sharedCollector]
      queryGlobalMemoryUsageAsync:^(LynxGlobalMemoryUsageResult *result) {
        XCTAssertEqual(result.collectionStatus, LynxMemoryCollectionStatusCompleted);
        XCTAssertEqual(result.expectedInstanceCount, 1);
        XCTAssertEqual(result.completedInstanceCount, 0);
        XCTAssertEqual(result.totalBytes, 0);
        XCTAssertEqual(result.instances.count, 0);
        [expectation fulfill];
      }];

  [self waitForExpectations:@[ expectation ] timeout:1.0];
  XCTAssertTrue([LynxMemoryUsageFetchers unregisterFetcher:fetcher]);
}

- (void)testCollectorUsesSnapshotAfterFetchersUnregisterDuringInFlight {
  LynxInstanceMemoryUsage *instance = LynxCreateMemoryUsageTestInstance(1, 10, 1, 20, 30, 40, @"");
  XCTestExpectation *nilFetcherInvoked = [self expectationWithDescription:@"nil fetcher"];
  XCTestExpectation *instanceFetcherInvoked = [self expectationWithDescription:@"instance fetcher"];
  __block LynxMemoryUsageFetcherCallback nilFetcherCallback = nil;
  __block LynxMemoryUsageFetcherCallback instanceFetcherCallback = nil;
  LynxMemoryUsageTestFetcher *nilFetcher = [[LynxMemoryUsageTestFetcher alloc]
      initWithHandler:^(LynxMemoryUsageFetcherCallback callback) {
        nilFetcherCallback = [callback copy];
        [nilFetcherInvoked fulfill];
      }];
  LynxMemoryUsageTestFetcher *instanceFetcher = [[LynxMemoryUsageTestFetcher alloc]
      initWithHandler:^(LynxMemoryUsageFetcherCallback callback) {
        instanceFetcherCallback = [callback copy];
        [instanceFetcherInvoked fulfill];
      }];
  XCTAssertTrue([LynxMemoryUsageFetchers registerFetcher:nilFetcher]);
  XCTAssertTrue([LynxMemoryUsageFetchers registerFetcher:instanceFetcher]);

  XCTestExpectation *expectation =
      [self expectationWithDescription:@"in-flight unregistered fetchers"];
  [[LynxMemoryUsageCollector sharedCollector]
      queryGlobalMemoryUsageAsync:^(LynxGlobalMemoryUsageResult *result) {
        XCTAssertEqual(result.collectionStatus, LynxMemoryCollectionStatusCompleted);
        XCTAssertEqual(result.expectedInstanceCount, 2);
        XCTAssertEqual(result.completedInstanceCount, 1);
        XCTAssertEqual(result.totalBytes, 100);
        XCTAssertEqual(result.instances.count, 1);
        XCTAssertEqual(result.instances[0].instanceId, 1);
        [expectation fulfill];
      }];

  [self waitForExpectations:@[ nilFetcherInvoked, instanceFetcherInvoked ] timeout:1.0];
  XCTAssertTrue([LynxMemoryUsageFetchers unregisterFetcher:nilFetcher]);
  XCTAssertTrue([LynxMemoryUsageFetchers unregisterFetcher:instanceFetcher]);
  XCTAssertNotNil(nilFetcherCallback);
  XCTAssertNotNil(instanceFetcherCallback);
  nilFetcherCallback(nil);
  instanceFetcherCallback(instance);

  [self waitForExpectations:@[ expectation ] timeout:1.0];
}

- (void)testCollectorReturnsTimeoutResultWhenFetcherDoesNotComplete {
  LynxMemoryUsageTestFetcher *fetcher = [[LynxMemoryUsageTestFetcher alloc]
      initWithHandler:^(LynxMemoryUsageFetcherCallback callback){
          // Intentionally keep the fetcher callback pending. The collector must fail
          // deterministically through its bounded timeout instead of hanging the test process.
      }];
  XCTAssertTrue([LynxMemoryUsageFetchers registerFetcher:fetcher]);

  XCTestExpectation *expectation = [self expectationWithDescription:@"memory usage timeout"];
  [[LynxMemoryUsageCollector sharedCollector]
      queryGlobalMemoryUsageAsync:^(LynxGlobalMemoryUsageResult *result) {
        XCTAssertEqual(result.collectionStatus, LynxMemoryCollectionStatusTimeout);
        XCTAssertEqual(result.expectedInstanceCount, 1);
        XCTAssertEqual(result.completedInstanceCount, 0);
        XCTAssertEqual(result.totalBytes, 0);
        XCTAssertEqual(result.instances.count, 0);
        [expectation fulfill];
      }];

  [self waitForExpectations:@[ expectation ] timeout:5.0];
  XCTAssertTrue([LynxMemoryUsageFetchers unregisterFetcher:fetcher]);
}

@end
