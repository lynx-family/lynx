// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/ios/path_darwin.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>

#include "gfx/public/path_parser.h"

namespace lynx::tasm {
namespace {

void AddRoundedRect(CGMutablePathRef path, const gfx::PathHeader& header) {
  const CGFloat x = header.x;
  const CGFloat y = header.y;
  const CGFloat right = x + header.width;
  const CGFloat bottom = y + header.height;
  const auto& r = header.radii;
  constexpr CGFloat kControl = 0.5522847498307936;
  CGPathMoveToPoint(path, nullptr, x + r[0], y);
  CGPathAddLineToPoint(path, nullptr, right - r[2], y);
  CGPathAddCurveToPoint(path, nullptr, right - r[2] + kControl * r[2], y, right,
                        y + r[3] - kControl * r[3], right, y + r[3]);
  CGPathAddLineToPoint(path, nullptr, right, bottom - r[5]);
  CGPathAddCurveToPoint(path, nullptr, right, bottom - r[5] + kControl * r[5],
                        right - r[4] + kControl * r[4], bottom, right - r[4], bottom);
  CGPathAddLineToPoint(path, nullptr, x + r[6], bottom);
  CGPathAddCurveToPoint(path, nullptr, x + r[6] - kControl * r[6], bottom, x,
                        bottom - r[7] + kControl * r[7], x, bottom - r[7]);
  CGPathAddLineToPoint(path, nullptr, x, y + r[1]);
  CGPathAddCurveToPoint(path, nullptr, x, y + r[1] - kControl * r[1], x + r[0] - kControl * r[0], y,
                        x + r[0], y);
  CGPathCloseSubpath(path);
}

CGPathRef CreateGeometry(const gfx::PathView& view) {
  CGMutablePathRef path = CGPathCreateMutable();
  if (path == nullptr) return nullptr;
  switch (view.header.kind) {
    case gfx::PathKind::kRect:
      if (view.header.width > 0 && view.header.height > 0) {
        CGPathAddRect(
            path, nullptr,
            CGRectMake(view.header.x, view.header.y, view.header.width, view.header.height));
      }
      break;
    case gfx::PathKind::kRoundedRect:
      if (view.header.width > 0 && view.header.height > 0) {
        AddRoundedRect(path, view.header);
      }
      break;
    case gfx::PathKind::kCommands: {
      const gfx::PathPoint* point = view.points;
      for (uint32_t i = 0; i < view.header.command_count; ++i) {
        switch (view.commands[i]) {
          case gfx::PathVerb::kBegin:
            CGPathMoveToPoint(path, nullptr, point[0].x, point[0].y);
            ++point;
            break;
          case gfx::PathVerb::kLine:
            CGPathAddLineToPoint(path, nullptr, point[0].x, point[0].y);
            ++point;
            break;
          case gfx::PathVerb::kQuadratic:
            CGPathAddQuadCurveToPoint(path, nullptr, point[0].x, point[0].y, point[1].x,
                                      point[1].y);
            point += 2;
            break;
          case gfx::PathVerb::kCubic:
            CGPathAddCurveToPoint(path, nullptr, point[0].x, point[0].y, point[1].x, point[1].y,
                                  point[2].x, point[2].y);
            point += 3;
            break;
          case gfx::PathVerb::kClose:
            CGPathCloseSubpath(path);
            break;
          case gfx::PathVerb::kEnd:
            break;
        }
      }
      break;
    }
  }
  CGPathRef result = CGPathCreateCopy(path);
  CGPathRelease(path);
  return result;
}

void CheckFinitePoints(void* context, const CGPathElement* element) {
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
  auto& finite = *static_cast<bool*>(context);
  for (size_t i = 0; i < count; ++i) {
    finite &= std::isfinite(element->points[i].x) && std::isfinite(element->points[i].y);
  }
}

struct SvgConsumer {
  CGMutablePathRef path;
  bool valid{true};
};

void AddSvgArc(void* context, float start_x, float start_y, float radius_x, float radius_y,
               float angle, bool large, bool sweep, float end_x, float end_y) {
  auto& consumer = *static_cast<SvgConsumer*>(context);
  if (start_x == end_x && start_y == end_y) return;
  double rx = std::abs(static_cast<double>(radius_x));
  double ry = std::abs(static_cast<double>(radius_y));
  if (rx == 0 || ry == 0) {
    CGPathAddLineToPoint(consumer.path, nullptr, end_x, end_y);
    return;
  }
  constexpr double kPi = 3.14159265358979323846;
  const double theta = std::fmod(static_cast<double>(angle), 360.) * kPi / 180.;
  const double cosine = std::cos(theta);
  const double sine = std::sin(theta);
  const double dx = (static_cast<double>(start_x) - end_x) / 2.;
  const double dy = (static_cast<double>(start_y) - end_y) / 2.;
  const double x = cosine * dx + sine * dy;
  const double y = -sine * dx + cosine * dy;
  double nx = x / rx;
  double ny = y / ry;
  double distance = nx * nx + ny * ny;
  if (distance > 1.) {
    const double factor = std::sqrt(distance);
    rx *= factor;
    ry *= factor;
    nx = x / rx;
    ny = y / ry;
    distance = nx * nx + ny * ny;
  }
  const double coefficient =
      (large == sweep ? -1. : 1.) * std::sqrt(std::max(0., (1. - distance) / distance));
  const double cx = coefficient * rx * ny;
  const double cy = -coefficient * ry * nx;
  const double center_x = cosine * cx - sine * cy + (static_cast<double>(start_x) + end_x) / 2.;
  const double center_y = sine * cx + cosine * cy + (static_cast<double>(start_y) + end_y) / 2.;
  double ux = (x - cx) / rx;
  double uy = (y - cy) / ry;
  const double vx = (-x - cx) / rx;
  const double vy = (-y - cy) / ry;
  const double minor_angle = 2. * std::asin(std::min(1., std::sqrt(distance)));
  const double delta = (large ? 2. * kPi - minor_angle : minor_angle) * (sweep ? 1. : -1.);
  CGAffineTransform ellipse =
      CGAffineTransformMake(rx * cosine, rx * sine, -ry * sine, ry * cosine, center_x, center_y);
  if (!std::isfinite(ellipse.a) || !std::isfinite(ellipse.b) || !std::isfinite(ellipse.c) ||
      !std::isfinite(ellipse.d) || !std::isfinite(center_x) || !std::isfinite(center_y) ||
      !std::isfinite(delta)) {
    consumer.valid = false;
    return;
  }
  const int segments = std::max(1, static_cast<int>(std::ceil(std::abs(delta) / (kPi / 2.))));
  const double step = delta / segments;
  const double alpha = 4. / 3. * std::tan(step / 4.);
  const double step_cosine = std::cos(step);
  const double step_sine = std::sin(step);
  double px = start_x;
  double py = start_y;
  for (int i = 0; i < segments; ++i) {
    const bool last = i == segments - 1;
    const double next_x = last ? vx : ux * step_cosine - uy * step_sine;
    const double next_y = last ? vy : ux * step_sine + uy * step_cosine;
    const double qx = last ? end_x : center_x + ellipse.a * next_x + ellipse.c * next_y;
    const double qy = last ? end_y : center_y + ellipse.b * next_x + ellipse.d * next_y;
    // Explicit endpoints and vector tangents avoid cancellation for almost-flat
    // arcs whose radii are much larger than their endpoint separation.
    CGPathAddCurveToPoint(consumer.path, nullptr, px + alpha * (-ellipse.a * uy + ellipse.c * ux),
                          py + alpha * (-ellipse.b * uy + ellipse.d * ux),
                          qx - alpha * (-ellipse.a * next_y + ellipse.c * next_x),
                          qy - alpha * (-ellipse.b * next_y + ellipse.d * next_x), qx, qy);
    px = qx;
    py = qy;
    ux = next_x;
    uy = next_y;
  }
}

CGPathRef CreateSvg(const gfx::SvgPathView& view) {
  SvgConsumer context{CGPathCreateMutable()};
  if (context.path == nullptr) return nullptr;
  LBSPathConsumer consumer{};
  consumer.type = kLBSPathConsumerTypeCoreGraphics;
  consumer.ctx = &context;
  consumer.MoveToPoint = [](void* ctx, float x, float y) {
    CGPathMoveToPoint(static_cast<SvgConsumer*>(ctx)->path, nullptr, x, y);
  };
  consumer.LineToPoint = [](void* ctx, float x, float y) {
    CGPathAddLineToPoint(static_cast<SvgConsumer*>(ctx)->path, nullptr, x, y);
  };
  consumer.QuadToPoint = [](void* ctx, float cx, float cy, float x, float y) {
    CGPathAddQuadCurveToPoint(static_cast<SvgConsumer*>(ctx)->path, nullptr, cx, cy, x, y);
  };
  consumer.CubicToPoint = [](void* ctx, float cx1, float cy1, float cx2, float cy2, float x,
                             float y) {
    CGPathAddCurveToPoint(static_cast<SvgConsumer*>(ctx)->path, nullptr, cx1, cy1, cx2, cy2, x, y);
  };
  consumer.EllipticToPoint = AddSvgArc;
  consumer.ClosePath = [](void* ctx) { CGPathCloseSubpath(static_cast<SvgConsumer*>(ctx)->path); };
  LBSParsePathWithConsumer(view.data.c_str(), &consumer);
  if (consumer.error != 0 || !context.valid) {
    CGPathRelease(context.path);
    return nullptr;
  }
  CGAffineTransform scale =
      CGAffineTransformMakeScale(view.source_to_layout_scale, view.source_to_layout_scale);
  CGPathRef result = CGPathCreateCopyByTransformingPath(context.path, &scale);
  CGPathRelease(context.path);
  if (result != nullptr) {
    bool finite = true;
    CGPathApply(result, &finite, CheckFinitePoints);
    if (!finite) {
      CGPathRelease(result);
      result = nullptr;
    }
  }
  return result;
}

}  // namespace

PathDarwin::PathDarwin(gfx::PathSource source) : gfx::PathImpl(std::move(source)) {}
PathDarwin::~PathDarwin() {
  if (path_ != nullptr) CGPathRelease(path_);
}

CGPathRef PathDarwin::GetCGPath() const {
  if (!realized_) {
    const auto source = GetSourceView();
    path_ = std::holds_alternative<gfx::PathView>(source)
                ? CreateGeometry(std::get<gfx::PathView>(source))
                : CreateSvg(std::get<gfx::SvgPathView>(source));
    realized_ = true;
  }
  return path_;
}

std::unique_ptr<gfx::PathImpl> PathDarwinFactory::Create(gfx::PathSource source) const {
  return std::make_unique<PathDarwin>(std::move(source));
}

}  // namespace lynx::tasm
