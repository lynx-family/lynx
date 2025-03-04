// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#import <Lynx/LynxError.h>
#import <XCTest/XCTest.h>

NSString* testErrorMessage = @"some error occurred";
NSString* testErrorFixSuggestion = @"some fix suggestion of this error";
static NSString* const testErrorContextInfoKey1 = @"lynx_context_test1";
static NSString* const testErrorContextInfoValue1 = @"test context info1";
static NSString* const testErrorContextInfoKey2 = @"lynx_context_test2";
static NSString* const testErrorContextInfoValue2 = @"test context info1";

@interface LynxErrorUnitTest : XCTestCase

@end
@implementation LynxErrorUnitTest {
  LynxError* _error;
}

- (void)setUp {
  NSDictionary* customInfo = [NSDictionary
      dictionaryWithObjectsAndKeys:@"some info1", @"info1", @"some info2", @"info2", nil];
  _error = [LynxError lynxErrorWithCode:301
                                message:testErrorMessage
                          fixSuggestion:testErrorFixSuggestion
                                  level:LynxErrorLevelWarn
                             customInfo:customInfo];
}

- (void)tearDown {
}

- (void)testIsValid {
  // test valid error
  XCTAssertTrue([_error isValid]);

  // test invalid error
  LynxError* error2 = [LynxError lynxErrorWithCode:301
                                           message:@""
                                     fixSuggestion:testErrorFixSuggestion
                                             level:LynxErrorLevelWarn
                                        customInfo:nil];
  XCTAssertFalse([error2 isValid]);
}

- (void)testGenerateJsonStr {
  // test override NSError's userInfo
  NSError* nsError = _error;

  NSString* errorJson = [[nsError userInfo] objectForKey:LynxErrorUserInfoKeyMessage];
  NSError* parseError = nil;
  id jsonObject =
      [NSJSONSerialization JSONObjectWithData:[errorJson dataUsingEncoding:NSUTF8StringEncoding]
                                      options:kNilOptions
                                        error:&parseError];

  // test generate json string with base error info
  if ([jsonObject isKindOfClass:[NSDictionary class]]) {
    NSDictionary* dictionary = (NSDictionary*)jsonObject;
    XCTAssertTrue([[dictionary objectForKey:@"error"] isEqualToString:testErrorMessage]);
    XCTAssertTrue([[dictionary objectForKey:@"error_code"] isEqualToNumber:@301]);
    XCTAssertTrue(
        [[dictionary objectForKey:@"fix_suggestion"] isEqualToString:testErrorFixSuggestion]);
    XCTAssertTrue([[dictionary objectForKey:@"info1"] isEqualToString:@"some info1"]);
    XCTAssertTrue([[dictionary objectForKey:@"info2"] isEqualToString:@"some info2"]);
    XCTAssertTrue([[dictionary objectForKey:@"level"] isEqualToString:@"warn"]);
  } else {
    XCTFail(@"Failed to parse error message to json");
  }

  // test regenereate error message after append additional info
  _error.templateUrl = @"template url";
  _error.cardVersion = @"0.0.1";
  _error.callStack = @"call stack";
  [_error addCustomInfo:@"some new info" forKey:@"info3"];
  [_error addCustomInfo:testErrorContextInfoValue1 forKey:testErrorContextInfoKey1];
  [_error addCustomInfo:testErrorContextInfoValue2 forKey:testErrorContextInfoKey2];
  NSString* errorJson2 = [[nsError userInfo] objectForKey:LynxErrorUserInfoKeyMessage];
  NSError* parseError2 = nil;
  id jsonObject2 =
      [NSJSONSerialization JSONObjectWithData:[errorJson2 dataUsingEncoding:NSUTF8StringEncoding]
                                      options:kNilOptions
                                        error:&parseError2];
  if ([jsonObject2 isKindOfClass:[NSDictionary class]]) {
    NSDictionary* dictionary2 = (NSDictionary*)jsonObject2;
    XCTAssertTrue([[dictionary2 objectForKey:@"url"] isEqualToString:@"template url"]);
    XCTAssertTrue([[dictionary2 objectForKey:@"card_version"] isEqualToString:@"0.0.1"]);
    XCTAssertTrue([[dictionary2 objectForKey:@"error_stack"] isEqualToString:@"call stack"]);
    XCTAssertTrue([[dictionary2 objectForKey:@"info3"] isEqualToString:@"some new info"]);
    // test generate context field
    id contextField = [dictionary2 objectForKey:@"context"];
    XCTAssertTrue([contextField isKindOfClass:[NSDictionary class]]);
    XCTAssertTrue([[contextField objectForKey:testErrorContextInfoKey1]
        isEqualToString:testErrorContextInfoValue1]);
    XCTAssertTrue([[contextField objectForKey:testErrorContextInfoKey2]
        isEqualToString:testErrorContextInfoValue2]);
  } else {
    XCTFail(@"Failed to parse error message to json");
  }
}

@end
