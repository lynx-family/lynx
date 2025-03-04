// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <DebugRouter/DebugRouter.h>
#import <Lynx/LynxMemoryListener.h>
#import <OCMock/OCMock.h>
#import <XCTest/XCTest.h>
#import "LynxMemoryController.h"

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

@end
