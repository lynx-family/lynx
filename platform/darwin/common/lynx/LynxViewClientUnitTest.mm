// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <XCTest/XCTest.h>
#import <objc/runtime.h>

#import "LynxEnv.h"
#import "LynxUnitTestUtils.h"
#import "LynxView.h"
#import "LynxViewClientV2.h"

@interface LynxViewClientV2Tester : NSObject <LynxViewLifecycleV2>

@property(readwrite) LynxView* lynxView;

@property(readwrite) XCTestExpectation* onPageStartedExpectation;
@property(readwrite) NSArray* onPageStartedExpectationArray;

- (instancetype)initWithLynxView:(nullable LynxView*)view;

@end

@implementation LynxViewClientV2Tester

- (instancetype)initWithLynxView:(nullable LynxView*)view {
  self = [super init];
  if (self) {
    [self setLynxView:view];
  }
  return self;
}

- (void)onPageStartedWithLynxView:(nonnull LynxView*)lynxView
                 withPipelineInfo:(nonnull LynxPipelineInfo*)info {
  XCTAssertTrue(_lynxView == lynxView);
  XCTAssertEqualObjects([_lynxView url], [info url]);
  static int count = 0;
  XCTAssertTrue([_onPageStartedExpectationArray[count++] integerValue] & [info pipelineOrigin]);
  [_onPageStartedExpectation fulfill];
}

@end

@interface LynxViewClientUnitTest : LynxUnitTest

@end

@implementation LynxViewClientUnitTest

- (void)setUp {
  [super setUp];
}

- (void)testAddLynxLifecycle {
  LynxViewClientV2Tester* clientA = [[LynxViewClientV2Tester alloc] initWithLynxView:nil];
  LynxViewClientV2Tester* clientB = [[LynxViewClientV2Tester alloc] initWithLynxView:nil];

  [[LynxEnv sharedInstance].lifecycleDispatcher addLifecycleClient:clientA];
  LynxView* lynxView = [[LynxView alloc] initWithBuilderBlock:^(LynxViewBuilder* builder){
  }];
  [lynxView addLifecycleClient:clientB];

  NSSet* expectedClients = [NSSet setWithObjects:clientA, clientB, nil];
  NSMutableSet* rawClients = [NSMutableSet set];
  [[[lynxView getLifecycleDispatcher] lifecycleClients]
      enumerateObjectsUsingBlock:^(id _Nonnull client, NSUInteger idx, BOOL* _Nonnull stop) {
        if ([client isKindOfClass:LynxLifecycleDispatcher.class]) {
          [rawClients addObjectsFromArray:[client lifecycleClients]];
        } else {
          [rawClients addObject:client];
        }
      }];
  XCTAssertEqualObjects(expectedClients, rawClients);
}

- (void)testOnPageStarted {
  LynxView* lynxView = [[LynxView alloc] initWithBuilderBlock:^(LynxViewBuilder* builder){
  }];

  LynxViewClientV2Tester* client = [[LynxViewClientV2Tester alloc] initWithLynxView:lynxView];
  XCTestExpectation* onPageStartedExpectation = [self expectationWithDescription:@"onPageStarted"];
  [onPageStartedExpectation setExpectedFulfillmentCount:3];
  [client setOnPageStartedExpectation:onPageStartedExpectation];
  [client
      setOnPageStartedExpectationArray:@[ @(LynxFirstScreen), @(LynxReload), @(LynxFirstScreen) ]];
  [lynxView addLifecycleClient:client];

  NSString* url = @"hello-world.js";
  NSData* data = [LynxUnitTest getNSData:url];
  [lynxView loadTemplate:data withURL:url];
  [lynxView reloadTemplateWithTemplateData:[[LynxTemplateData alloc] initWithDictionary:@{}]];
  [lynxView loadTemplate:data withURL:url];

  [self waitForExpectationsWithTimeout:5 handler:nil];
}
@end
