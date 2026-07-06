// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxElement.h>
#import <Lynx/LynxEngineProxy.h>
#import <Lynx/LynxTemplateRender.h>
#import <Lynx/LynxThreadManager.h>
#import <Lynx/LynxView.h>
#import <XCTest/XCTest.h>

@interface LynxElementUnitTest : XCTestCase
@end

@implementation LynxElementUnitTest

- (void)testPublicStatusConstants {
  XCTAssertEqualObjects(LynxElementStatusAlive, @"alive");
  XCTAssertEqualObjects(LynxElementStatusDetached, @"detached");
}

- (void)testPublicElementSelectorsCompile {
  LynxElement *element = nil;
  [element findNodeById:@"target"
               callback:^(LynxElement *_Nullable result){
               }];
  [element
      findNodeByFilter:^BOOL(LynxElement *_Nonnull node) {
        return node != nil;
      }
              callback:^(LynxElement *_Nullable result){
              }];
  [element
      findNodesByFilter:^BOOL(LynxElement *_Nonnull node) {
        return node != nil;
      }
               callback:^(NSArray<LynxElement *> *_Nonnull result){
               }];
  [element getStatus:^(LynxElementStatus _Nonnull result){
  }];
  [element getSign:^(NSNumber *_Nonnull result){
  }];
  [element getTagName:^(NSString *_Nullable result){
  }];
  [element getId:^(NSString *_Nullable result){
  }];
  [element getParent:^(LynxElement *_Nullable result){
  }];
  [element getChildren:^(NSArray<LynxElement *> *_Nonnull result){
  }];
  [element getChildCount:^(NSNumber *_Nonnull result){
  }];
  [element getRoot:^(LynxElement *_Nullable result){
  }];
  [element getPositionInfo:^(NSDictionary<NSString *, id> *_Nullable result){
  }];
  [element getDataset:^(NSDictionary<NSString *, id> *_Nonnull result){
  }];
  [element getAttribute:@"name"
               callback:^(id _Nullable result){
               }];
  [element getAttributes:^(NSDictionary<NSString *, id> *_Nonnull result){
  }];
  [element getPlatformView:^(UIView *_Nullable result){
  }];
  [element toJSONString:^(NSString *_Nonnull result){
  }];
}

- (void)testPublicRootOwnerSelectorsCompile {
  LynxTemplateRender *render = nil;
  LynxView *view = nil;
  [render getLynxElementRoot:^(LynxElement *_Nullable result){
  }];
  [view getLynxElementRoot:^(LynxElement *_Nullable result){
  }];
}

- (void)testMainQueueImmediateBlockRunsSynchronously {
  XCTAssertTrue([NSThread isMainThread]);
  __block BOOL callbackCalled = NO;

  [LynxThreadManager runBlockInMainQueueImmediately:^{
    callbackCalled = YES;
  }];

  XCTAssertTrue(callbackCalled);
}

- (void)testMainQueueImmediateBlockFromBackgroundRunsOnMainThread {
  XCTestExpectation *expectation = [self expectationWithDescription:@"main queue block"];

  dispatch_async(dispatch_get_global_queue(QOS_CLASS_DEFAULT, 0), ^{
    [LynxThreadManager runBlockInMainQueueImmediately:^{
      XCTAssertTrue([NSThread isMainThread]);
      [expectation fulfill];
    }];
  });

  [self waitForExpectationsWithTimeout:5 handler:nil];
}

- (void)testEngineProxyFallbackCallbackRunsSynchronouslyOnMainThread {
  XCTAssertTrue([NSThread isMainThread]);
  LynxEngineProxy *proxy = [[LynxEngineProxy alloc] init];
  __block BOOL callbackCalled = NO;

  [proxy queryLynxElementRoot:^(id _Nullable result) {
    XCTAssertNil(result);
    callbackCalled = YES;
  }];

  XCTAssertTrue(callbackCalled);
}

- (void)testRootFallbackCallbackRunsOnMainThread {
  XCTestExpectation *expectation = [self expectationWithDescription:@"root callback"];
  LynxView *view = [[LynxView alloc] init];

  dispatch_async(dispatch_get_global_queue(QOS_CLASS_DEFAULT, 0), ^{
    [view getLynxElementRoot:^(LynxElement *_Nullable result) {
      XCTAssertNil(result);
      XCTAssertTrue([NSThread isMainThread]);
      [expectation fulfill];
    }];
  });

  [self waitForExpectationsWithTimeout:5 handler:nil];
}

@end
