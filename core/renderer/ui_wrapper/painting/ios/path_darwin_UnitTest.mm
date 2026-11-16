// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <QuartzCore/QuartzCore.h>
#import <XCTest/XCTest.h>

#include <cmath>
#include <vector>

#include "core/renderer/ui_wrapper/painting/ios/path_darwin.h"
#include "gfx/public/display_list.h"
#include "gfx/public/path.h"

namespace {

struct NativeElement {
  CGPathElementType type;
  std::vector<CGPoint> points;
};

std::vector<NativeElement> Elements(CGPathRef path) {
  std::vector<NativeElement> elements;
  CGPathApply(path, &elements, [](void* context, const CGPathElement* element) {
    size_t count = 0;
    switch (element->type) {
      case kCGPathElementMoveToPoint:
      case kCGPathElementAddLineToPoint:
        count = 1;
        break;
      case kCGPathElementAddQuadCurveToPoint:
        count = 2;
        break;
      case kCGPathElementAddCurveToPoint:
        count = 3;
        break;
      case kCGPathElementCloseSubpath:
        break;
    }
    NativeElement copy{element->type, {}};
    for (size_t i = 0; i < count; ++i) copy.points.push_back(element->points[i]);
    static_cast<std::vector<NativeElement>*>(context)->push_back(std::move(copy));
  });
  return elements;
}

CGPathRef NativePath(const lynx::gfx::Path& path) {
  return static_cast<const lynx::tasm::PathDarwin&>(path.GetImpl()).GetCGPath();
}

}  // namespace

@interface PathDarwinUnitTest : XCTestCase
@end

@implementation PathDarwinUnitTest

- (void)testRectAndRepeatedNativeAccess {
  lynx::tasm::PathDarwinFactory factory;
  auto path = lynx::gfx::Path::MakeRect(100, 80, factory);
  const auto key = path->GetKey();
  CGPathRef native = NativePath(*path);
  XCTAssertNotEqual(native, nullptr);
  XCTAssertTrue(CGRectEqualToRect(CGPathGetPathBoundingBox(native), CGRectMake(0, 0, 100, 80)));
  XCTAssertTrue(CGPathContainsPoint(native, nullptr, CGPointMake(50, 40), false));
  XCTAssertFalse(CGPathContainsPoint(native, nullptr, CGPointMake(101, 40), false));
  CGPathRetain(native);
  for (size_t i = 0; i < 10; ++i) {
    XCTAssertEqual(NativePath(*path), native);
    XCTAssertEqual(path->GetKey(), key);
  }
  lynx::tasm::DisplayList first;
  lynx::tasm::DisplayList replacement;
  first.RetainPath(path);
  replacement.RetainPath(path);
  first.Clear();
  XCTAssertEqual(NativePath(*replacement.GetPath(0)), native);
  CGPathRelease(native);
}

- (void)testRoundedRectPreservesAllEightResolvedRadii {
  lynx::tasm::PathDarwinFactory factory;
  const float radii[8] = {10, 15, 20, 25, 30, 35, 40, 45};
  auto path = lynx::gfx::Path::MakeRoundedRect(100, 100, radii, factory);
  CGPathRef native = NativePath(*path);
  auto elements = Elements(native);
  XCTAssertEqual(elements.size(), 10u);
  const CGPoint endpoints[] = {{100, 25}, {70, 100}, {0, 55}, {10, 0}};
  for (size_t i = 0; i < 4; ++i) {
    const auto& curve = elements[2 + i * 2];
    XCTAssertEqual(curve.type, kCGPathElementAddCurveToPoint);
    XCTAssertTrue(CGPointEqualToPoint(curve.points[2], endpoints[i]));
  }
  XCTAssertEqualWithAccuracy(elements[2].points[0].x, 80 + 20 * 0.5522847498307936, 0.00001);
  XCTAssertEqualWithAccuracy(elements[8].points[1].x, 10 - 10 * 0.5522847498307936, 0.00001);
  XCTAssertTrue(CGPointEqualToPoint(elements[1].points[0], CGPointMake(80, 0)));
  XCTAssertTrue(CGPointEqualToPoint(elements[3].points[0], CGPointMake(100, 65)));
  XCTAssertTrue(CGPointEqualToPoint(elements[5].points[0], CGPointMake(40, 100)));
  XCTAssertTrue(CGPointEqualToPoint(elements[7].points[0], CGPointMake(0, 15)));
  XCTAssertFalse(CGPathContainsPoint(native, nullptr, CGPointMake(1, 1), false));
  XCTAssertTrue(CGPathContainsPoint(native, nullptr, CGPointMake(50, 50), false));

  const float oversized[8] = {80, 80, 80, 80, 80, 80, 80, 80};
  auto normalized = lynx::gfx::Path::MakeRoundedRect(100, 40, oversized, factory);
  auto normalizedElements = Elements(NativePath(*normalized));
  XCTAssertTrue(CGPointEqualToPoint(normalizedElements[0].points[0], CGPointMake(20, 0)));
  XCTAssertTrue(CGPointEqualToPoint(normalizedElements[2].points[2], CGPointMake(100, 20)));
}

- (void)testCommandsPreserveCurvesAndOpenSubpaths {
  lynx::tasm::PathDarwinFactory factory;
  lynx::gfx::PathBuilder builder;
  builder.MoveTo(1, 2);
  builder.LineTo(3, 4);
  builder.QuadTo(5, 6, 7, 8);
  builder.CubicTo(9, 10, 11, 12, 13, 14);
  builder.MoveTo(20, 20);
  builder.LineTo(30, 30);
  builder.Close();
  auto path = builder.Build(factory);
  auto elements = Elements(NativePath(*path));
  XCTAssertEqual(elements.size(), 7u);
  XCTAssertEqual(elements[2].type, kCGPathElementAddQuadCurveToPoint);
  XCTAssertTrue(CGPointEqualToPoint(elements[2].points[0], CGPointMake(5, 6)));
  XCTAssertTrue(CGPointEqualToPoint(elements[2].points[1], CGPointMake(7, 8)));
  XCTAssertEqual(elements[3].type, kCGPathElementAddCurveToPoint);
  XCTAssertTrue(CGPointEqualToPoint(elements[3].points[2], CGPointMake(13, 14)));
  XCTAssertEqual(elements[4].type, kCGPathElementMoveToPoint);
  XCTAssertEqual(elements.back().type, kCGPathElementCloseSubpath);
}

- (void)testSvgScaleArcsAndFillRuleShareImmutableGeometry {
  lynx::tasm::PathDarwinFactory factory;
  auto path = lynx::gfx::Path::MakeSvgPathData("M0 0H40V40H0Z M10 10H30V30H10Z", 2, factory);
  CGPathRef native = NativePath(*path);
  XCTAssertTrue(CGRectEqualToRect(CGPathGetPathBoundingBox(native), CGRectMake(0, 0, 80, 80)));
  XCTAssertTrue(CGPathContainsPoint(native, nullptr, CGPointMake(40, 40), false));
  XCTAssertFalse(CGPathContainsPoint(native, nullptr, CGPointMake(40, 40), true));
  XCTAssertTrue(CGPathContainsPoint(native, nullptr, CGPointMake(10, 10), true));
  XCTAssertEqual(NativePath(*path), native);

  auto arc = lynx::gfx::Path::MakeSvgPathData("M0 10A10 10 0 0 1 20 10L0 10Z", 2, factory);
  CGPathRef arcNative = NativePath(*arc);
  XCTAssertTrue(CGPathContainsPoint(arcNative, nullptr, CGPointMake(20, 10), false));
  XCTAssertFalse(CGPathContainsPoint(arcNative, nullptr, CGPointMake(20, 30), false));
  auto arcElements = Elements(arcNative);
  XCTAssertEqual(arcElements[1].type, kCGPathElementAddCurveToPoint);
}

- (void)testMalformedSvgAndEmptyGeometry {
  lynx::tasm::PathDarwinFactory factory;
  for (const char* data : {"M0 0L10 0L", "M0 0A10 10 0 0 1 10", "M1e100 0"}) {
    auto path = lynx::gfx::Path::MakeSvgPathData(data, 1, factory);
    XCTAssertEqual(NativePath(*path), nullptr);
    XCTAssertEqual(NativePath(*path), nullptr);
  }
  auto empty = lynx::gfx::Path::MakeSvgPathData("", 1, factory);
  XCTAssertNotEqual(NativePath(*empty), nullptr);
  XCTAssertTrue(CGPathIsEmpty(NativePath(*empty)));
  auto zero = lynx::gfx::Path::MakeRect(0, 20, factory);
  XCTAssertFalse(CGPathContainsPoint(NativePath(*zero), nullptr, CGPointMake(0, 10), false));
  lynx::gfx::PathBuilder builder;
  auto noCommands = builder.Build(factory);
  XCTAssertTrue(CGPathIsEmpty(NativePath(*noCommands)));
}

- (void)testLargeFiniteSvgArcDoesNotPublishOnlyItsPrefix {
  lynx::tasm::PathDarwinFactory factory;
  auto path = lynx::gfx::Path::MakeSvgPathData("M0 0H100V100H0Z A1 1 0 0 1 1e30 0", 1, factory);
  CGPathRef native = NativePath(*path);
  XCTAssertNotEqual(native, nullptr);
  auto elements = Elements(native);
  bool hasCurve = false;
  for (const auto& element : elements) {
    hasCurve |= element.type == kCGPathElementAddCurveToPoint;
    for (CGPoint point : element.points) {
      XCTAssertTrue(std::isfinite(point.x));
      XCTAssertTrue(std::isfinite(point.y));
    }
  }
  XCTAssertTrue(hasCurve);
  XCTAssertGreaterThan(CGRectGetMaxX(CGPathGetPathBoundingBox(native)), 9e29);
}

- (void)testAlmostFlatSvgArcPreservesItsExplicitEndpoint {
  lynx::tasm::PathDarwinFactory factory;
  for (const char* data :
       {"M0 0A1e30 1e30 0 0 1 10 0", "M0 0A1e30 1e30 45 0 1 10 0", "M0 0A1e30 1e30 45 0 0 10 0",
        "M0 0A1e30 2e30 45 0 1 10 0", "M0 0A1e30 2e30 45 0 0 10 0"}) {
    auto path = lynx::gfx::Path::MakeSvgPathData(data, 1, factory);
    CGPathRef native = NativePath(*path);
    XCTAssertNotEqual(native, nullptr);
    XCTAssertTrue(CGPointEqualToPoint(CGPathGetCurrentPoint(native), CGPointMake(10, 0)));
    CGRect bounds = CGPathGetPathBoundingBox(native);
    XCTAssertEqualWithAccuracy(CGRectGetMinX(bounds), 0, 0.00001);
    XCTAssertEqualWithAccuracy(CGRectGetMaxX(bounds), 10, 0.00001);
    XCTAssertLessThan(bounds.size.height, 0.00001);
  }
}

- (void)testSvgArcSweepCorrectionRotationAndDegenerateRadii {
  lynx::tasm::PathDarwinFactory factory;
  auto clockwise = lynx::gfx::Path::MakeSvgPathData("M0 0A1 1 0 0 1 20 0Z", 1, factory);
  auto counterclockwise = lynx::gfx::Path::MakeSvgPathData("M0 0A10 10 0 0 0 20 0Z", 1, factory);
  XCTAssertTrue(CGPathContainsPoint(NativePath(*clockwise), nullptr, CGPointMake(10, -5), false));
  XCTAssertFalse(CGPathContainsPoint(NativePath(*clockwise), nullptr, CGPointMake(10, 5), false));
  XCTAssertTrue(
      CGPathContainsPoint(NativePath(*counterclockwise), nullptr, CGPointMake(10, 5), false));
  auto rotated = lynx::gfx::Path::MakeSvgPathData("M0 0A20 10 45 1 1 30 20", 1, factory);
  XCTAssertTrue(
      CGPointEqualToPoint(CGPathGetCurrentPoint(NativePath(*rotated)), CGPointMake(30, 20)));
  XCTAssertGreaterThan(Elements(NativePath(*rotated)).size(), 3u);
  auto zeroRadius = lynx::gfx::Path::MakeSvgPathData("M0 0A0 10 0 0 1 20 10", 1, factory);
  auto elements = Elements(NativePath(*zeroRadius));
  XCTAssertEqual(elements.size(), 2u);
  XCTAssertEqual(elements[1].type, kCGPathElementAddLineToPoint);
  XCTAssertTrue(CGPointEqualToPoint(elements[1].points[0], CGPointMake(20, 10)));
  auto coincident = lynx::gfx::Path::MakeSvgPathData("M3 4A10 10 0 1 1 3 4", 1, factory);
  XCTAssertEqual(Elements(NativePath(*coincident)).size(), 1u);
}

- (void)testLargeRotatedSvgArcPreservesAlmostCompleteContour {
  lynx::tasm::PathDarwinFactory factory;
  for (const char* data : {"M0 0A1e30 1e30 45 1 1 10 0", "M0 0A1e30 1e30 45 1 0 10 0",
                           "M0 0A1e30 2e30 45 1 1 10 0", "M0 0A1e30 2e30 45 1 0 10 0"}) {
    auto path = lynx::gfx::Path::MakeSvgPathData(data, 1, factory);
    CGPathRef native = NativePath(*path);
    XCTAssertNotEqual(native, nullptr);
    XCTAssertTrue(CGPointEqualToPoint(CGPathGetCurrentPoint(native), CGPointMake(10, 0)));
    CGRect bounds = CGPathGetPathBoundingBox(native);
    XCTAssertGreaterThan(bounds.size.width, 1e30);
    XCTAssertGreaterThan(bounds.size.height, 1e30);
    size_t curves = 0;
    for (const auto& element : Elements(native)) {
      curves += element.type == kCGPathElementAddCurveToPoint;
      for (CGPoint point : element.points) {
        XCTAssertTrue(std::isfinite(point.x));
        XCTAssertTrue(std::isfinite(point.y));
      }
    }
    XCTAssertEqual(curves, 4u);
  }
}

- (void)testListAndLayerRetainGeometryAcrossReplacementAndSourceRelease {
  lynx::tasm::PathDarwinFactory factory;
  lynx::tasm::DisplayList list;
  auto path = lynx::gfx::Path::MakeRect(30, 40, factory);
  const auto index = list.RetainPath(path);
  const auto oldKey = path->GetKey();
  CGPathRef native = NativePath(*path);
  CAShapeLayer* layer = [CAShapeLayer layer];
  layer.path = native;
  path = lynx::gfx::Path::MakeRect(60, 70, factory);
  XCTAssertNotEqual(path->GetKey(), oldKey);
  XCTAssertEqual(NativePath(*list.GetPath(index)), native);
  XCTAssertTrue(CGRectEqualToRect(CGPathGetPathBoundingBox(native), CGRectMake(0, 0, 30, 40)));
  XCTAssertTrue(
      CGRectEqualToRect(CGPathGetPathBoundingBox(NativePath(*path)), CGRectMake(0, 0, 60, 70)));
  list.Clear();
  XCTAssertTrue(CGRectEqualToRect(CGPathGetPathBoundingBox(layer.path), CGRectMake(0, 0, 30, 40)));
  layer.path = nullptr;
}

@end
