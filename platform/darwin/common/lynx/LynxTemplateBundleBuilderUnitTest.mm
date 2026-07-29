// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxTemplateBundleBuilder.h>
#import <XCTest/XCTest.h>

@interface LynxTemplateBundleBuilderUnitTest : XCTestCase

@end

@implementation LynxTemplateBundleBuilderUnitTest

- (void)testBuildCreatesTemplateBundleFromMainAndBackgroundScripts {
  NSString* mainThreadScript = @"function processData(data) { return data || {}; }\n"
                                "function renderPage(data, options) { return null; }\n";
  LynxTemplateBundle* bundle = [[[[LynxTemplateBundleBuilder create] main:mainThreadScript]
      background:@"(function(){ globalThis.__builder_bts = true; })();"] build];

  XCTAssertNotNil(bundle);
  XCTAssertNil([bundle errorMsg]);
}

- (void)testBuildReturnsNilWithoutMainThreadScript {
  LynxTemplateBundle* bundle =
      [[[LynxTemplateBundleBuilder create] background:@"(function(){})()"] build];

  XCTAssertNil(bundle);
}

@end
