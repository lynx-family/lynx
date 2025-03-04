// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <optional>
#import "LynxBackgroundDrawable.h"
#import "LynxCSSType.h"
#import "LynxUIUnitUtils.h"
#include "base/include/log/logging.h"

@implementation LynxBackgroundLinearGradientDrawable

// In CAGradientLayer, the gradient will first standardize to square then use the proportional
// start and end point to render shadow. Thus the slop of the shadow line will be malformed.
// We use the following steps to re-calculate the start and end point in the standardized square.
// Image analysis is in document linked to this commit.
static struct LineSegment fixPoints(CGPoint start, CGPoint end, CGSize bounds) {
  if (ABS(start.x - end.x) < FLT_EPSILON || ABS(start.y - end.y) < FLT_EPSILON) {
    LineSegment res = LineSegment(start, end).divided(CGPointMake(bounds.width, bounds.height));
    return res;
  }
  // clang-format off
  // originalSegment - Line segment from absolute position start and end.
  // bisectorSegment - perpendicular bisector of segment originalSegment.
  // bisectorSegmentNormalized - the bisectorSegment after the rectangle normalized to a square.
  // finalSegmentNormalized - perpendicular bisector of bisectorSegmentNormalized in normalized square.
  // finalSegment - scale finalSegmentNormalized back to original size.
  // finalLine - line on which final segments locates.
  // clang-format on

  // color changes along this segment.
  LineSegment originalSegment = LineSegment(start, end);

  // 1. calculate perpendicular bisector. All points on this segment has the same color, so as
  // segments parallel to it.
  LineSegment bisectorSegment = originalSegment.perpendicularBisector();

  // 2. Normalize to standard square.
  CGPoint multiplier = calculateMultipliers(bounds);
  LineSegment bisectorSegmentNormalized = bisectorSegment.multiplied(multiplier);

  // 3. create scaled perpendicular bisector.
  LineSegment finalSegmentNormalized = bisectorSegmentNormalized.perpendicularBisector();

  // 4. Scale back to rectangle
  LineSegment finalSegment = finalSegmentNormalized.divided(multiplier);

  // 5. Extend line
  Line finalLine = finalSegment.line();

  // 6. Extend two lines across the start point and the end point, which are parallel to
  // bisectorSegment.
  Line startParallelLine = Line(bisectorSegment.slope(), originalSegment.p1);
  Line endParallelLine = Line(bisectorSegment.slope(), originalSegment.p2);

  // 7. Find the intersection of these lines
  std::optional<CGPoint> finalStart = finalLine.intersection(&startParallelLine);
  std::optional<CGPoint> finalEnd = finalLine.intersection(&endParallelLine);

  // 8. Convert back to relative coordinates
  if (finalStart != std::nullopt && finalEnd != std::nullopt) {
    LineSegment result = LineSegment(*finalStart, *finalEnd);
    return result.divided(CGPointMake(bounds.width, bounds.height));
  }
  return originalSegment;
}

// Line helper.
// @note to ensure the line is meaningful, the line should not perpendicular to the x-axis.
struct Line {
  // y = kx + b;
  CGFloat k;
  CGFloat b;
  Line(CGFloat k, CGFloat b) : k(k), b(b){};
  Line(CGFloat k, CGPoint p) : k(k), b(p.y - k * p.x){};

  Line(CGPoint p1, CGPoint p2) {
    this->k = (p2.y - p1.y) / (p2.x - p1.x);
    this->b = p1.y - this->k * p1.x;
  }

  CGFloat y(CGFloat x) { return k * x + b; }

  CGPoint point(CGFloat x) { return (CGPoint){x, this->y(x)}; }

  std::optional<CGPoint> intersection(struct Line *line) {
    if (this->k == line->k) {
      return std::nullopt;
    }

    CGFloat x = (line->b - this->b) / (this->k - line->k);
    CGFloat y = this->y(x);
    return (CGPoint){x, y};
  }
};

static CGPoint calculateMultipliers(CGSize bounds) {
  if (bounds.height <= bounds.width) {
    return {1, bounds.width / bounds.height};
  } else {
    return {bounds.height / bounds.width, 1};
  }
}

struct LineSegment {
  CGPoint p1;
  CGPoint p2;
  LineSegment(CGPoint p1, CGPoint p2) : p1(p1), p2(p2){};
  LineSegment(CGPoint p1, CGFloat k, CGFloat distance) : p1(p1) {
    struct Line line = Line(k, p1);
    CGPoint measuringPoint = line.point(p1.x + 1);
    CGFloat measuringDeltaH = LineSegment(p1, measuringPoint).distance();
    CGFloat dx = distance / measuringDeltaH;
    this->p2 = line.point(p1.x + dx);
  }

  CGFloat length() {
    CGFloat dx = p1.x - p2.x;
    CGFloat dy = p1.y - p2.y;
    return sqrt(dx * dx + dy * dy);
  }

  CGFloat slope() { return (p2.y - p1.y) / (p2.x - p1.x); }

  CGPoint midpoint() { return (CGPoint){(p1.x + p2.x) / 2, (p1.y + p2.y) / 2}; }

  CGFloat distance() { return p1.x <= p2.x ? length() : -length(); }

  struct LineSegment perpendicularBisector() {
    CGPoint midPoint = midpoint();
    CGFloat k = -1 / slope();
    CGPoint p1 = LineSegment(midPoint, k, -distance() / 2).p2;
    CGPoint p2 = LineSegment(midPoint, k, distance() / 2).p2;
    return LineSegment(p1, p2);
  }

  LineSegment multiplied(CGPoint multiplier) {
    return LineSegment(CGPointMake(p1.x * multiplier.x, p1.y * multiplier.y),
                       CGPointMake(p2.x * multiplier.x, p2.y * multiplier.y));
  }

  LineSegment divided(CGPoint divider) {
    return multiplied(CGPointMake(1 / divider.x, 1 / divider.y));
  }

  Line line() { return Line(p1, p2); }
};

- (LynxBackgroundImageType)type {
  return LynxBackgroundImageLinearGradient;
}

- (void)onPrepareGradientWithSize:(CGSize)gradientSize {
  if (ABS(gradientSize.width) < FLT_EPSILON || ABS(gradientSize.height) < FLT_EPSILON) {
    return;
  }

  CGPoint start, end;

  [((LynxLinearGradient *)self.gradient) computeStartPoint:&start
                                               andEndPoint:&end
                                                  withSize:&gradientSize];

  LineSegment seg = fixPoints(start, end, gradientSize);
  self.gradientLayer.startPoint = seg.p1;
  self.gradientLayer.endPoint = seg.p2;
  self.gradientLayer.type = kCAGradientLayerAxial;
}

- (instancetype)initWithArray:(NSArray *)array {
  self = [super init];
  if (self) {
    if (array == nil) {
      LOGE("linear gradient native parse error, array is null");
    } else if ([array count] < 3) {
      LOGE("linear gradient native parse error, array must have 3 element.");
    } else {
      self.gradient = [[LynxLinearGradient alloc] initWithArray:array];
    }
  }
  return self;
}
@end
