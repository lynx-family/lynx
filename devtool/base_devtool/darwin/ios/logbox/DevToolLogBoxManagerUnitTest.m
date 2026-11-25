// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <XCTest/XCTest.h>
#import <objc/message.h>
#import "DevToolLogBoxManager.h"

static NSString* kNativeError =
    @"{\"sdk\":\"1.0.0\",\"level\":\"error\",\"card_version\":\"error\",\"url\":\"xxx\",\"error\":"
    @"\"test error\\n\",\"error_code\":100}";
static NSString* kJSError =
    @"{\"sdk\":\"1.0.0\",\"level\":\"error\",\"url\":\"xxx\",\"error\":\"{\\\"sentry\\\":{"
    @"\\\"exception\\\":{\\\"values\\\":[]},\\\"level\\\":\\\"error\\\",\\\"sdk\\\":{\\\"name\\\":"
    @"\\\"sentry.javascript.browser\\\",\\\"packages\\\":[{\\\"name\\\":\\\"\\\",\\\"version\\\":"
    @"\\\"1.0.0\\\"}],\\\"version\\\":\\\"1.0.0\\\",\\\"integrations\\\":[]},\\\"platform\\\":"
    @"\\\"javascript\\\",\\\"tags\\\":{\\\"lib_version\\\":\\\"\\\",\\\"version_code\\\":"
    @"\\\"unknown_version\\\",\\\"run_type\\\":\\\"lynx_core\\\",\\\"extra\\\":\\\"at Card onLoad "
    @"\\\\ntest "
    @"error\\\",\\\"error_type\\\":\\\"USER_RUNTIME_ERROR\\\"}},\\\"pid\\\":\\\"USER_ERROR\\\","
    @"\\\"url\\\":\\\"file://"
    @"lynx_core.js\\\",\\\"rawError\\\":{\\\"stack\\\":\\\"\\\",\\\"message\\\":\\\"at Card onLoad "
    @"\\\\ntest error\\\"}}\",\"error_code\":201}";
static NSString* kJSErrorMissingMessageField =
    @"{\"sdk\":\"1.0.0\",\"level\":\"error\",\"url\":\"xxx\",\"error\":\"{\\\"sentry\\\":{"
    @"\\\"exception\\\":{\\\"values\\\":[]},\\\"level\\\":\\\"error\\\",\\\"sdk\\\":{\\\"name\\\":"
    @"\\\"sentry.javascript.browser\\\",\\\"packages\\\":[{\\\"name\\\":\\\"\\\",\\\"version\\\":"
    @"\\\"1.0.0\\\"}],\\\"version\\\":\\\"1.0.0\\\",\\\"integrations\\\":[]},\\\"platform\\\":"
    @"\\\"javascript\\\",\\\"tags\\\":{\\\"lib_version\\\":\\\"\\\",\\\"version_code\\\":"
    @"\\\"unknown_version\\\",\\\"run_type\\\":\\\"lynx_core\\\",\\\"extra\\\":\\\"at Card onLoad "
    @"\\\\ntest "
    @"error\\\",\\\"error_type\\\":\\\"USER_RUNTIME_ERROR\\\"}},\\\"pid\\\":\\\"USER_ERROR\\\","
    @"\\\"url\\\":\\\"file://"
    @"lynx_core.js\\\",\\\"rawError\\\":{\\\"stack\\\":\\\"\\\"}}\",\"error_code\":201}";
static NSString* kRawMessage = @"raw error message\n";

static const NSString* kExpectBriefNativeErrorMsg = @"test error ";
static const NSString* kExpectBriefJSErrorMsg = @"at Card onLoad  test error";
static const NSString* kExpectBriefJSErrorMsgMissingMessageField =
    @"{\"sdk\":\"1.0.0\",\"level\":\"error\",\"url\":\"xxx\",\"error\"";
static const NSString* kExpectBriefRawMessage = @"raw error message ";

@interface DevToolLogBoxManagerUnitTest : XCTestCase
@end

@implementation DevToolLogBoxManagerUnitTest

- (void)testExtractBriefMessage {
  Class managerClass = NSClassFromString(@"DevToolLogBoxManager");
  SEL methodSel = NSSelectorFromString(@"extractBriefMessage:");
  if (!managerClass || !methodSel || ![managerClass respondsToSelector:methodSel]) {
    // if cannot found method, skip the test
    return;
  }
  id (*method)(Class, SEL, NSString*) = (id(*)(Class, SEL, NSString*))objc_msgSend;

  // test extract brief message from native error message
  NSString* briefNativeErrorMsg = method(managerClass, methodSel, kNativeError);
  XCTAssertTrue([briefNativeErrorMsg isEqual:kExpectBriefNativeErrorMsg]);
  // test extract brief message from js error message
  NSString* briefJSErrorMsg = method(managerClass, methodSel, kJSError);
  XCTAssertTrue([briefJSErrorMsg isEqual:kExpectBriefJSErrorMsg]);

  NSString* briefJSErrorMsgMissingMsgField =
      method(managerClass, methodSel, kJSErrorMissingMessageField);
  XCTAssertTrue([briefJSErrorMsgMissingMsgField isEqual:kExpectBriefJSErrorMsgMissingMessageField]);

  // test extract brief message from raw error message
  NSString* briefRawMessage = method(managerClass, methodSel, kRawMessage);
  XCTAssertTrue([briefRawMessage isEqual:kExpectBriefRawMessage]);
  // test pass empty string to method
  NSString* emptyRet = method(managerClass, methodSel, @"");
  XCTAssertTrue([emptyRet isEqual:@""]);

  NSArray<NSArray<NSString*>*>* cases = @[
    @[ @"[{\"error\":\"bad\"}]", @"[{\"error\":\"bad\"}]" ],
    @[ @"{\"error\":\"[{\\\"status\\\":\\\"fail\\\"}]\"}", @"[{\"status\":\"fail\"}]" ],
    @[ @"{\"error\":\"[]\"}", @"[]" ],
    @[ @"{\"error\":[{\"status\":\"fail\"}]}", @"{\"error\":[{\"status\":\"fail\"}]}" ],
    @[ @"{\"error\":{\"rawError\":[]}}", @"{\"error\":{\"rawError\":[]}}" ],
    @[
      @"{\"error\":{\"rawError\":{\"message\":42}}}", @"{\"error\":{\"rawError\":{\"message\":42}}}"
    ],
    @[
      @"{\"error\":{\"rawError\":{\"message\":null}}}",
      @"{\"error\":{\"rawError\":{\"message\":null}}}"
    ],
    @[
      @"{\"error\":{\"rawError\":{\"message\":[1]}}}",
      @"{\"error\":{\"rawError\":{\"message\":[1]}}}"
    ],
    @[
      @"{\"error\":{\"rawError\":{\"message\":{\"a\":1}}}}",
      @"{\"error\":{\"rawError\":{\"message\":{\"a\":1}}}}"
    ],
    @[ @"{\"error\":{\"rawError\":{\"message\":\"direct\"}}}", @"direct" ],
    @[ @"{\"error\":\"{\\\"rawError\\\":{\\\"message\\\":\\\"inner\\\"}}\"}", @"inner" ],
  ];
  NSString* truncationInput = [[@"x" stringByPaddingToLength:49 withString:@"x"
                                             startingAtIndex:0] stringByAppendingString:@"\ny"];
  for (NSArray<NSString*>* testCase in cases) {
    XCTAssertEqualObjects(method(managerClass, methodSel, testCase[0]), testCase[1], @"input: %@",
                          testCase[0]);
  }
  XCTAssertEqualObjects(method(managerClass, methodSel, truncationInput),
                        [[truncationInput substringToIndex:49] stringByAppendingString:@" "]);
}

@end
