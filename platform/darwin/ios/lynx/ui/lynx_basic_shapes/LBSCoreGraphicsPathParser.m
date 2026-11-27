// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LBSCoreGraphicsPathParser.h"

#import <stdio.h>
#import <stdlib.h>

#import <CoreGraphics/CGPath.h>
#include "gfx/public/path_parser.h"

// Member functions of path segment consumer.
static void LBSCoreGraphicsMoveTo(void *ctx, float x, float y);
static void LBSCoreGraphicsLineTo(void *ctx, float x, float y);
static void LBSCoreGraphicsCubicTo(void *ctx, float cp1x, float cp1y, float cp2x, float cp2y,
                                   float x, float y);
static void LBSCoreGraphicsQuadTo(void *ctx, float cpx, float cpy, float x, float y);
static void LBSCoreGraphicsEllipticTo(void *ctx, float cpx, float cpy, float a, float b,
                                      float theta, bool isMoreThanHalf, bool isPositiveArc, float x,
                                      float y);
static void LBSCoreGraphicsClosePath(void *ctx);

/**
 Creates an `LBSPathConsumer` object.

 This function creates an `LBSPathConsumer` object, which is a struct that
 represents a consumer for a CGPath. It initializes the `CGMutablePathRef`
 variable and allocates memory for the `LBSPathConsumer` struct using `malloc`.
 The struct is then initialized with the necessary function pointers and the
 created path.

 The `LBSPathConsumer` object is used to consume path elements and perform
 operations on a CGPath.

 @return An `LBSPathConsumer` object that can be used to consume path elements.
 */
LBSPathConsumer *LBSMakeCGPathConsumer(void) {
  CGMutablePathRef path = CGPathCreateMutable();
  LBSPathConsumer *consumer = (LBSPathConsumer *)malloc(sizeof(LBSPathConsumer));
  (*consumer) = (LBSPathConsumer){.type = kLBSPathConsumerTypeCoreGraphics,
                                  .ctx = path,
                                  .error = 0,
                                  .MoveToPoint = LBSCoreGraphicsMoveTo,
                                  .LineToPoint = LBSCoreGraphicsLineTo,
                                  .CubicToPoint = LBSCoreGraphicsCubicTo,
                                  .EllipticToPoint = LBSCoreGraphicsEllipticTo,
                                  .QuadToPoint = LBSCoreGraphicsQuadTo,
                                  .ClosePath = LBSCoreGraphicsClosePath};
  return consumer;
}

// Release the resources in the path segment consumer created by
// `LBSMakeCGPathConsumer`.
void LBSReleaseCGPathConsumer(LBSPathConsumer *consumer) {
  if (consumer) {
    CGPathRelease(consumer->ctx);
    free(consumer);
  }
}

static void LBSCoreGraphicsMoveTo(void *ctx, float x, float y) {
  CGPathMoveToPoint(ctx, NULL, x, y);
}

static void LBSCoreGraphicsLineTo(void *ctx, float x, float y) {
  CGPathAddLineToPoint(ctx, NULL, x, y);
}

static void LBSCoreGraphicsCubicTo(void *ctx, float cp1x, float cp1y, float cp2x, float cp2y,
                                   float x, float y) {
  CGPathAddCurveToPoint(ctx, NULL, cp1x, cp1y, cp2x, cp2y, x, y);
}

static void LBSCoreGraphicsQuadTo(void *ctx, float cpx, float cpy, float x, float y) {
  CGPathAddQuadCurveToPoint(ctx, NULL, cpx, cpy, x, y);
}

static void LBSArcLineTo(void *ctx, double x, double y) { CGPathAddLineToPoint(ctx, NULL, x, y); }

static void LBSArcCubicTo(void *ctx, double c1x, double c1y, double c2x, double c2y, double x,
                          double y) {
  CGPathAddCurveToPoint(ctx, NULL, c1x, c1y, c2x, c2y, x, y);
}

static void LBSCoreGraphicsEllipticTo(void *ctx, float cpx, float cpy, float a, float b,
                                      float theta, bool isLarge, bool isSweep, float x, float y) {
  const LBSSvgArcSink sink = {ctx, LBSArcLineTo, LBSArcCubicTo};
  LBSAppendSvgArc(&sink, cpx, cpy, a, b, theta, isLarge, isSweep, x, y);
}

static void LBSCoreGraphicsClosePath(void *ctx) { CGPathCloseSubpath(ctx); }

CGPathRef LBSCreatePathFromData(const char *data) {
  LBSPathConsumer *consumer = LBSMakeCGPathConsumer();
  LBSParsePathWithConsumer(data, consumer);
  CGPathRef path = consumer->error == 0 ? CGPathRetain(consumer->ctx) : NULL;
  LBSReleaseCGPathConsumer(consumer);
  return path;
}
