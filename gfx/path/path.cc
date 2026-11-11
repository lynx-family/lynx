// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "gfx/public/path.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <utility>

#include "base/include/compiler_specific.h"
#include "gfx/public/path_impl_factory.h"

namespace lynx::gfx {
namespace {
bool ValidSize(float width, float height) {
  return std::isfinite(width) && std::isfinite(height) && width >= 0 &&
         height >= 0;
}
size_t GeometryBytes(size_t commands, size_t points) {
  return sizeof(PathStorage) + commands * sizeof(PathVerb) +
         points * sizeof(PathPoint);
}
PathVerb* GeometryCommands(void* geometry) {
  return reinterpret_cast<PathVerb*>(static_cast<unsigned char*>(geometry) +
                                     sizeof(PathStorage));
}
PathPoint* GeometryPoints(void* geometry, size_t commands) {
  return reinterpret_cast<PathPoint*>(GeometryCommands(geometry) + commands);
}
bool ValidPoint(float x, float y) {
  return std::isfinite(x) && std::isfinite(y);
}
bool NormalizeRoundedRect(float width, float height, const float (&radii)[8],
                          PathHeader& header) {
  if (!ValidSize(width, height)) return false;
  header.kind = PathKind::kRoundedRect;
  header.width = width;
  header.height = height;
  for (size_t i = 0; i < 8; ++i) {
    if (!std::isfinite(radii[i]) || radii[i] < 0) return false;
    header.radii[i] = radii[i];
  }
  float factor = 1.f;
  const auto limit = [&factor](float extent, float a, float b) {
    const double sum = static_cast<double>(a) + b;
    if (sum > extent)
      factor = std::min(factor, static_cast<float>(extent / sum));
  };
  limit(width, radii[0], radii[2]);
  limit(width, radii[6], radii[4]);
  limit(height, radii[1], radii[7]);
  limit(height, radii[3], radii[5]);
  for (float& radius : header.radii) radius *= factor;
  return true;
}
constexpr double kHalfPi = 1.57079632679489661923;
constexpr double kSampleStep = 0.01;
constexpr size_t kQuarterSamples = static_cast<size_t>(kHalfPi / kSampleStep);
constexpr size_t kSuperPoints = 4 * (kQuarterSamples + 2);
constexpr float kCircleControl = 0.5522847498307936f;
bool ValidExponent(float value) { return std::isfinite(value) && value > 0; }
struct LameSample {
  double cos_x, sin_x, cos_y, sin_y;
};
double LamePower(double value, float exponent) {
  return exponent == 2 ? value : std::pow(value, 2. / exponent);
}
void SampleLame(float ex, float ey, LameSample (&samples)[kQuarterSamples]) {
  double angle = kSampleStep;
  for (auto& sample : samples) {
    double c = std::cos(angle), s = std::sin(angle);
    sample.cos_x = LamePower(c, ex);
    sample.sin_x = LamePower(s, ex);
    sample.cos_y = ey == ex ? sample.cos_x : LamePower(c, ey);
    sample.sin_y = ey == ex ? sample.sin_x : LamePower(s, ey);
    angle += kSampleStep;
  }
}
void LameQuarter(PathBuilder& builder, float rx, float ry, float cx, float cy,
                 const LameSample (&samples)[kQuarterSamples], int quadrant) {
  const float sx = (quadrant == 0 || quadrant == 3) ? 1.f : -1.f;
  const float sy = quadrant < 2 ? 1.f : -1.f;
  const bool odd = (quadrant & 1) != 0;
  const float start_x = odd ? cx : cx + sx * rx;
  const float start_y = odd ? cy + sy * ry : cy;
  if (quadrant == 0)
    builder.MoveTo(start_x, start_y);
  else
    builder.LineTo(start_x, start_y);
  for (const auto& sample : samples) {
    const double x = odd ? sample.sin_x : sample.cos_x;
    const double y = odd ? sample.cos_y : sample.sin_y;
    builder.LineTo(static_cast<float>(cx + sx * rx * x),
                   static_cast<float>(cy + sy * ry * y));
  }
  builder.LineTo(odd ? cx + sx * rx : cx, odd ? cy : cy + sy * ry);
}

// Copyright 2014 The Android Open Source Project.
bool ArcToCubics(PathBuilder& builder, float x0, float y0, float rx, float ry,
                 float rotation, bool large, bool positive, float x1,
                 float y1) {
  if (!ValidPoint(x0, y0) || !ValidPoint(x1, y1) || !ValidPoint(rx, ry) ||
      !std::isfinite(rotation))
    return false;
  if (x0 == x1 && y0 == y1) return true;
  rx = std::abs(rx);
  ry = std::abs(ry);
  if (rx == 0 || ry == 0) {
    builder.LineTo(x1, y1);
    return true;
  }
  const double angle = rotation * (kHalfPi / 90);
  const double cos_angle = std::cos(angle), sin_angle = std::sin(angle);
  double cx, cy, start, sweep;
  for (int correction = 0;; ++correction) {
    const double ax = (x0 * cos_angle + y0 * sin_angle) / rx;
    const double ay = (-x0 * sin_angle + y0 * cos_angle) / ry;
    const double bx = (x1 * cos_angle + y1 * sin_angle) / rx;
    const double by = (-x1 * sin_angle + y1 * cos_angle) / ry;
    const double dx = ax - bx, dy = ay - by;
    const double distance = dx * dx + dy * dy;
    if (distance == 0) return true;
    const double discriminant = 1 / distance - 0.25;
    if (discriminant < 0) {
      if (correction == 1) return false;
      const float adjust = static_cast<float>(std::sqrt(distance) / 1.99999);
      rx *= adjust;
      ry *= adjust;
      if (!ValidPoint(rx, ry) || rx == 0 || ry == 0) return false;
      continue;
    }
    const double factor = std::sqrt(discriminant);
    const double sign = large == positive ? 1 : -1;
    cx = (ax + bx) / 2 - sign * factor * dy;
    cy = (ay + by) / 2 + sign * factor * dx;
    start = std::atan2(ay - cy, ax - cx);
    sweep = std::atan2(by - cy, bx - cx) - start;
    if (positive != (sweep >= 0))
      sweep += sweep > 0 ? -4 * kHalfPi : 4 * kHalfPi;
    const double scaled_x = cx * rx, scaled_y = cy * ry;
    cx = scaled_x * cos_angle - scaled_y * sin_angle;
    cy = scaled_x * sin_angle + scaled_y * cos_angle;
    break;
  }
  // Match AndroidX's at-most-45-degree cubic approximation for SVG arcs.
  const int segments =
      static_cast<int>(std::ceil(std::abs(sweep * 2 / kHalfPi)));
  if (segments == 0) return true;
  if (segments > 8) return false;
  const double step = sweep / segments;
  double previous_x = x0, previous_y = y0;
  double dx =
      -rx * cos_angle * std::sin(start) - ry * sin_angle * std::cos(start);
  double dy =
      -rx * sin_angle * std::sin(start) + ry * cos_angle * std::cos(start);
  for (int i = 0; i < segments; ++i) {
    const double end = start + step;
    const double sin_end = std::sin(end), cos_end = std::cos(end);
    const double x = cx + rx * cos_angle * cos_end - ry * sin_angle * sin_end;
    const double y = cy + rx * sin_angle * cos_end + ry * cos_angle * sin_end;
    const double next_dx = -rx * cos_angle * sin_end - ry * sin_angle * cos_end;
    const double next_dy = -rx * sin_angle * sin_end + ry * cos_angle * cos_end;
    const double tangent = std::tan((end - start) / 2);
    const double alpha =
        std::sin(end - start) * (std::sqrt(4 + 3 * tangent * tangent) - 1) / 3;
    builder.CubicTo(static_cast<float>(previous_x + alpha * dx),
                    static_cast<float>(previous_y + alpha * dy),
                    static_cast<float>(x - alpha * next_dx),
                    static_cast<float>(y - alpha * next_dy),
                    i + 1 == segments ? x1 : static_cast<float>(x),
                    i + 1 == segments ? y1 : static_cast<float>(y));
    start = end;
    previous_x = x;
    previous_y = y;
    dx = next_dx;
    dy = next_dy;
  }
  return true;
}
}  // namespace

PathSourceView PathSource::GetSourceView() const {
  if (source_to_layout_scale_ > 0)
    return SvgPathView{svg_.str(), source_to_layout_scale_};
  return PathView{
      header_, geometry_ ? GeometryCommands(geometry_.get()) : nullptr,
      geometry_ ? GeometryPoints(geometry_.get(), header_.command_count)
                : nullptr};
}
Path::Path(std::unique_ptr<PathImpl> impl) : impl_(std::move(impl)) {}
fml::RefPtr<Path> Path::MakeFromSource(PathSource source,
                                       const PathImplFactory& factory) {
  auto impl = factory.Create(std::move(source));
  if (!impl) std::abort();
  return fml::AdoptRef(new Path(std::move(impl)));
}
Path::~Path() = default;

fml::RefPtr<Path> Path::MakeRect(float width, float height,
                                 const PathImplFactory& factory) {
  if (!ValidSize(width, height)) return nullptr;
  PathHeader header{};
  header.kind = PathKind::kRect;
  header.width = width;
  header.height = height;
  return MakeFromSource(PathSource(header), factory);
}
fml::RefPtr<Path> Path::MakeRoundedRect(float width, float height,
                                        const float (&radii)[8],
                                        const PathImplFactory& factory) {
  PathHeader header{};
  if (!NormalizeRoundedRect(width, height, radii, header)) return nullptr;
  return MakeFromSource(PathSource(header), factory);
}
fml::RefPtr<Path> Path::MakeEllipse(float rx, float ry,
                                    const PathImplFactory& factory) {
  if (!std::isfinite(rx) || !std::isfinite(ry) || rx < 0 || ry < 0)
    return nullptr;
  if (rx == 0 || ry == 0) return Path::MakeRect(0, 0, factory);
  PathBuilder b(6, 13);
  b.MoveTo(rx, 0);
  b.CubicTo(rx, kCircleControl * ry, kCircleControl * rx, ry, 0, ry);
  b.CubicTo(-kCircleControl * rx, ry, -rx, kCircleControl * ry, -rx, 0);
  b.CubicTo(-rx, -kCircleControl * ry, -kCircleControl * rx, -ry, 0, -ry);
  b.CubicTo(kCircleControl * rx, -ry, rx, -kCircleControl * ry, rx, 0);
  b.Close();
  return b.Build(factory);
}
fml::RefPtr<Path> Path::MakeSuperEllipse(float rx, float ry, float ex, float ey,
                                         const PathImplFactory& factory) {
  if (!std::isfinite(rx) || !std::isfinite(ry) || rx < 0 || ry < 0 ||
      !ValidExponent(ex) || !ValidExponent(ey))
    return nullptr;
  if (rx == 0 || ry == 0) return Path::MakeRect(0, 0, factory);
  LameSample samples[kQuarterSamples];
  SampleLame(ex, ey, samples);
  PathBuilder b(kSuperPoints + 1, kSuperPoints);
  for (int q = 0; q < 4; ++q) LameQuarter(b, rx, ry, 0, 0, samples, q);
  b.Close();
  return b.Build(factory);
}
fml::RefPtr<Path> Path::MakeSuperRoundedRect(float width, float height,
                                             const float (&radii)[8], float ex,
                                             float ey,
                                             const PathImplFactory& factory) {
  if (!ValidExponent(ex) || !ValidExponent(ey)) return nullptr;
  PathHeader header{};
  if (!NormalizeRoundedRect(width, height, radii, header)) return nullptr;
  if (width == 0 || height == 0)
    return Path::MakeRoundedRect(width, height, radii, factory);
  const auto& r = header.radii;
  LameSample samples[kQuarterSamples];
  SampleLame(ex, ey, samples);
  PathBuilder b(kSuperPoints + 1, kSuperPoints);
  LameQuarter(b, r[4], r[5], width - r[4], height - r[5], samples, 0);
  LameQuarter(b, r[6], r[7], r[6], height - r[7], samples, 1);
  LameQuarter(b, r[0], r[1], r[0], r[1], samples, 2);
  LameQuarter(b, r[2], r[3], width - r[2], r[3], samples, 3);
  b.Close();
  return b.Build(factory);
}
fml::RefPtr<Path> Path::MakeSvgPathData(base::String data, float scale,
                                        const PathImplFactory& factory) {
  if (!std::isfinite(scale) || scale <= 0 || data.length() > kMaxSvgBytes ||
      std::memchr(data.c_str(), '\0', data.length()))
    return nullptr;
  return MakeFromSource(PathSource(std::move(data), scale), factory);
}

PathBuilder::PathBuilder(size_t commands, size_t points)
    : in_subpath_(false), valid_(true), start_point_index_(0) {
  header_.kind = PathKind::kCommands;
  valid_ = Reserve(commands, points);
}
PathBuilder::~PathBuilder() { std::free(geometry_); }
bool PathBuilder::Reserve(size_t commands, size_t points) {
  if (commands > 2 * Path::kMaxPoints || points > Path::kMaxPoints)
    return false;
  if (commands <= command_capacity_ && points <= point_capacity_) return true;
  commands = std::max(commands, static_cast<size_t>(command_capacity_));
  points = std::max(points, static_cast<size_t>(point_capacity_));
  void* grown = std::realloc(geometry_, GeometryBytes(commands, points));
  if (!grown) return false;
  if (header_.point_count && commands != command_capacity_)
    std::memmove(GeometryPoints(grown, commands),
                 GeometryPoints(grown, command_capacity_),
                 header_.point_count * sizeof(PathPoint));
  geometry_ = grown;
  command_capacity_ = static_cast<uint32_t>(commands);
  point_capacity_ = static_cast<uint32_t>(points);
  return true;
}
bool PathBuilder::Grow(uint32_t point_count) {
  if (point_count > Path::kMaxPoints ||
      header_.command_count >= 2 * Path::kMaxPoints)
    return false;
  size_t commands = command_capacity_, capacity = point_capacity_;
  const bool grow_commands = header_.command_count == commands;
  const bool grow_points = point_count > capacity;
  // Grow a nearly full companion region together to avoid alternating
  // reallocations for command and point arrays. Preserve asymmetric reserves.
  if (grow_commands ||
      (grow_points && 2 * (header_.command_count + 1) >= commands))
    commands =
        std::min(2 * Path::kMaxPoints, std::max(size_t{8}, 2 * commands));
  if (grow_points || (grow_commands && point_count != header_.point_count &&
                      2 * point_count >= capacity))
    capacity = std::min(Path::kMaxPoints, std::max(size_t{16}, 2 * capacity));
  return Reserve(commands, capacity);
}
PathPoint* PathBuilder::Append(PathVerb verb, uint32_t count) {
  if (!valid_) return nullptr;
  const uint32_t point_count = header_.point_count + count;
  if ((header_.command_count == command_capacity_ ||
       point_count > point_capacity_) &&
      !Grow(point_count)) {
    valid_ = false;
    return nullptr;
  }
  GeometryCommands(geometry_)[header_.command_count++] = verb;
  PathPoint* points =
      GeometryPoints(geometry_, command_capacity_) + header_.point_count;
  header_.point_count = point_count;
  return points;
}
void PathBuilder::MoveTo(float x, float y) {
  if (in_subpath_) End();
  in_subpath_ = true;
  if (!ValidPoint(x, y)) {
    valid_ = false;
    return;
  }
  if (auto* point = Append(PathVerb::kBegin, 1)) {
    *point = {x, y};
    start_point_index_ = header_.point_count - 1;
  }
}
void PathBuilder::LineTo(float x, float y) {
  if (!ValidPoint(x, y) || (UNLIKELY(!in_subpath_) && !ResumePath())) {
    valid_ = false;
    return;
  }
  if (auto* point = Append(PathVerb::kLine, 1)) *point = {x, y};
}
void PathBuilder::QuadTo(float cx, float cy, float x, float y) {
  if (!ValidPoint(cx, cy) || !ValidPoint(x, y) ||
      (UNLIKELY(!in_subpath_) && !ResumePath())) {
    valid_ = false;
    return;
  }
  if (auto* points = Append(PathVerb::kQuadratic, 2)) {
    points[0] = {cx, cy};
    points[1] = {x, y};
  }
}
void PathBuilder::CubicTo(float cx1, float cy1, float cx2, float cy2, float x,
                          float y) {
  if (!ValidPoint(cx1, cy1) || !ValidPoint(cx2, cy2) || !ValidPoint(x, y) ||
      (UNLIKELY(!in_subpath_) && !ResumePath())) {
    valid_ = false;
    return;
  }
  if (auto* points = Append(PathVerb::kCubic, 3)) {
    points[0] = {cx1, cy1};
    points[1] = {cx2, cy2};
    points[2] = {x, y};
  }
}
void PathBuilder::Close() {
  if (!in_subpath_) {
    if (!header_.point_count) valid_ = false;
    return;
  }
  Append(PathVerb::kClose, 0);
  in_subpath_ = false;
}
void PathBuilder::End() {
  if (!in_subpath_) {
    valid_ = false;
    return;
  }
  Append(PathVerb::kEnd, 0);
  in_subpath_ = false;
}
bool PathBuilder::ResumePath() {
  if (!valid_ || !header_.point_count) return false;
  const auto point =
      GeometryPoints(geometry_, command_capacity_)[start_point_index_];
  MoveTo(point.x, point.y);
  return valid_;
}
LBSPathConsumer PathBuilder::GetConsumer() {
  LBSPathConsumer c{};
  c.ctx = this;
  c.MoveToPoint = [](void* ctx, float x, float y) {
    static_cast<PathBuilder*>(ctx)->MoveTo(x, y);
  };
  c.LineToPoint = [](void* ctx, float x, float y) {
    static_cast<PathBuilder*>(ctx)->LineTo(x, y);
  };
  c.QuadToPoint = [](void* ctx, float cx, float cy, float x, float y) {
    static_cast<PathBuilder*>(ctx)->QuadTo(cx, cy, x, y);
  };
  c.CubicToPoint = [](void* ctx, float a, float b, float c, float d, float x,
                      float y) {
    static_cast<PathBuilder*>(ctx)->CubicTo(a, b, c, d, x, y);
  };
  c.ClosePath = [](void* ctx) { static_cast<PathBuilder*>(ctx)->Close(); };
  c.EllipticToPoint = [](void* ctx, float x0, float y0, float rx, float ry,
                         float rotation, bool large, bool positive, float x1,
                         float y1) {
    auto* builder = static_cast<PathBuilder*>(ctx);
    if (!builder->valid_) return;
    if (!builder->header_.point_count ||
        !ArcToCubics(*builder, x0, y0, rx, ry, rotation, large, positive, x1,
                     y1))
      builder->valid_ = false;
  };
  return c;
}
std::optional<PathSource> PathBuilder::BuildSource() {
  if (in_subpath_) End();
  std::optional<PathSource> result;
  if (valid_) {
    if (header_.point_count && command_capacity_ != header_.command_count)
      std::memmove(GeometryPoints(geometry_, header_.command_count),
                   GeometryPoints(geometry_, command_capacity_),
                   header_.point_count * sizeof(PathPoint));
    result = PathSource(header_, geometry_);
    geometry_ = nullptr;
  }
  std::free(geometry_);
  geometry_ = nullptr;
  header_ = {};
  header_.kind = PathKind::kCommands;
  command_capacity_ = point_capacity_ = 0;
  start_point_index_ = 0;
  in_subpath_ = false;
  valid_ = true;
  return result;
}
fml::RefPtr<Path> PathBuilder::Build(const PathImplFactory& factory) {
  auto source = BuildSource();
  return source ? Path::MakeFromSource(std::move(*source), factory) : nullptr;
}
}  // namespace lynx::gfx
