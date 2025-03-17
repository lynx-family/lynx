// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxColorUtils.h>
#import <XCTest/XCTest.h>

@interface LynxColorUnitsUnitTest : XCTestCase

@end

@implementation LynxColorUnitsUnitTest

- (void)setUp {
  // Put setup code here. This method is called before the invocation of each test method in the
  // class.
}

- (void)tearDown {
  // Put teardown code here. This method is called after the invocation of each test method in the
  // class.
}

- (NSString *)representInHex:(UIColor *)color {
  const CGFloat *components = CGColorGetComponents(color.CGColor);
  size_t count = CGColorGetNumberOfComponents(color.CGColor);
  XCTAssertEqual(count, 4);
  return [[NSString stringWithFormat:@"#%02lX%02lX%02lX%02lX", lroundf(components[0] * 255.0),
                                     lroundf(components[1] * 255.0), lroundf(components[2] * 255.0),
                                     lroundf(components[3] * 255.0)] lowercaseString];
}

- (void)testConvertNSStringToUIColor {
  UIColor *color = nil;

  color = [LynxColorUtils convertNSStringToUIColor:@"#cc9966"];
  XCTAssertEqualObjects([self representInHex:color], @"#cc9966ff");

  color = [LynxColorUtils convertNSStringToUIColor:@"#c96"];
  XCTAssertEqualObjects([self representInHex:color], @"#cc9966ff");

  color = [LynxColorUtils convertNSStringToUIColor:@"#c963"];
  XCTAssertEqualObjects([self representInHex:color], @"#cc996633");

  color = [LynxColorUtils convertNSStringToUIColor:@"#cc996633"];
  XCTAssertEqualObjects([self representInHex:color], @"#cc996633");

  color = [LynxColorUtils convertNSStringToUIColor:@"rgba(255, 0, 0, 0.3)"];
  XCTAssertEqualObjects([self representInHex:color], @"#ff00004d");

  color = [LynxColorUtils convertNSStringToUIColor:@"rgb(255, 0, 0)"];
  XCTAssertEqualObjects([self representInHex:color], @"#ff0000ff");

  color = [LynxColorUtils convertNSStringToUIColor:@"hsl(120, 100%, 50%)"];
  XCTAssertEqualObjects([self representInHex:color], @"#00ff00ff");

  color = [LynxColorUtils convertNSStringToUIColor:@"hsla(120, 100%, 50%, 0.3)"];
  XCTAssertEqualObjects([self representInHex:color], @"#00ff004d");

  color = [LynxColorUtils convertNSStringToUIColor:@"aqua"];
  XCTAssertEqualObjects([self representInHex:color], @"#00ffffff");

  color = [LynxColorUtils convertNSStringToUIColor:@"errorColor"];
  XCTAssertNil(color);
}

@end
