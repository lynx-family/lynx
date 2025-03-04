// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <LynxDevtool/LynxDevToolErrorUtils.h>
#import <XCTest/XCTest.h>

static int64_t const kLevelWarn = 1;
static int64_t const kLevelError = 2;

@interface LynxDevToolErrorUtilsUnitTest : XCTestCase
@end

@implementation LynxDevToolErrorUtilsUnitTest

- (void)testGetKeyMessage {
  // test error with message
  LynxError *errorWithMsg = [LynxError lynxErrorWithCode:1
                                                 message:@"test"
                                           fixSuggestion:@"test"
                                                   level:LynxErrorLevelWarn];
  NSString *keyMessage = [LynxDevToolErrorUtils getKeyMessage:errorWithMsg];
  XCTAssertEqualObjects(@"code: 1\nmessage: test\nfix_suggestion: test", keyMessage);

  // test error is null
  NSString *keyMessageNil = [LynxDevToolErrorUtils getKeyMessage:nil];
  XCTAssertEqualObjects(@"", keyMessageNil);

  // test error is invalid
  LynxError *errorInvalid = [LynxError lynxErrorWithCode:1
                                                 message:@""
                                           fixSuggestion:@"test"
                                                   level:@"test"];
  NSString *keyMessageInvalid = [LynxDevToolErrorUtils getKeyMessage:errorInvalid];
  XCTAssertEqualObjects(@"", keyMessageInvalid);
}

- (void)testErrorLevelStrToInt {
  // test error level is empty string
  int32_t levelEmpty = [LynxDevToolErrorUtils intValueFromErrorLevelString:@""];
  XCTAssertEqual(levelEmpty, kLevelError);

  // test error level is nil
  int32_t levelNil = [LynxDevToolErrorUtils intValueFromErrorLevelString:nil];
  XCTAssertEqual(levelNil, kLevelError);

  // test error level is error
  int32_t levelError = [LynxDevToolErrorUtils intValueFromErrorLevelString:@"error"];
  XCTAssertEqual(levelError, kLevelError);

  // test error level is warn
  int32_t levelWarn = [LynxDevToolErrorUtils intValueFromErrorLevelString:@"warn"];
  XCTAssertEqual(levelWarn, kLevelWarn);
}

@end
