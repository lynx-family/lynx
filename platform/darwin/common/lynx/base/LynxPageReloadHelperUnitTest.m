// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <XCTest/XCTest.h>
#import "LynxPageReloadHelper+Internal.h"
#import "LynxPageReloadHelper.h"
#import "LynxTemplateData.h"
#import "LynxView.h"

@interface LynxPageReloadHelperUnitTest : XCTestCase

@end

@implementation LynxPageReloadHelperUnitTest {
  LynxPageReloadHelper* _reloader;
  NSString* _content;
}

- (void)setUp {
  // Put setup code here. This method is called before the invocation of each test method in the
  // class.
  _reloader = [[LynxPageReloadHelper alloc] initWithLynxView:[[LynxView alloc] init]];
  _content = @"Hello Lynx";
  NSData* data = [_content dataUsingEncoding:NSUTF8StringEncoding];
  [_reloader loadFromLocalFile:data
                       withURL:@"UnitTest"
                      initData:[[LynxTemplateData alloc] initWithDictionary:nil]];
}

- (void)tearDown {
  // Put teardown code here. This method is called after the invocation of each test method in the
  // class.
  _reloader = nil;
  _content = nil;
}

- (void)testGetTemplateJs {
  NSString* content = [_reloader getTemplateJsInfo:0 withSize:1024];
  NSData* data = [[NSData alloc] initWithBase64EncodedString:content options:0];

  NSString* result = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];

  XCTAssertTrue([result compare:_content] == NSOrderedSame);
}

@end
