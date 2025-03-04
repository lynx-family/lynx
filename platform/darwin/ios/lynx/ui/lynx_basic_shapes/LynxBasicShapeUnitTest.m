// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <XCTest/XCTest.h>
#import <objc/runtime.h>
#import "LBSPathParser.h"
#import "LynxBackgroundUtils.h"
#import "LynxBasicShape.h"
#import "LynxCSSType.h"

static void LBSStringMoveTo(void* ctx, float x, float y) {
  char op[20];
  sprintf(op, "M %.0f %.0f ", x, y);
  strcat(ctx, op);
}

static void LBSStringLineTo(void* ctx, float x, float y) {
  char op[20];
  sprintf(op, "L %.0f %.0f ", x, y);
  strcat(ctx, op);
}

static void LBSStringCubicTo(void* ctx, float cp1x, float cp1y, float cp2x, float cp2y, float x,
                             float y) {
  char op[50];
  sprintf(op, "C %.0f %.0f %.0f %.0f %.0f %.0f ", cp1x, cp1y, cp2x, cp2y, x, y);
  strcat(ctx, op);
}
static void LBSStringQuadTo(void* ctx, float cpx, float cpy, float x, float y) {
  char op[50];
  sprintf(op, "Q %.0f %.0f %.0f %.0f ", cpx, cpy, x, y);
  strcat(ctx, op);
}
static void LBSStringEllipticTo(void* ctx, float cpx, float cpy, float a, float b, float theta,
                                bool isMoreThanHalf, bool isPositiveArc, float x, float y) {
  char op[50];
  sprintf(op, "A %.0f %.0f %.0f %d %d %.0f %.0f ", a, b, theta, isMoreThanHalf, isPositiveArc, x,
          y);
  strcat(ctx, op);
}
static void LBSStringClosePath(void* ctx) { strcat(ctx, "Z"); }

LBSPathConsumer* LBSMakeStringPathConsumer(void) {
  LBSPathConsumer* consumer = (LBSPathConsumer*)malloc(sizeof(LBSPathConsumer));
  *consumer = (LBSPathConsumer){.type = kLBSPathConsumerTypeString,
                                .ctx = (char*)malloc(512),
                                .MoveToPoint = LBSStringMoveTo,
                                .LineToPoint = LBSStringLineTo,
                                .CubicToPoint = LBSStringCubicTo,
                                .QuadToPoint = LBSStringQuadTo,
                                .EllipticToPoint = LBSStringEllipticTo,
                                .ClosePath = LBSStringClosePath};
  return consumer;
}

void LBSReleaseStringPathConsumer(LBSPathConsumer* consumer) {
  if (consumer) {
    free(consumer->ctx);
    free(consumer);
  }
}

@interface LynxBasicShape () {
 @public
  LynxBorderUnitValue* _params;
}
@end

@interface LynxBasicShapeUnitTest : XCTestCase

@end

@implementation LynxBasicShapeUnitTest

- (void)testCreateBasicShapeInset {
  NSNumber* type = [NSNumber numberWithInt:LynxBasicShapeTypeInset];
  NSArray* array = @[ type, @30, @1 ];
  LynxBasicShape* shape = LBSCreateBasicShapeFromArray(array);
  XCTAssertNil(shape);
  // rect
  array = @[ type, @30, @1, @30, @1, @30, @1, @30, @1 ];
  shape = LBSCreateBasicShapeFromArray(array);
  XCTAssertNotNil(shape);
  XCTAssertEqual(shape->_params[1].val, 30);
  XCTAssertEqual(shape->_params[1].unit, LynxBorderValueUnitPercent);
  // rounded corner
  array = @[
    type, @30, @1,  @30, @1,  @30, @1,  @30, @1,  @30, @1,  @30, @1,
    @30,  @1,  @30, @1,  @30, @1,  @30, @1,  @30, @1,  @30, @1
  ];
  shape = LBSCreateBasicShapeFromArray(array);
  XCTAssertNotNil(shape);
  // super ellipse corner
  array = @[
    type, @30, @1, @30, @1, @30, @1, @30, @1, @3,  @3, @30, @1, @30,
    @1,   @30, @1, @30, @1, @30, @1, @30, @1, @30, @1, @30, @1
  ];
  shape = LBSCreateBasicShapeFromArray(array);

  XCTAssertNotNil(shape);
  // Test no object released.
  __weak LynxBasicShape* ws = shape;
  [self addTeardownBlock:^{
    XCTAssertNil(ws);
  }];
}

- (void)testCreateBasicShapeCircle {
  NSNumber* type = [NSNumber numberWithInt:LynxBasicShapeTypeCircle];
  // Test invalid
  NSArray<NSNumber*>* array = @[ type, @50, @1 ];
  LynxBasicShape* shape = LBSCreateBasicShapeFromArray(array);
  XCTAssertNil(shape);
  // test valid
  // circle(50% at center top)
  array = @[ type, @50, @1, @50, @1, @0, @0 ];
  shape = LBSCreateBasicShapeFromArray(array);
  XCTAssertNotNil(shape);
  XCTAssertEqual(shape->_params[0].val, 50);
  XCTAssertEqual(shape->_params[0].unit, LynxBorderValueUnitPercent);
  XCTAssertEqual(shape->_params[1].val, 50);
  XCTAssertEqual(shape->_params[1].unit, LynxBorderValueUnitPercent);
  XCTAssertEqual(shape->_params[2].val, 0);
  XCTAssertEqual(shape->_params[2].unit, LynxBorderValueUnitDefault);
  // test dealloc
  __weak LynxBasicShape* ws = shape;
  [self addTeardownBlock:^{
    XCTAssertNil(ws);
  }];
}

- (void)testPathParser {
  {
    LBSPathConsumer* consumer = LBSMakeStringPathConsumer();
    LBSParsePathWithConsumer("M 10,50"
                             "Q 25,25 40,50"
                             "t 30,0 30,0",
                             consumer);
    // The control point is the reflection of the control point of the previous curve command about
    // the current point. 2 * 40 - 25 = 55, and etc.

    const char* a = "M 10 50 Q 25 25 40 50 Q 55 75 70 50 Q 85 25 100 50 ";
    XCTAssertTrue(strcmp(a, consumer->ctx) == 0);
    LBSReleaseStringPathConsumer(consumer);
  }
  {
    LBSPathConsumer* consumer = LBSMakeStringPathConsumer();
    LBSParsePathWithConsumer("M 10,50"
                             "q 25,25 40,50"
                             "T 30,0 30,0",
                             consumer);
    // The control point is the reflection of the control point of the previous curve command about
    // the current point. 2 * 40 - 25 = 55, and etc.

    const char* a = "M 10 50 Q 35 75 50 100 Q 65 125 30 0 Q -5 -125 30 0 ";
    XCTAssertTrue(strcmp(a, consumer->ctx) == 0);
    LBSReleaseStringPathConsumer(consumer);
  }
  {
    LBSPathConsumer* consumer = LBSMakeStringPathConsumer();
    LBSParsePathWithConsumer("M 6,10"
                             "A 6 4 10 1 0 14,10",
                             consumer);
    const char* a = "M 6 10 A 6 4 10 1 0 14 10 ";

    XCTAssertTrue(strcmp(a, consumer->ctx) == 0);
    LBSReleaseStringPathConsumer(consumer);
  }
  {
    LBSPathConsumer* consumer = LBSMakeStringPathConsumer();
    LBSParsePathWithConsumer("M 6,10"
                             "a 6 4 10 1 0 14,10",
                             consumer);
    const char* a = "M 6 10 A 6 4 10 1 0 20 20 ";
    XCTAssertTrue(strcmp(a, consumer->ctx) == 0);
    LBSReleaseStringPathConsumer(consumer);
  }
  {
    LBSPathConsumer* consumer = LBSMakeStringPathConsumer();
    LBSParsePathWithConsumer("M 10,90"
                             "C 30,90 25,10 50,10"
                             "S 70,90 90,90",
                             consumer);
    //    Draw a smooth cubic Bézier curve from the current point to the end point specified by x,y.
    //    The end control point is specified by x2,y2. The start control point is the reflection of
    //    the end control point of the previous curve command about the current point.
    const char* a = "M 10 90 C 30 90 25 10 50 10 C 75 10 70 90 90 90 ";
    XCTAssertTrue(strcmp(a, consumer->ctx) == 0);
    LBSReleaseStringPathConsumer(consumer);
  }
  {
    LBSPathConsumer* consumer = LBSMakeStringPathConsumer();
    LBSParsePathWithConsumer("M 110,90"
                             "c 20,0 15,-80 40,-80"
                             "s 20,80 40,80",
                             consumer);
    //    Draw a smooth cubic Bézier curve from the current point to the end point specified by x,y.
    //    The end control point is specified by x2,y2. The start control point is the reflection of
    //    the end control point of the previous curve command about the current point.
    const char* a = "M 110 90 C 130 90 125 10 150 10 C 175 10 170 90 190 90 ";
    XCTAssertTrue(strcmp(a, consumer->ctx) == 0);
    LBSReleaseStringPathConsumer(consumer);
  }
  {
    LBSPathConsumer* consumer = LBSMakeStringPathConsumer();
    LBSParsePathWithConsumer("M 10,10"
                             "L 90,90"
                             "V 10"
                             "H 50",
                             consumer);
    const char* a = "M 10 10 L 90 90 L 90 10 L 50 10 ";
    XCTAssertTrue(strcmp(a, consumer->ctx) == 0);
    LBSReleaseStringPathConsumer(consumer);
  }
  {
    LBSPathConsumer* consumer = LBSMakeStringPathConsumer();
    LBSParsePathWithConsumer("M 110,10"
                             "l 80,80"
                             "v -80"
                             "h -40",
                             consumer);
    const char* a = "M 110 10 L 190 90 L 190 10 L 150 10 ";
    XCTAssertTrue(strcmp(a, consumer->ctx) == 0);
    LBSReleaseStringPathConsumer(consumer);
  }
}
@end
