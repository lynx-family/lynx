// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxHtmlEscape.h>
#import <XCTest/XCTest.h>

@interface LynxHtmlEscapeUnitTest : XCTestCase

@end

@implementation LynxHtmlEscapeUnitTest

- (void)setUp {
  // Put setup code here. This method is called before the invocation of each test method in the
  // class.
}

- (void)tearDown {
  // Put teardown code here. This method is called after the invocation of each test method in the
  // class.
}

- (void)testStringByUnescapingFromHtml {
  XCTAssertEqualObjects([@"x &gt; 10" stringByUnescapingFromHtml], @"x > 10");
  XCTAssertEqualObjects([@"<h1>Pride &amp; Prejudice</h1>" stringByUnescapingFromHtml],
                        @"<h1>Pride & Prejudice</h1>");
}

@end
