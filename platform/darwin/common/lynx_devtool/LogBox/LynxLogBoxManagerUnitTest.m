// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <XCTest/XCTest.h>
#import <objc/message.h>
#import "LynxLogBoxManager.h"

static NSString* kNativeError =
    @"{\"sdk\":\"1.0.0\",\"level\":\"error\",\"card_version\":\"error\",\"url\":\"xxx\",\"error\":"
    @"\"test error\\n\",\"error_code\":100}";
static NSString* kJSError =
    @"{\"sdk\":\"1.0.0\",\"level\":\"error\",\"url\":\"xxx\",\"error\":\"{\\\"sentry\\\":{"
    @"\\\"exception\\\":{\\\"values\\\":[]},\\\"level\\\":\\\"error\\\",\\\"sdk\\\":{\\\"name\\\":"
    @"\\\"sentry.javascript.lynx\\\",\\\"packages\\\":[{\\\"name\\\":\\\"\\\",\\\"version\\\":"
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
    @"\\\"sentry.javascript.lynx\\\",\\\"packages\\\":[{\\\"name\\\":\\\"\\\",\\\"version\\\":"
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

@interface LynxLogBoxManagerUnitTest : XCTestCase
@end

@implementation LynxLogBoxManagerUnitTest

- (void)testExtractBriefMessage {
  Class managerClass = NSClassFromString(@"LynxLogBoxManager");
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
}

@end
