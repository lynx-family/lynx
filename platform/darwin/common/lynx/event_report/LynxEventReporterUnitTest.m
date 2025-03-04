// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <XCTest/XCTest.h>
#import "LynxEventReporter.h"
#import "LynxService.h"
#import "LynxServiceEventReporterProtocol.h"

// Event report async, so test case need waiting result.
static dispatch_semaphore_t testCaseLock;
static BOOL testRet = YES;
static const int32_t kTestInstanceId = 10010;
static const int32_t kTestInstanceId2 = 10011;

@interface MockEventReporterService : NSObject <LynxEventReportObserverProtocol>

@end

@implementation MockEventReporterService

- (void)onReportEvent:(nonnull NSString *)eventName
           instanceId:(NSInteger)instanceId
                props:(NSDictionary *_Nullable)props
            extraData:(NSDictionary *_Nullable)extraData {
  if (instanceId != kTestInstanceId && instanceId != kUnknownInstanceId) {
    return;
  }
  testRet = testRet && [[props objectForKey:@"prop_k_1"] isEqualToString:@"prop_v_1"] &&
            [[props objectForKey:@"prop_k_2"] isEqualToString:@"prop_v_2"] &&
            [[props objectForKey:@"prop_k_3"] isEqualToString:@"prop_v_3"];
  if ([eventName isEqualToString:@"testClearCache"] ||
      [eventName isEqualToString:@"testOnEventWithUnknownInstanceId"] ||
      [eventName isEqualToString:@"testOnEventWithPropsBuilderAndUnknownInstanceId"]) {
    testRet = testRet && (extraData == nil || extraData.count == 0);
    dispatch_semaphore_signal(testCaseLock);
    return;
  }

  BOOL genericInfoRet =
      [[props objectForKey:@"generic_info_k_1"] isEqualToString:@"generic_info_v_1"] &&
      [[props objectForKey:@"generic_info_k_2"] isEqualToString:@"generic_info_v_2"] &&
      [[props objectForKey:@"generic_info_k_3"] isEqualToString:@"generic_info_v_3"];

  if (![eventName isEqualToString:@"testRemoveGenericInfo"]) {
    testRet = testRet && genericInfoRet;
  }

  BOOL extraParamsRet =
      [[extraData objectForKey:@"extra_param_k_1"] isEqualToString:@"extra_param_v_1"] &&
      [[extraData objectForKey:@"extra_param_k_2"] isEqualToString:@"extra_param_v_2"];

  testRet = testRet && extraParamsRet;
  if ([eventName isEqualToString:@"testPutExtraParams"]) {
    testRet = testRet &&
              [[extraData objectForKey:@"extra_param_k_3"] isEqualToString:@"extra_param_v_33"];
  } else {
    testRet =
        testRet && [[extraData objectForKey:@"extra_param_k_3"] isEqualToString:@"extra_param_v_3"];
    if ([eventName isEqualToString:@"testMoveExtraParams"]) {
      testRet = testRet &&
                [[extraData objectForKey:@"extra_param_k_4"] isEqualToString:@"extra_param_v_4"];
    }
  }

  dispatch_semaphore_signal(testCaseLock);
}

- (void)onTimingSetup:(nonnull NSDictionary *)timingInfo
        withExtraData:(NSDictionary *_Nullable)extraData {
}

- (void)onTimingUpdate:(nonnull NSDictionary *)allTimingInfo
          updateTiming:(nonnull NSDictionary *)updateTiming
             extraData:(NSDictionary *_Nullable)extraData {
}

@end

@interface LynxEventReporter (HookLynxEventReporterService)

+ (instancetype)sharedInstance;

@end

@interface LynxEventReporterUnitTest : XCTestCase

@property(strong, nonatomic) MockEventReporterService *service;

@end

@implementation LynxEventReporterUnitTest

- (void)setUp {
  // Put setup code here. This method is called before the invocation of each test method in the
  // class.
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    self.service = [MockEventReporterService new];
    [LynxEventReporter addEventReportObserver:self.service];
    testCaseLock = dispatch_semaphore_create(0);
  });
  [LynxEventReporter updateGenericInfo:@"generic_info_v_1"
                                   key:@"generic_info_k_1"
                            instanceId:kTestInstanceId];
  [LynxEventReporter updateGenericInfo:@"generic_info_v_2"
                                   key:@"generic_info_k_2"
                            instanceId:kTestInstanceId];
  [LynxEventReporter updateGenericInfo:@"generic_info_v_3"
                                   key:@"generic_info_k_3"
                            instanceId:kTestInstanceId];
  [LynxEventReporter putExtraParams:@{
    @"extra_param_k_1" : @"extra_param_v_1",
    @"extra_param_k_2" : @"extra_param_v_2",
    @"extra_param_k_3" : @"extra_param_v_3"
  }
                      forInstanceId:kTestInstanceId];
  testRet = YES;
}

- (void)tearDown {
  // Put teardown code here. This method is called after the invocation of each test method in the
  // class.
}

- (void)testOnEvent {
  [LynxEventReporter onEvent:@"testOnEvent"
                  instanceId:kTestInstanceId
                       props:@{
                         @"prop_k_1" : @"prop_v_1",
                         @"prop_k_2" : @"prop_v_2",
                         @"prop_k_3" : @"prop_v_3"
                       }];
  dispatch_semaphore_wait(testCaseLock, DISPATCH_TIME_FOREVER);
  XCTAssertEqual(testRet, YES);
}

- (void)testOnEventWithUnknownInstanceId {
  [LynxEventReporter onEvent:@"testOnEventWithUnknownInstanceId"
                  instanceId:kUnknownInstanceId
                       props:@{
                         @"prop_k_1" : @"prop_v_1",
                         @"prop_k_2" : @"prop_v_2",
                         @"prop_k_3" : @"prop_v_3"
                       }];
  dispatch_semaphore_wait(testCaseLock, DISPATCH_TIME_FOREVER);
  XCTAssertEqual(testRet, YES);
}

- (void)testOnEventWithPropsBuilder {
  [LynxEventReporter
           onEvent:@"testOnEventWithPropsBuilder"
        instanceId:kTestInstanceId
      propsBuilder:^NSDictionary<NSString *, NSObject *> *_Nonnull {
        BOOL isMainQueue = dispatch_queue_get_label(DISPATCH_CURRENT_QUEUE_LABEL) ==
                           dispatch_queue_get_label(dispatch_get_main_queue());
        testRet = !isMainQueue;
        return @{@"prop_k_1" : @"prop_v_1", @"prop_k_2" : @"prop_v_2", @"prop_k_3" : @"prop_v_3"};
      }];

  dispatch_semaphore_wait(testCaseLock, DISPATCH_TIME_FOREVER);
  XCTAssertEqual(testRet, YES);
}

- (void)testOnEventWithPropsBuilderAndUnknownInstanceId {
  [LynxEventReporter
           onEvent:@"testOnEventWithPropsBuilderAndUnknownInstanceId"
        instanceId:kUnknownInstanceId
      propsBuilder:^NSDictionary<NSString *, NSObject *> *_Nonnull {
        BOOL isMainQueue = dispatch_queue_get_label(DISPATCH_CURRENT_QUEUE_LABEL) ==
                           dispatch_queue_get_label(dispatch_get_main_queue());
        testRet = !isMainQueue;
        return @{@"prop_k_1" : @"prop_v_1", @"prop_k_2" : @"prop_v_2", @"prop_k_3" : @"prop_v_3"};
      }];

  dispatch_semaphore_wait(testCaseLock, DISPATCH_TIME_FOREVER);
  XCTAssertEqual(testRet, YES);
}

- (void)testUpdateGenericInfo {
  [LynxEventReporter updateGenericInfo:@"testUpdateGenericInfo"
                                   key:@"generic_info_k_testUpdateGenericInfo"
                            instanceId:kTestInstanceId];
  [LynxEventReporter updateGenericInfo:@"testUpdateGenericInfo_1"
                                   key:@"generic_info_k_testUpdateGenericInfo_1"
                            instanceId:kTestInstanceId2];

  [LynxEventReporter
           onEvent:@"testUpdateGenericInfo"
        instanceId:kTestInstanceId
      propsBuilder:^NSDictionary<NSString *, NSObject *> *_Nonnull {
        BOOL isMainQueue = dispatch_queue_get_label(DISPATCH_CURRENT_QUEUE_LABEL) ==
                           dispatch_queue_get_label(dispatch_get_main_queue());
        testRet = !isMainQueue;
        return @{@"prop_k_1" : @"prop_v_1", @"prop_k_2" : @"prop_v_2", @"prop_k_3" : @"prop_v_3"};
      }];

  dispatch_semaphore_wait(testCaseLock, DISPATCH_TIME_FOREVER);
  XCTAssertEqual(testRet, YES);
}

- (void)testRemoveGenericInfo {
  [LynxEventReporter updateGenericInfo:@"testUpdateGenericInfo"
                                   key:@"generic_info_k_testUpdateGenericInfo"
                            instanceId:kTestInstanceId];
  [LynxEventReporter updateGenericInfo:@"testUpdateGenericInfo_1"
                                   key:@"generic_info_k_testUpdateGenericInfo_1"
                            instanceId:kTestInstanceId2];
  [LynxEventReporter removeGenericInfo:kTestInstanceId];
  [LynxEventReporter
           onEvent:@"testRemoveGenericInfo"
        instanceId:kTestInstanceId
      propsBuilder:^NSDictionary<NSString *, NSObject *> *_Nonnull {
        BOOL isMainQueue = dispatch_queue_get_label(DISPATCH_CURRENT_QUEUE_LABEL) ==
                           dispatch_queue_get_label(dispatch_get_main_queue());
        testRet = !isMainQueue;
        return @{@"prop_k_1" : @"prop_v_1", @"prop_k_2" : @"prop_v_2", @"prop_k_3" : @"prop_v_3"};
      }];

  dispatch_semaphore_wait(testCaseLock, DISPATCH_TIME_FOREVER);
  XCTAssertEqual(testRet, YES);
}

- (void)testPutExtraParams {
  [LynxEventReporter putExtraParams:@{
    @"extra_param_k_3" : @"extra_param_v_33",
    @"extra_param_k_4" : @"extra_param_v_4"
  }
                      forInstanceId:kTestInstanceId];
  [LynxEventReporter
           onEvent:@"testPutExtraParams"
        instanceId:kTestInstanceId
      propsBuilder:^NSDictionary<NSString *, NSObject *> *_Nonnull {
        BOOL isMainQueue = dispatch_queue_get_label(DISPATCH_CURRENT_QUEUE_LABEL) ==
                           dispatch_queue_get_label(dispatch_get_main_queue());
        testRet = !isMainQueue;
        return @{@"prop_k_1" : @"prop_v_1", @"prop_k_2" : @"prop_v_2", @"prop_k_3" : @"prop_v_3"};
      }];

  dispatch_semaphore_wait(testCaseLock, DISPATCH_TIME_FOREVER);
  XCTAssertEqual(testRet, YES);
}

- (void)testMoveExtraParams {
  [LynxEventReporter putExtraParams:@{
    @"extra_param_k_3" : @"extra_param_v_33",
    @"extra_param_k_4" : @"extra_param_v_4"
  }
                      forInstanceId:kTestInstanceId2];
  [LynxEventReporter moveExtraParams:kTestInstanceId2 toInstanceId:kTestInstanceId];
  [LynxEventReporter
           onEvent:@"testMoveExtraParams"
        instanceId:kTestInstanceId
      propsBuilder:^NSDictionary<NSString *, NSObject *> *_Nonnull {
        BOOL isMainQueue = dispatch_queue_get_label(DISPATCH_CURRENT_QUEUE_LABEL) ==
                           dispatch_queue_get_label(dispatch_get_main_queue());
        testRet = !isMainQueue;
        return @{@"prop_k_1" : @"prop_v_1", @"prop_k_2" : @"prop_v_2", @"prop_k_3" : @"prop_v_3"};
      }];

  dispatch_semaphore_wait(testCaseLock, DISPATCH_TIME_FOREVER);
  XCTAssertEqual(testRet, YES);
}

- (void)testClearCache {
  [LynxEventReporter updateGenericInfo:@"testUpdateGenericInfo"
                                   key:@"generic_info_k_testUpdateGenericInfo"
                            instanceId:kTestInstanceId];
  [LynxEventReporter updateGenericInfo:@"testUpdateGenericInfo_1"
                                   key:@"generic_info_k_testUpdateGenericInfo_1"
                            instanceId:kTestInstanceId2];
  [LynxEventReporter putExtraParams:@{
    @"extra_param_k_3" : @"extra_param_v_33",
    @"extra_param_k_4" : @"extra_param_v_4"
  }
                      forInstanceId:kTestInstanceId];
  [LynxEventReporter clearCacheForInstanceId:kTestInstanceId];
  [LynxEventReporter
           onEvent:@"testClearCache"
        instanceId:kTestInstanceId
      propsBuilder:^NSDictionary<NSString *, NSObject *> *_Nonnull {
        BOOL isMainQueue = dispatch_queue_get_label(DISPATCH_CURRENT_QUEUE_LABEL) ==
                           dispatch_queue_get_label(dispatch_get_main_queue());
        testRet = !isMainQueue;
        return @{@"prop_k_1" : @"prop_v_1", @"prop_k_2" : @"prop_v_2", @"prop_k_3" : @"prop_v_3"};
      }];

  dispatch_semaphore_wait(testCaseLock, DISPATCH_TIME_FOREVER);
  XCTAssertEqual(testRet, YES);
}

@end
