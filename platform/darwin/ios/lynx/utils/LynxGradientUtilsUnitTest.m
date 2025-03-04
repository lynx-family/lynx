// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxCSSType.h>
#import <Lynx/LynxGradient.h>
#import <Lynx/LynxGradientUtils.h>
#import <XCTest/XCTest.h>

@interface LynxGradientUtilsUnitTest : XCTestCase

@end

@implementation LynxGradientUtilsUnitTest

- (void)setUp {
}

- (void)tearDown {
  // Put teardown code here. This method is called after the invocation of each test method in the
  // class.
}

- (void)testParseLinearGradient {
  NSString* gradientDef = @"linear-gradient(to right, red 25%, blue 75%)";
  struct LynxLengthContext context = (struct LynxLengthContext){100.f, .font_scale_sp_only = true};
  NSArray* array = [LynxGradientUtils getGradientArrayFromString:gradientDef
                                               withLengthContext:context];
  // [type, gradient_data]
  XCTAssertEqual(2, [array count]);
  XCTAssertEqual(LynxBackgroundImageLinearGradient, [[array objectAtIndex:0] intValue]);
  XCTAssertTrue([[array objectAtIndex:1] isKindOfClass:[NSArray class]]);
  // [direction degree, color array, position array, direction type]
  NSArray* gradientArray = [array objectAtIndex:1];
  XCTAssertEqual(4, [gradientArray count]);
  XCTAssertEqual([[gradientArray objectAtIndex:3] intValue], LynxLinearGradientDirectionToRight);
  XCTAssertTrue([[gradientArray objectAtIndex:1] isKindOfClass:[NSArray class]]);
  NSArray* colorArray = [gradientArray objectAtIndex:1];
  NSArray* expectedColor = @[ @0xffff0000, @0xff0000ff ];
  XCTAssertTrue([colorArray isEqualToArray:expectedColor]);
  XCTAssertTrue([[gradientArray objectAtIndex:2] isKindOfClass:[NSArray class]]);
  NSArray* positionArray = [gradientArray objectAtIndex:2];
  NSArray* expectedPosition = @[ @25, @75 ];
  XCTAssertTrue([positionArray isEqualToArray:expectedPosition]);
}

- (void)testParseRadialGradient {
  NSString* gradientDef = @"radial-gradient(ellipse 10px 5px at top, red, transparent)";
  struct LynxLengthContext context =
      (struct LynxLengthContext){100.f, .layouts_unit_per_px = 1,
                                 .physical_pixels_per_layout_unit = 1, .font_scale_sp_only = true};
  NSArray* array = [LynxGradientUtils getGradientArrayFromString:gradientDef
                                               withLengthContext:context];
  XCTAssertNotEqual(0, [array count]);
  // [type, gradient_data]
  XCTAssertEqual(LynxBackgroundImageRadialGradient, [[array objectAtIndex:0] intValue]);
  XCTAssertTrue([[array objectAtIndex:1] isKindOfClass:[NSArray class]]);
  NSArray* gradientArray = [array objectAtIndex:1];
  // [shape array, color array, position array]
  XCTAssertTrue([[gradientArray objectAtIndex:0] isKindOfClass:[NSArray class]]);
  NSArray* shapeArray = [gradientArray objectAtIndex:0];
  /**
     0 - shape
     1 - shape size type
     2 - posXVal
     3 - posXType
     4 - posYVal
     5 - posYType
     6 - sizeX pattern
     7 - sizeX val
     8 - sizeY pattern
     9 - sizeY val
     10 - computed sizeX val
     11 - computed sizeX unit
     12 - computed sizeY val
     13 - computed sizeY unit
   */
  NSArray* expectedShape = @[
    @(LynxRadialGradientShapeEllipse), @(LynxRadialGradientSizeLength),
    @(-LynxBackgroundPositionCenter), @(LynxBackgroundPositionCenter),
    @(-LynxBackgroundPositionTop), @(LynxBackgroundPositionTop), @5 /*PX*/, @10, @5, @5, @10, @0,
    @5, @0
  ];
  XCTAssertTrue([shapeArray isEqualToArray:expectedShape]);

  NSArray* expectedColor = @[ @0xffff0000, @0x00000000 ];
  NSArray* colorArray = [gradientArray objectAtIndex:1];
  XCTAssertTrue([colorArray isEqualToArray:expectedColor]);
  NSArray* positionArray = [gradientArray objectAtIndex:2];
  XCTAssertEqual([positionArray count], 0);
}

- (void)testInvalid {
  NSString* gradientDef = @"unknown-gradient(ellipse 10px 5px )";
  struct LynxLengthContext context =
      (struct LynxLengthContext){100.f, .layouts_unit_per_px = 1,
                                 .physical_pixels_per_layout_unit = 1, .font_scale_sp_only = true};
  NSArray* array = [LynxGradientUtils getGradientArrayFromString:gradientDef
                                               withLengthContext:context];
  XCTAssertNil(array);
}

@end
