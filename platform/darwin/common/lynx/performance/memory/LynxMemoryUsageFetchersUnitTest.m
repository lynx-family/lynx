// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <XCTest/XCTest.h>

#import "LynxMemoryUsageFetcher.h"

@interface LynxMemoryUsageFetcherRegistryTestFetcher : NSObject <LynxMemoryUsageFetcher>

@end

@implementation LynxMemoryUsageFetcherRegistryTestFetcher

- (void)queryMemoryUsageAsync:(LynxMemoryUsageFetcherCallback)callback {
  callback(nil);
}

@end

@interface LynxMemoryUsageEqualTestFetcher : LynxMemoryUsageFetcherRegistryTestFetcher

@end

@implementation LynxMemoryUsageEqualTestFetcher

- (BOOL)isEqual:(id)object {
  return [object isKindOfClass:[LynxMemoryUsageEqualTestFetcher class]];
}

- (NSUInteger)hash {
  return 1;
}

@end

@interface LynxMemoryUsageFetchersUnitTest : XCTestCase

@end

@implementation LynxMemoryUsageFetchersUnitTest

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

- (void)testRegistryRejectsNilDuplicateAndUnknownFetchers {
  LynxMemoryUsageFetcherRegistryTestFetcher *fetcher =
      [[LynxMemoryUsageFetcherRegistryTestFetcher alloc] init];
  LynxMemoryUsageFetcherRegistryTestFetcher *unknownFetcher =
      [[LynxMemoryUsageFetcherRegistryTestFetcher alloc] init];

  XCTAssertFalse([LynxMemoryUsageFetchers registerFetcher:nil]);
  XCTAssertTrue([LynxMemoryUsageFetchers registerFetcher:fetcher]);
  XCTAssertFalse([LynxMemoryUsageFetchers registerFetcher:fetcher]);
  XCTAssertEqual([LynxMemoryUsageFetchers fetchersForCurrentQuery].count, 1);
  XCTAssertFalse([LynxMemoryUsageFetchers unregisterFetcher:unknownFetcher]);
  XCTAssertTrue([LynxMemoryUsageFetchers unregisterFetcher:fetcher]);
  XCTAssertFalse([LynxMemoryUsageFetchers unregisterFetcher:fetcher]);
  XCTAssertEqual([LynxMemoryUsageFetchers fetchersForCurrentQuery].count, 0);
}

- (void)testRegistrySnapshotKeepsRegisteredFetchersStable {
  LynxMemoryUsageFetcherRegistryTestFetcher *firstFetcher =
      [[LynxMemoryUsageFetcherRegistryTestFetcher alloc] init];
  LynxMemoryUsageFetcherRegistryTestFetcher *secondFetcher =
      [[LynxMemoryUsageFetcherRegistryTestFetcher alloc] init];

  XCTAssertTrue([LynxMemoryUsageFetchers registerFetcher:firstFetcher]);
  NSArray<id<LynxMemoryUsageFetcher>> *snapshot = [LynxMemoryUsageFetchers fetchersForCurrentQuery];
  XCTAssertTrue([LynxMemoryUsageFetchers registerFetcher:secondFetcher]);
  XCTAssertEqual(snapshot.count, 1);
  XCTAssertTrue(snapshot[0] == firstFetcher);
  XCTAssertEqual([LynxMemoryUsageFetchers fetchersForCurrentQuery].count, 2);
}

- (void)testRegistryDoesNotRetainFetchers {
  __weak LynxMemoryUsageFetcherRegistryTestFetcher *weakFetcher = nil;
  @autoreleasepool {
    LynxMemoryUsageFetcherRegistryTestFetcher *fetcher =
        [[LynxMemoryUsageFetcherRegistryTestFetcher alloc] init];
    weakFetcher = fetcher;

    XCTAssertTrue([LynxMemoryUsageFetchers registerFetcher:fetcher]);
    XCTAssertEqual([LynxMemoryUsageFetchers fetchersForCurrentQuery].count, 1);
  }

  XCTAssertNil(weakFetcher);
  XCTAssertEqual([LynxMemoryUsageFetchers fetchersForCurrentQuery].count, 0);
}

- (void)testRegistryUsesObjectIdentityForEqualFetchers {
  LynxMemoryUsageEqualTestFetcher *firstFetcher = [[LynxMemoryUsageEqualTestFetcher alloc] init];
  LynxMemoryUsageEqualTestFetcher *secondFetcher = [[LynxMemoryUsageEqualTestFetcher alloc] init];

  XCTAssertEqualObjects(firstFetcher, secondFetcher);
  XCTAssertTrue([LynxMemoryUsageFetchers registerFetcher:firstFetcher]);
  XCTAssertTrue([LynxMemoryUsageFetchers registerFetcher:secondFetcher]);

  NSInteger identityMatchCount = 0;
  for (id<LynxMemoryUsageFetcher> fetcher in [LynxMemoryUsageFetchers fetchersForCurrentQuery]) {
    if (fetcher == firstFetcher || fetcher == secondFetcher) {
      identityMatchCount++;
    }
  }
  XCTAssertEqual(identityMatchCount, 2);
}

@end
