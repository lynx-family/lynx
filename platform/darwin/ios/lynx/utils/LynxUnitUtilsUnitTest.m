// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxScreenMetrics.h>
#import <Lynx/LynxUnitUtils.h>
#import <XCTest/XCTest.h>

@interface LynxUnitUtilsUnitTest : XCTestCase

@end

@implementation LynxUnitUtilsUnitTest

- (void)setUp {
  // Put setup code here. This method is called before the invocation of each test method in the
  // class.
  LynxScreenMetrics* defaultScreenMetrics = [LynxScreenMetrics getDefaultLynxScreenMetrics];
  [defaultScreenMetrics setScreenSize:CGSizeMake(414, 896)];
  [defaultScreenMetrics setScale:2];
}

- (void)tearDown {
  // Put teardown code here. This method is called after the invocation of each test method in the
  // class.
}

- (BOOL)floatEqualFirst:(CGFloat)first toSecond:(CGFloat)second {
  return fabs(first - second) < 0.00001;
}

- (void)testToPtFromUnitValue {
  CGFloat ptValue = 0;

  ptValue = [LynxUnitUtils toPtFromUnitValue:@"375rpx"];
  XCTAssertTrue([self floatEqualFirst:ptValue toSecond:207]);

  ptValue = [LynxUnitUtils toPtFromUnitValue:@"105.4ppx"];
  XCTAssertTrue([self floatEqualFirst:ptValue toSecond:52.7]);

  ptValue = [LynxUnitUtils toPtFromUnitValue:@"105.7px"];
  XCTAssertTrue([self floatEqualFirst:ptValue toSecond:105.7]);

  ptValue = [LynxUnitUtils toPtFromUnitValue:@"105.7"];
  XCTAssertTrue([self floatEqualFirst:ptValue toSecond:105.7]);

  ptValue = [LynxUnitUtils toPtFromUnitValue:@"error"];
  XCTAssertTrue([self floatEqualFirst:ptValue toSecond:0]);
}

- (void)disable_testToPtFromUnitValueWithDefaultPt {
  // Default value is not work!
  CGFloat ptValue = 0;

  ptValue = [LynxUnitUtils toPtFromUnitValue:@"errorpx" withDefaultPt:10];
  XCTAssertTrue([self floatEqualFirst:ptValue toSecond:10]);

  ptValue = [LynxUnitUtils toPtFromUnitValue:@"error" withDefaultPt:10];
  XCTAssertTrue([self floatEqualFirst:ptValue toSecond:10]);

  ptValue = [LynxUnitUtils toPtFromUnitValue:@"" withDefaultPt:10];
  XCTAssertTrue([self floatEqualFirst:ptValue toSecond:10]);
}

- (void)testToPtFromUnitValueWithFontSize {
  CGFloat ptValue = 0;

  ptValue = [LynxUnitUtils toPtFromUnitValue:@"1.3em"
                                rootFontSize:100
                                 curFontSize:50
                                   rootWidth:0
                                  rootHeight:0];
  XCTAssertTrue([self floatEqualFirst:ptValue toSecond:65]);

  ptValue = [LynxUnitUtils toPtFromUnitValue:@"1.8rem"
                                rootFontSize:100
                                 curFontSize:50
                                   rootWidth:0
                                  rootHeight:0];
  XCTAssertTrue([self floatEqualFirst:ptValue toSecond:180]);
}

- (void)testToPtFromUnitValueWithHeightWidth {
  CGFloat ptValue = 0;

  ptValue = [LynxUnitUtils toPtFromUnitValue:@"10vh"
                                rootFontSize:100
                                 curFontSize:50
                                   rootWidth:0
                                  rootHeight:100];
  XCTAssertTrue([self floatEqualFirst:ptValue toSecond:10]);

  ptValue = [LynxUnitUtils toPtFromUnitValue:@"10vw"
                                rootFontSize:100
                                 curFontSize:50
                                   rootWidth:100
                                  rootHeight:0];
  XCTAssertTrue([self floatEqualFirst:ptValue toSecond:10]);
}

- (void)testToPtFromUnitValueWithScreenMetrics {
  CGFloat ptValue = 0;
  LynxScreenMetrics* screenMertics = [[LynxScreenMetrics alloc] init];
  [screenMertics setScreenSize:CGSizeMake(828, 1338)];
  [screenMertics setScale:3];

  ptValue = [LynxUnitUtils toPtWithScreenMetrics:screenMertics
                                       unitValue:@"375rpx"
                                    rootFontSize:0
                                     curFontSize:0
                                       rootWidth:0
                                      rootHeight:0
                                   withDefaultPt:0];
  XCTAssertTrue([self floatEqualFirst:ptValue toSecond:414]);

  ptValue = [LynxUnitUtils toPtWithScreenMetrics:screenMertics
                                       unitValue:@"105.4ppx"
                                    rootFontSize:0
                                     curFontSize:0
                                       rootWidth:0
                                      rootHeight:0
                                   withDefaultPt:0];
  XCTAssertTrue([self floatEqualFirst:ptValue toSecond:35.133334]);
}

@end
