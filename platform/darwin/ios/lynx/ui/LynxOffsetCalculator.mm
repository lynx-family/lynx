// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#import "LynxOffsetCalculator.h"
#import <Lynx/LynxLog.h>
#import "LynxLRUMap.h"

@implementation PathLengthCache
- (instancetype)init {
  if (self = [super init]) {
    _segmentLengths = [NSMutableArray array];
  }
  return self;
}
- (void)addSegmentLength:(CGFloat)length {
  [self.segmentLengths addObject:@(length)];
}
@end

static int32_t kMaxCacheSize = 10;
static LynxLRUMap *lruPathCache;

@implementation LynxOffsetCalculator

#pragma mark - Private Methods

typedef struct {
  CGFloat totalLength;
  CGFloat targetLength;
  CGPoint currentPoint;
  CGPoint startPoint;
  CGPoint resultPoint;
  CGPoint resultPointTangent;
  BOOL isAutoRotate;
  BOOL found;
  int segments;                    // Number of sampling points used for curve segments
  void *cache;                     // save PathLengthCache
  NSUInteger currentSegmentIndex;  // save current curve index
} PathInfo;

// Get the tangent vector of the quadratic Bezier curve at position t.
static CGPoint getQuadraticBezierTangent(CGPoint start, CGPoint control, CGPoint end, CGFloat t) {
  CGFloat t1 = 1.0 - t;
  return CGPointMake(2 * t1 * (control.x - start.x) + 2 * t * (end.x - control.x),
                     2 * t1 * (control.y - start.y) + 2 * t * (end.y - control.y));
}

// Get the tangent vector of the cubic Bezier curve at position t.
static CGPoint getCubicBezierTangent(CGPoint start, CGPoint control1, CGPoint control2, CGPoint end,
                                     CGFloat t) {
  CGFloat t1 = 1.0 - t;
  return CGPointMake(3 * t1 * t1 * (control1.x - start.x) + 6 * t1 * t * (control2.x - control1.x) +
                         3 * t * t * (end.x - control2.x),
                     3 * t1 * t1 * (control1.y - start.y) + 6 * t1 * t * (control2.y - control1.y) +
                         3 * t * t * (end.y - control2.y));
}

// Get the tangent vector at t.
static CGPoint getLineTangent(CGPoint start, CGPoint end) {
  return CGPointMake(end.x - start.x, end.y - start.y);
}

// Calculate the Angle between the vector and the positive Y-axis.
static CGFloat angleWithPositiveYAxis(CGPoint vector) {
  CGPoint yAxis = CGPointMake(0, 1);
  CGFloat dotProduct = vector.x * yAxis.x + vector.y * yAxis.y;
  CGFloat magnitudeProduct = hypot(vector.x, vector.y) * hypot(yAxis.x, yAxis.y);
  CGFloat cosTheta = dotProduct / magnitudeProduct;
  CGFloat angle = acos(cosTheta);
  if (vector.x < 0) {
    angle = 2 * M_PI - angle;
  }
  return angle;
}

// Calculate the length of the quadratic Bezier curve.
static CGFloat calculateQuadraticBezierLength(CGPoint start, CGPoint control, CGPoint end,
                                              int segments) {
  CGFloat length = 0;
  CGPoint previousPoint = start;

  for (int i = 1; i <= segments; i++) {
    CGFloat t = (CGFloat)i / segments;
    CGFloat t1 = 1.0 - t;
    CGPoint point = CGPointMake(t1 * t1 * start.x + 2 * t1 * t * control.x + t * t * end.x,
                                t1 * t1 * start.y + 2 * t1 * t * control.y + t * t * end.y);
    length += hypot(point.x - previousPoint.x, point.y - previousPoint.y);
    previousPoint = point;
  }
  return length;
}

// Calculate the length of the cubic Bezier curve.
static CGFloat calculateCubicBezierLength(CGPoint start, CGPoint control1, CGPoint control2,
                                          CGPoint end, int segments) {
  CGFloat length = 0;
  CGPoint previousPoint = start;

  for (int i = 1; i <= segments; i++) {
    CGFloat t = (CGFloat)i / segments;
    CGFloat t1 = 1.0 - t;
    CGPoint point = CGPointMake(t1 * t1 * t1 * start.x + 3 * t1 * t1 * t * control1.x +
                                    3 * t1 * t * t * control2.x + t * t * t * end.x,
                                t1 * t1 * t1 * start.y + 3 * t1 * t1 * t * control1.y +
                                    3 * t1 * t * t * control2.y + t * t * t * end.y);
    length += hypot(point.x - previousPoint.x, point.y - previousPoint.y);
    previousPoint = point;
  }
  return length;
}

// Calculate the arc length.
__attribute__((unused)) static CGFloat calculateArcLength(CGPoint start, CGPoint end,
                                                          CGPoint control, CGFloat radius,
                                                          int segments) {
  CGFloat length = 0;
  CGPoint previousPoint = start;

  CGFloat ax = start.x - control.x;
  CGFloat ay = start.y - control.y;
  CGFloat bx = end.x - control.x;
  CGFloat by = end.y - control.y;
  CGFloat a = sqrt(ax * ax + ay * ay);
  CGFloat b = sqrt(bx * bx + by * by);

  CGFloat angle = acos((ax * bx + ay * by) / (a * b));
  CGFloat anglePerSegment = angle / segments;

  CGFloat startAngle = atan2(ay, ax);

  for (int i = 1; i <= segments; i++) {
    CGFloat currentAngle = startAngle + anglePerSegment * i;
    CGPoint point =
        CGPointMake(control.x + radius * cos(currentAngle), control.y + radius * sin(currentAngle));
    length += hypot(point.x - previousPoint.x, point.y - previousPoint.y);
    previousPoint = point;
  }

  return length;
}

// Get the points on the quadratic Bezier curve.
static CGPoint getQuadraticBezierPoint(CGPoint start, CGPoint control, CGPoint end, CGFloat t) {
  CGFloat t1 = 1.0 - t;
  return CGPointMake(t1 * t1 * start.x + 2 * t1 * t * control.x + t * t * end.x,
                     t1 * t1 * start.y + 2 * t1 * t * control.y + t * t * end.y);
}

// Get the points on the cubic Bezier curve.
static CGPoint getCubicBezierPoint(CGPoint start, CGPoint control1, CGPoint control2, CGPoint end,
                                   CGFloat t) {
  CGFloat t1 = 1.0 - t;
  return CGPointMake(t1 * t1 * t1 * start.x + 3 * t1 * t1 * t * control1.x +
                         3 * t1 * t * t * control2.x + t * t * t * end.x,
                     t1 * t1 * t1 * start.y + 3 * t1 * t1 * t * control1.y +
                         3 * t1 * t * t * control2.y + t * t * t * end.y);
}

// Get the points on the arc curve
__attribute__((unused)) static CGPoint getArcPoint(CGPoint start, CGPoint end, CGPoint control,
                                                   CGFloat radius, CGFloat t) {
  CGFloat ax = start.x - control.x;
  CGFloat ay = start.y - control.y;
  CGFloat bx = end.x - control.x;
  CGFloat by = end.y - control.y;
  CGFloat startAngle = atan2(ay, ax);
  CGFloat angle = acos((ax * bx + ay * by) / (sqrt(ax * ax + ay * ay) * sqrt(bx * bx + by * by)));
  CGFloat currentAngle = startAngle + angle * t;
  return CGPointMake(control.x + radius * cos(currentAngle),
                     control.y + radius * sin(currentAngle));
}

// The length of line segment is counted and the calculation result is cached
static void calculatePathLengthFunction(void *info, const CGPathElement *element) {
  PathInfo *pathInfo = (PathInfo *)info;
  CGPoint points[3];
  CGPoint lastPoint = pathInfo->currentPoint;
  CGFloat segmentLength = 0;

  switch (element->type) {
    case kCGPathElementMoveToPoint:
      points[0] = element->points[0];
      pathInfo->currentPoint.x = points[0].x;
      pathInfo->currentPoint.y = points[0].y;
      break;

    case kCGPathElementAddLineToPoint:
      points[0] = element->points[0];
      segmentLength = hypot(points[0].x - lastPoint.x, points[0].y - lastPoint.y);
      pathInfo->totalLength += segmentLength;
      [(__bridge PathLengthCache *)pathInfo->cache addSegmentLength:segmentLength];
      pathInfo->currentPoint.x = points[0].x;
      pathInfo->currentPoint.y = points[0].y;
      break;

    case kCGPathElementAddQuadCurveToPoint:
      points[0] = element->points[0];
      points[1] = element->points[1];
      segmentLength =
          calculateQuadraticBezierLength(lastPoint, points[0], points[1], pathInfo->segments);
      [(__bridge PathLengthCache *)pathInfo->cache addSegmentLength:segmentLength];
      pathInfo->totalLength += segmentLength;
      pathInfo->currentPoint = points[1];
      break;

    case kCGPathElementAddCurveToPoint:
      points[0] = element->points[0];
      points[1] = element->points[1];
      points[2] = element->points[2];
      segmentLength = calculateCubicBezierLength(lastPoint, points[0], points[1], points[2],
                                                 pathInfo->segments);
      [(__bridge PathLengthCache *)pathInfo->cache addSegmentLength:segmentLength];
      pathInfo->totalLength += segmentLength;
      pathInfo->currentPoint = points[2];
      break;

    case kCGPathElementCloseSubpath:
      segmentLength = hypot(pathInfo->currentPoint.x - pathInfo->startPoint.x,
                            pathInfo->currentPoint.y - pathInfo->startPoint.y);
      [(__bridge PathLengthCache *)pathInfo->cache addSegmentLength:segmentLength];
      pathInfo->totalLength += segmentLength;
      pathInfo->currentPoint = pathInfo->startPoint;
      break;
  }
}

// Finding the end Point
static void findTargetPoint(void *info, const CGPathElement *element) {
  PathInfo *pathInfo = (PathInfo *)info;
  if (pathInfo->found) return;
  BOOL need_calculate_tangent = pathInfo->isAutoRotate;
  CGPoint points[3];
  CGPoint lastPoint = pathInfo->currentPoint;
  PathLengthCache *cache = (__bridge PathLengthCache *)pathInfo->cache;

  switch (element->type) {
    case kCGPathElementMoveToPoint:
      points[0] = element->points[0];
      pathInfo->currentPoint = points[0];
      break;

    case kCGPathElementAddLineToPoint:
    case kCGPathElementAddQuadCurveToPoint:
    case kCGPathElementAddCurveToPoint:
    case kCGPathElementCloseSubpath: {
      // Uses the cached segment length.
      CGFloat segmentLength = [cache.segmentLengths[pathInfo->currentSegmentIndex] doubleValue];

      if (pathInfo->totalLength + segmentLength >= pathInfo->targetLength) {
        CGFloat remainingLength = pathInfo->targetLength - pathInfo->totalLength;
        CGFloat ratio = remainingLength / segmentLength;

        // Points are calculated according to different types.
        switch (element->type) {
          case kCGPathElementAddLineToPoint:
            points[0] = element->points[0];
            if (need_calculate_tangent) {
              pathInfo->resultPointTangent = getLineTangent(lastPoint, points[0]);
            }
            pathInfo->resultPoint = CGPointMake(lastPoint.x + (points[0].x - lastPoint.x) * ratio,
                                                lastPoint.y + (points[0].y - lastPoint.y) * ratio);
            break;

          case kCGPathElementAddQuadCurveToPoint:
            if (need_calculate_tangent) {
              pathInfo->resultPointTangent = getQuadraticBezierTangent(
                  lastPoint, element->points[0], element->points[1], ratio);
            }
            pathInfo->resultPoint =
                getQuadraticBezierPoint(lastPoint, element->points[0], element->points[1], ratio);
            break;

          case kCGPathElementAddCurveToPoint:
            if (need_calculate_tangent) {
              pathInfo->resultPointTangent = getCubicBezierTangent(
                  lastPoint, element->points[0], element->points[1], element->points[2], ratio);
            }
            pathInfo->resultPoint = getCubicBezierPoint(
                lastPoint, element->points[0], element->points[1], element->points[2], ratio);
            break;

          case kCGPathElementCloseSubpath:
            if (need_calculate_tangent) {
              pathInfo->resultPointTangent = getLineTangent(lastPoint, pathInfo->startPoint);
            }
            pathInfo->resultPoint =
                CGPointMake(lastPoint.x + (pathInfo->startPoint.x - lastPoint.x) * ratio,
                            lastPoint.y + (pathInfo->startPoint.y - lastPoint.y) * ratio);
            break;
          default:
            break;
        }
        pathInfo->found = YES;
      }

      pathInfo->totalLength += segmentLength;
      pathInfo->currentSegmentIndex++;

      // Update the current point.
      switch (element->type) {
        case kCGPathElementAddLineToPoint:
          pathInfo->currentPoint = element->points[0];
          break;
        case kCGPathElementAddQuadCurveToPoint:
          pathInfo->currentPoint = element->points[1];
          break;
        case kCGPathElementAddCurveToPoint:
          pathInfo->currentPoint = element->points[2];
          break;
        case kCGPathElementCloseSubpath:
          pathInfo->currentPoint = pathInfo->startPoint;
          break;
        default:
          break;
      }
      break;
    }
  }
}

#pragma mark - Public Methods

+ (void)initialize {
  if (self == [LynxOffsetCalculator class]) {
    lruPathCache = [[LynxLRUMap alloc] initWithCapacity:kMaxCacheSize];
  }
}

+ (CGPoint)pointAtProgress:(CGFloat)progress
                    onPath:(CGPathRef)path
               withTangent:(nullable CGFloat *)tangent {
  if (!path) return CGPointZero;

  progress = MAX(0, MIN(1, progress));
  PathLengthCache *cache = [lruPathCache get:(__bridge id)path];
  if (!cache) {
    cache = [[PathLengthCache alloc] init];
    PathInfo info = {0,
                     0,
                     CGPointZero,
                     CGPointZero,
                     CGPointZero,
                     CGPointZero,
                     NO,
                     NO,
                     100,
                     (__bridge void *)cache,
                     0};
    if (tangent != nullptr) {
      info.isAutoRotate = YES;
    }
    CGPathApply(path, &info, calculatePathLengthFunction);
    cache.totalLength = info.totalLength;

    // LRU cache
    [lruPathCache set:(__bridge id)path value:cache];
  }
  PathInfo info = {
      0, 0, CGPointZero, CGPointZero, CGPointZero, CGPointZero, NO, NO, 100, (__bridge void *)cache,
      0};
  if (tangent != nullptr) {
    info.isAutoRotate = YES;
  }
  info.totalLength = cache.totalLength;
  info.targetLength = info.totalLength * progress;
  info.totalLength = 0;

  CGPathApply(path, &info, findTargetPoint);
  if (tangent != nullptr) {
    *tangent = angleWithPositiveYAxis(info.resultPointTangent);
  }
  return info.resultPoint;
}

@end
