// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxMemoryListener.h>
#import <OCMock/OCMock.h>
#import <XCTest/XCTest.h>

@interface LynxMemoryReporterTest : NSObject <LynxMemoryReporter>

- (void)uploadImageInfo:(NSDictionary*)data;

@end

@implementation LynxMemoryReporterTest

- (void)uploadImageInfo:(NSDictionary*)data {
}

@end

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

- (void)testUploadImageInfo {
  LynxMemoryReporterTest* reporter = [[LynxMemoryReporterTest alloc] init];
  _listener.memoryReporters = [[NSMutableArray alloc] init];
  NSDictionary* dictionary =
      @{@"type" : @"image", @"image_url" : @"LynxMemoryListenerUnitTest.png"};
  reporter = OCMPartialMock(reporter);
  [_listener addMemoryReporter:reporter];
  OCMExpect([reporter uploadImageInfo:[OCMArg any]]);
  [_listener uploadImageInfo:dictionary];
  OCMVerifyAll(reporter);
  [_listener removeMemoryReporter:reporter];
}

- (void)testAddMemoryReporter {
  NSMutableArray<id<LynxMemoryReporter>>* array = OCMClassMock([NSMutableArray class]);
  _listener.memoryReporters = array;
  LynxMemoryReporterTest* reporter = [[LynxMemoryReporterTest alloc] init];
  OCMExpect([array addObject:[OCMArg any]]);
  [_listener addMemoryReporter:reporter];
  OCMVerifyAll(array);
  [_listener removeMemoryReporter:reporter];
}

- (void)testRemoveMemoryReporter {
  NSMutableArray<id<LynxMemoryReporter>>* array = OCMClassMock([NSMutableArray class]);
  _listener.memoryReporters = array;
  LynxMemoryReporterTest* reporter = [[LynxMemoryReporterTest alloc] init];
  [_listener addMemoryReporter:reporter];
  OCMExpect([array removeObject:[OCMArg any]]);
  [_listener removeMemoryReporter:reporter];
  OCMVerifyAll(array);
}

@end
