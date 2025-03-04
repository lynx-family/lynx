// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LBSCoreGraphicsPathParser.h"

#import <math.h>
#import <stdio.h>
#import <stdlib.h>

#import <CoreGraphics/CGPath.h>
#import "LBSPathParser.h"

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

static void LBSArcToBezier(CGMutablePathRef p, double cx, double cy, double a, double b, double e1x,
                           double e1y, double theta, double start, double sweep) {
  // Maximum of 45 degrees per cubic Bezier segment
  int numSegments = (int)ceil(fabs(sweep * 4 / M_PI));

  double eta1 = start;
  double cosTheta = cos(theta);
  double sinTheta = sin(theta);
  double cosEta1 = cos(eta1);
  double sinEta1 = sin(eta1);
  double ep1x = (-a * cosTheta * sinEta1) - (b * sinTheta * cosEta1);
  double ep1y = (-a * sinTheta * sinEta1) + (b * cosTheta * cosEta1);

  double anglePerSegment = sweep / numSegments;
  for (int i = 0; i < numSegments; i++) {
    double eta2 = eta1 + anglePerSegment;
    double sinEta2 = sin(eta2);
    double cosEta2 = cos(eta2);
    double e2x = cx + (a * cosTheta * cosEta2) - (b * sinTheta * sinEta2);
    double e2y = cy + (a * sinTheta * cosEta2) + (b * cosTheta * sinEta2);
    double ep2x = -a * cosTheta * sinEta2 - b * sinTheta * cosEta2;
    double ep2y = -a * sinTheta * sinEta2 + b * cosTheta * cosEta2;
    double tanDiff2 = tan((eta2 - eta1) / 2);
    double alpha = sin(eta2 - eta1) * (sqrt(4 + (3 * tanDiff2 * tanDiff2)) - 1) / 3;
    double q1x = e1x + alpha * ep1x;
    double q1y = e1y + alpha * ep1y;
    double q2x = e2x - alpha * ep2x;
    double q2y = e2y - alpha * ep2y;
    CGPathAddCurveToPoint(p, NULL, q1x, q1y, q2x, q2y, e2x, e2y);
    eta1 = eta2;
    e1x = e2x;
    e1y = e2y;
    ep1x = ep2x;
    ep1y = ep2y;
  }
}

static void LBSCoreGraphicsEllipticTo(void *ctx, float cpx, float cpy, float a, float b,
                                      float theta, bool isLarge, bool isSweep, float x, float y) {
  float thetaD = M_PI * theta / 180.f;
  float cosTheta = cosf(thetaD);
  float sinTheta = sinf(thetaD);
  float x0p = (cpx * cosTheta + cpy * sinTheta) / a;
  float y0p = (-cpx * sinTheta + cpy * cosTheta) / b;
  float x1p = (x * cosTheta + y * sinTheta) / a;
  float y1p = (-x * sinTheta + y * cosTheta) / b;

  float dx = x0p - x1p;
  float dy = y0p - y1p;
  float xm = (x0p + x1p) / 2;
  float ym = (y0p + y1p) / 2;

  float dCircle = dx * dx + dy * dy;
  if (fabsf(dCircle) < 1e-6) {
    // Path parse error in elliptical arc: all points are coincident
    return;
  }
  float disc = 1.0f / dCircle - 1.0f / 4.0f;
  if (disc < 0) {
    float adjust = sqrtf(dCircle) / 1.99999f;
    LBSCoreGraphicsEllipticTo(ctx, cpx, cpy, a * adjust, b * adjust, theta, isLarge, isSweep, x, y);
    return;
  }
  float s = sqrtf(disc);
  float sDx = s * dx;
  float sDy = s * dy;
  float cx;
  float cy;
  if (isLarge == isSweep) {
    cx = xm - sDy;
    cy = ym + sDx;
  } else {
    cx = xm + sDy;
    cy = ym - sDx;
  }
  float eta0 = atan2((y0p - cy), (x0p - cx));
  float eta1 = atan2((y1p - cy), (x1p - cx));
  float sweep = (eta1 - eta0);
  if (isSweep != (sweep >= 0)) {
    if (sweep > 0) {
      sweep -= 2 * M_PI;
    } else {
      sweep += 2 * M_PI;
    }
  }
  cx *= a;
  cy *= b;
  double tCx = cx;
  cx = cx * cosTheta - cy * sinTheta;
  cy = tCx * sinTheta + cy * cosTheta;
  LBSArcToBezier(ctx, cx, cy, a, b, cpx, cpy, thetaD, eta0, sweep);
}

static void LBSCoreGraphicsClosePath(void *ctx) { CGPathCloseSubpath(ctx); }

CGPathRef LBSCreatePathFromData(const char *data) {
  LBSPathConsumer *consumer = LBSMakeCGPathConsumer();
  LBSParsePathWithConsumer(data, consumer);
  CGPathRef path = consumer->error == 0 ? CGPathRetain(consumer->ctx) : NULL;
  LBSReleaseCGPathConsumer(consumer);
  return path;
}
