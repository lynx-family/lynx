// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <DebugRouter/DebugRouter.h>
#import <Lynx/LynxMemoryListener.h>
#import <Lynx/LynxMemoryRecord.h>
#import <Lynx/LynxMemoryUsageQuery.h>
#import <Lynx/LynxMemoryUsageResult.h>
#import <LynxDevtool/LynxMemoryController.h>
#import <OCMock/OCMock.h>
#import <XCTest/XCTest.h>

@interface LynxMemoryListenerUnitTest : XCTestCase

@end

@implementation LynxMemoryListenerUnitTest {
  LynxMemoryListener* _listener;
}

- (void)setUp {
  // Put setup code here. This method is called before the invocation of each test method in the
  // class.
  _listener = [LynxMemoryListener shareInstance];
}

- (void)tearDown {
  // Put teardown code here. This method is called after the invocation of each test method in the
  // class.
  _listener = nil;
}

- (void)testAddMemoryReporter {
  _listener = OCMPartialMock(_listener);
  OCMExpect([_listener addMemoryReporter:[OCMArg any]]);
  [[LynxMemoryController shareInstance] startMemoryTracing];
  OCMVerifyAll(_listener);
}

- (void)testRemoveMemoryReporter {
  _listener = OCMPartialMock(_listener);
  OCMExpect([_listener removeMemoryReporter:[OCMArg any]]);
  [[LynxMemoryController shareInstance] stopMemoryTracing];
  OCMVerifyAll(_listener);
}

- (void)testUploadImageInfo {
  DebugRouter* debugRouter = [DebugRouter instance];
  debugRouter = OCMPartialMock(debugRouter);
  NSDictionary* data = @{@"type" : @"image", @"image_url" : @"LynxMemoryListenerUnitTest.png"};

  NSDictionary* param = [[NSMutableDictionary alloc] initWithDictionary:@{@"data" : data}];
  NSMutableDictionary* msg = [[NSMutableDictionary alloc] init];
  msg[@"params"] = param;
  msg[@"method"] = @"Memory.uploadImageInfo";
  if ([NSJSONSerialization isValidJSONObject:msg]) {
    NSData* jsonData = [NSJSONSerialization dataWithJSONObject:msg options:0 error:nil];
    NSString* jsonStr = [[NSString alloc] initWithData:jsonData encoding:NSUTF8StringEncoding];
    OCMExpect([debugRouter sendDataAsync:jsonStr WithType:@"CDP" ForSession:-1]);

    [[LynxMemoryController shareInstance] uploadImageInfo:data];
    OCMVerifyAll(debugRouter);
  }
}

- (void)testQueryAllMemoryUsageSerializesResult {
  LynxMemoryRecord* viewRecord =
      [[LynxMemoryRecord alloc] initWithCategory:@"view"
                                       sizeBytes:40
                                          detail:@{@"" : @"ignored", @"source" : @"test"}];
  viewRecord.instanceCount = 2;
  LynxInstanceMemoryUsage* largerInstance = [[LynxInstanceMemoryUsage alloc] init];
  largerInstance.instanceId = 2;
  largerInstance.pageId = @"page-large";
  largerInstance.url = @"https://example.com/large";
  largerInstance.totalBytes = 100;
  largerInstance.elementBytes = 20;
  largerInstance.elementNodeCount = 3;
  largerInstance.viewBytes = 40;
  largerInstance.viewDetail = @{@"" : viewRecord, @"view" : viewRecord};
  largerInstance.mainThreadRuntimeBytes = 30;
  largerInstance.backgroundThreadRuntimeBytes = 10;
  largerInstance.btsRuntimeGroupId = @"group-1";
  largerInstance.btsRuntimeType = @"quickjs";
  largerInstance.btsRuntimeTotalBytes = 200;

  LynxInstanceMemoryUsage* emptyStringInstance = [[LynxInstanceMemoryUsage alloc] init];
  emptyStringInstance.instanceId = 1;
  emptyStringInstance.totalBytes = 10;

  LynxGlobalMemoryUsageResult* queryResult = [[LynxGlobalMemoryUsageResult alloc] init];
  queryResult.collectionStartMs = 1234;
  queryResult.collectionStatus = LynxMemoryCollectionStatusTimeout;
  queryResult.collectionDurationMs = 20;
  queryResult.collectionTimeoutMs = 123;
  queryResult.expectedInstanceCount = 2;
  queryResult.completedInstanceCount = 2;
  queryResult.totalBytes = 110;
  queryResult.appBytes = 220;
  queryResult.elementBytes = 20;
  queryResult.elementNodeCount = 3;
  queryResult.viewBytes = 40;
  queryResult.mainThreadRuntimeBytes = 30;
  queryResult.backgroundThreadRuntimeBytes = 10;
  queryResult.instances = @[ largerInstance, emptyStringInstance ];
  LynxInstanceMemoryUsage* residual = [[LynxInstanceMemoryUsage alloc] init];
  residual.instanceId = 3;
  residual.pageId = @"native-page-key";
  residual.url = @"https://host/exited?route=detail#original";
  residual.btsRuntimeGroupId = @"group-1";
  residual.btsRuntimeType = @"quickjs";
  residual.btsRuntimeTotalBytes = 200;
  residual.backgroundThreadRuntimeResidualBytes = 100;
  queryResult.destroyedInstances = @[ residual ];

  id queryMock = OCMClassMock([LynxMemoryUsageQuery class]);
  OCMStub(ClassMethod([queryMock sharedInstance])).andReturn(queryMock);
  OCMStub([queryMock queryLynxGlobalMemoryUsageAsync:[OCMArg any] timeoutMs:123])
      .andDo(^(NSInvocation* invocation) {
        __unsafe_unretained LynxGlobalMemoryUsageCallback callback = nil;
        [invocation getArgument:&callback atIndex:2];
        if (callback) {
          callback(queryResult);
        }
      });

  XCTestExpectation* expectation = [self expectationWithDescription:@"memory usage callback"];
  [[LynxMemoryController shareInstance]
      queryAllMemoryUsageWithTimeoutMs:123
                              callback:^(NSString* resultJson, NSString* errorMessage) {
                                XCTAssertEqual(errorMessage.length, 0);
                                XCTAssertGreaterThan(resultJson.length, 0);
                                NSData* data = [resultJson dataUsingEncoding:NSUTF8StringEncoding];
                                NSDictionary* result = [NSJSONSerialization JSONObjectWithData:data
                                                                                       options:0
                                                                                         error:nil];
                                XCTAssertTrue([result isKindOfClass:[NSDictionary class]]);
                                XCTAssertEqualObjects(result[@"collectionStartMs"], @1234);
                                XCTAssertEqualObjects(result[@"collectionStatus"], @"timeout");
                                XCTAssertEqualObjects(result[@"collectionDurationMs"], @20);
                                XCTAssertEqualObjects(result[@"collectionTimeoutMs"], @123);
                                XCTAssertEqualObjects(result[@"expectedInstanceCount"], @2);
                                XCTAssertEqualObjects(result[@"completedInstanceCount"], @2);
                                XCTAssertEqualObjects(result[@"totalBytes"], @110);
                                XCTAssertEqualObjects(result[@"appBytes"], @220);
                                XCTAssertEqualObjects(result[@"elementBytes"], @20);
                                XCTAssertEqualObjects(result[@"elementNodeCount"], @3);
                                XCTAssertEqualObjects(result[@"viewBytes"], @40);
                                XCTAssertEqualObjects(result[@"mainThreadRuntimeBytes"], @30);
                                XCTAssertEqualObjects(result[@"backgroundThreadRuntimeBytes"], @10);
                                NSArray* instances = result[@"instances"];
                                XCTAssertEqual(instances.count, 2);
                                NSDictionary* firstInstance = instances[0];
                                XCTAssertEqualObjects(firstInstance[@"instanceId"], @2);
                                XCTAssertEqualObjects(firstInstance[@"pageId"], @"page-large");
                                XCTAssertEqualObjects(firstInstance[@"url"],
                                                      @"https://example.com/large");
                                XCTAssertEqualObjects(firstInstance[@"btsRuntimeGroupId"],
                                                      @"group-1");
                                NSDictionary* viewDetail = firstInstance[@"viewDetail"];
                                XCTAssertNil([viewDetail objectForKey:@""]);
                                NSDictionary* record = viewDetail[@"view"];
                                XCTAssertEqualObjects(record[@"category"], @"view");
                                XCTAssertEqualObjects(record[@"sizeBytes"], @40);
                                XCTAssertEqualObjects(record[@"instanceCount"], @2);
                                NSDictionary* detail = record[@"detail"];
                                XCTAssertNil([detail objectForKey:@""]);
                                XCTAssertEqualObjects(detail[@"source"], @"test");
                                XCTAssertEqualObjects(firstInstance[@"btsRuntimeType"], @"quickjs");
                                XCTAssertEqualObjects(firstInstance[@"btsRuntimeTotalBytes"], @200);
                                XCTAssertNil(result[@"ratioToApp"]);
                                NSArray* destroyedInstances = result[@"destroyedInstances"];
                                XCTAssertEqual(destroyedInstances.count, 1);
                                NSDictionary* destroyed = destroyedInstances[0];
                                XCTAssertEqualObjects(destroyed[@"instanceId"], @3);
                                XCTAssertEqualObjects(destroyed[@"pageId"], @"native-page-key");
                                XCTAssertEqualObjects(destroyed[@"url"], residual.url);
                                XCTAssertEqualObjects(
                                    destroyed[@"backgroundThreadRuntimeResidualBytes"], @100);
                                for (NSString* key in @[
                                       @"totalBytes", @"elementBytes", @"elementNodeCount",
                                       @"viewBytes", @"viewDetail", @"mainThreadRuntimeBytes",
                                       @"backgroundThreadRuntimeBytes"
                                     ]) {
                                  XCTAssertNil(destroyed[key]);
                                }
                                NSDictionary* secondInstance = instances[1];
                                XCTAssertEqualObjects(secondInstance[@"instanceId"], @1);
                                XCTAssertEqualObjects(secondInstance[@"pageId"], @"");
                                XCTAssertEqualObjects(secondInstance[@"url"], @"");
                                XCTAssertEqualObjects(secondInstance[@"viewDetail"], @{});
                                XCTAssertEqualObjects(secondInstance[@"btsRuntimeGroupId"], @"");
                                [expectation fulfill];
                              }];
  [self waitForExpectations:@[ expectation ] timeout:1.0];
  [queryMock stopMocking];
}

- (void)testQueryAllMemoryUsagePropagatesMonitoringDisabledError {
  LynxGlobalMemoryUsageResult* queryResult = [[LynxGlobalMemoryUsageResult alloc] init];
  queryResult.collectionStatus = LynxMemoryCollectionStatusError;
  queryResult.errorMessage = @"Memory monitoring disabled";

  id queryMock = OCMClassMock([LynxMemoryUsageQuery class]);
  OCMStub(ClassMethod([queryMock sharedInstance])).andReturn(queryMock);
  OCMStub([queryMock queryLynxGlobalMemoryUsageAsync:[OCMArg any] timeoutMs:123])
      .andDo(^(NSInvocation* invocation) {
        __unsafe_unretained LynxGlobalMemoryUsageCallback callback = nil;
        [invocation getArgument:&callback atIndex:2];
        if (callback) {
          callback(queryResult);
        }
      });

  XCTestExpectation* expectation = [self expectationWithDescription:@"memory usage error callback"];
  [[LynxMemoryController shareInstance]
      queryAllMemoryUsageWithTimeoutMs:123
                              callback:^(NSString* resultJson, NSString* errorMessage) {
                                XCTAssertEqualObjects(errorMessage, queryResult.errorMessage);
                                NSData* data = [resultJson dataUsingEncoding:NSUTF8StringEncoding];
                                NSDictionary* result = [NSJSONSerialization JSONObjectWithData:data
                                                                                       options:0
                                                                                         error:nil];
                                XCTAssertTrue([result isKindOfClass:[NSDictionary class]]);
                                XCTAssertEqualObjects(result[@"collectionStatus"], @"error");
                                XCTAssertEqualObjects(result[@"errorMessage"], errorMessage);
                                XCTAssertEqualObjects(result[@"destroyedInstances"], @[]);
                                [expectation fulfill];
                              }];
  [self waitForExpectations:@[ expectation ] timeout:1.0];
  [queryMock stopMocking];
}

@end
