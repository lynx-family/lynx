// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxMemoryRecord.h>
#import <Lynx/LynxMemoryUsageResult.h>
#import <XCTest/XCTest.h>

#import "platform/darwin/common/lynx/performance/memory/LynxMemoryUsageFetcher.h"

#include "core/shell/performance/native_memory_usage_query.h"

@interface LynxTemplateRenderInstanceMemoryUsageQuery : NSObject

+ (NSDictionary<NSString *, LynxMemoryRecord *> *)
    copyViewMemoryRecords:(NSDictionary<NSString *, LynxMemoryRecord *> *)records
               totalBytes:(int64_t *)totalBytes;

- (instancetype)initWithInstanceId:(int32_t)instanceId
                               url:(NSString *)url
                          callback:(LynxMemoryUsageFetcherCallback)callback;
- (void)completeNativeSnapshot:(const lynx::tasm::performance::NativeMemoryUsageSnapshot &)snapshot;
- (void)completeViewSnapshotWithBytes:(int64_t)viewBytes
                               detail:(NSDictionary<NSString *, LynxMemoryRecord *> *)detail;
- (void)completeInvalidSnapshot;

@end

@interface LynxTemplateRenderMemoryUsageUnitTest : XCTestCase

@end

@implementation LynxTemplateRenderMemoryUsageUnitTest

- (void)testInstanceMemoryUsageQueryCombinesNativeAndViewSamplesOnce {
  __block NSMutableArray<LynxInstanceMemoryUsage *> *results = [NSMutableArray array];
  LynxTemplateRenderInstanceMemoryUsageQuery *query =
      [[LynxTemplateRenderInstanceMemoryUsageQuery alloc]
          initWithInstanceId:42
                         url:@"https://lynx.test/page"
                    callback:^(LynxInstanceMemoryUsage *result) {
                      if (result) {
                        [results addObject:result];
                      }
                    }];
  LynxMemoryRecord *viewRecord =
      [[LynxMemoryRecord alloc] initWithCategory:@"view"
                                       sizeBytes:32
                                          detail:@{@"class" : @"LynxUIView"}];
  viewRecord.instanceCount = 2;

  [query completeViewSnapshotWithBytes:32 detail:@{@"LynxUIView" : viewRecord}];
  XCTAssertEqual(results.count, 0);

  lynx::tasm::performance::NativeMemoryUsageSnapshot snapshot;
  snapshot.element_bytes_ = 11;
  snapshot.element_node_count_ = 3;
  snapshot.main_thread_runtime_bytes_ = 13;
  snapshot.background_thread_runtime_bytes_ = 17;
  snapshot.bts_runtime_group_id_ = "group-a";
  [query completeNativeSnapshot:snapshot];

  XCTAssertEqual(results.count, 1);
  LynxInstanceMemoryUsage *result = results.firstObject;
  XCTAssertEqual(result.instanceId, 42);
  XCTAssertEqualObjects(result.pageId, @"https://lynx.test/page");
  XCTAssertEqualObjects(result.url, @"https://lynx.test/page");
  XCTAssertEqual(result.elementBytes, 11);
  XCTAssertEqual(result.elementNodeCount, 3);
  XCTAssertEqual(result.viewBytes, 32);
  LynxMemoryRecord *resultViewRecord = result.viewDetail[@"LynxUIView"];
  XCTAssertEqual(resultViewRecord.sizeBytes, 32);
  XCTAssertEqual(resultViewRecord.instanceCount, 2);
  XCTAssertEqualObjects(resultViewRecord.detail[@"class"], @"LynxUIView");
  XCTAssertEqual(result.mainThreadRuntimeBytes, 13);
  XCTAssertEqual(result.backgroundThreadRuntimeBytes, 17);
  XCTAssertEqualObjects(result.btsRuntimeGroupId, @"group-a");
  XCTAssertEqual(result.totalBytes, 73);

  [query completeNativeSnapshot:snapshot];
  XCTAssertEqual(results.count, 1);
}

- (void)testInstanceMemoryUsageQueryReturnsNilForInvalidInstanceSnapshot {
  __block NSInteger callbackCount = 0;
  __block LynxInstanceMemoryUsage *callbackResult = nil;
  LynxTemplateRenderInstanceMemoryUsageQuery *query =
      [[LynxTemplateRenderInstanceMemoryUsageQuery alloc]
          initWithInstanceId:42
                         url:@"https://lynx.test/page"
                    callback:^(LynxInstanceMemoryUsage *result) {
                      callbackCount++;
                      callbackResult = result;
                    }];

  [query completeInvalidSnapshot];
  XCTAssertEqual(callbackCount, 0);

  lynx::tasm::performance::NativeMemoryUsageSnapshot snapshot;
  snapshot.element_bytes_ = 11;
  [query completeNativeSnapshot:snapshot];

  XCTAssertEqual(callbackCount, 1);
  XCTAssertNil(callbackResult);
}

- (void)testCopyViewMemoryRecordsReturnsDetachedRecords {
  NSMutableDictionary<NSString *, NSString *> *imageDetail = [@{@"url" : @"image-a"} mutableCopy];
  LynxMemoryRecord *imageRecord = [[LynxMemoryRecord alloc] initWithCategory:@"image"
                                                                   sizeBytes:50
                                                                      detail:imageDetail];
  imageRecord.instanceCount = 2;
  LynxMemoryRecord *textRecord = [[LynxMemoryRecord alloc] initWithCategory:@"text"
                                                                  sizeBytes:12
                                                                     detail:nil];
  textRecord.instanceCount = 3;

  int64_t totalBytes = 0;
  NSDictionary<NSString *, LynxMemoryRecord *> *records =
      [LynxTemplateRenderInstanceMemoryUsageQuery copyViewMemoryRecords:@{
        @"image" : imageRecord,
        @"" : textRecord,
      }
                                                             totalBytes:&totalBytes];

  imageRecord.sizeBytes = 99;
  imageRecord.instanceCount = 9;
  imageDetail[@"url"] = @"image-b";

  XCTAssertEqual(totalBytes, 62);
  LynxMemoryRecord *imageCopy = records[@"image"];
  XCTAssertEqual(imageCopy.sizeBytes, 50);
  XCTAssertEqual(imageCopy.instanceCount, 2);
  XCTAssertEqualObjects(imageCopy.detail[@"url"], @"image-a");
  XCTAssertNotNil(records[@"text"]);
}

@end
