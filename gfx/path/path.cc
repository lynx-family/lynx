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
}  // namespace

PathSourceView PathSource::GetSourceView() const {
  if (source_to_layout_scale_ > 0)
    return SvgPathView{svg_.str(), source_to_layout_scale_};
  return PathView{
      header_, geometry_ ? GeometryCommands(geometry_.get()) : nullptr,
      geometry_ ? GeometryPoints(geometry_.get(), header_.command_count)
                : nullptr};
}
Path::Path(PathSource source) : impl_(std::move(source)) {}
Path::~Path() = default;

fml::RefPtr<Path> Path::MakeRect(float width, float height) {
  if (!ValidSize(width, height)) return nullptr;
  PathHeader header{};
  header.kind = PathKind::kRect;
  header.width = width;
  header.height = height;
  return fml::AdoptRef(new Path(PathSource(header)));
}
fml::RefPtr<Path> Path::MakeSvgPathData(base::String data, float scale) {
  if (!std::isfinite(scale) || scale <= 0 || data.length() > kMaxSvgBytes ||
      std::memchr(data.c_str(), '\0', data.length()))
    return nullptr;
  return fml::AdoptRef(new Path(PathSource(std::move(data), scale)));
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
  c.EllipticToPoint = [](void* ctx, float, float, float, float, float, bool,
                         bool, float, float) {
    static_cast<PathBuilder*>(ctx)->valid_ = false;
  };
  return c;
}
fml::RefPtr<Path> PathBuilder::Build() {
  if (in_subpath_) End();
  fml::RefPtr<Path> result;
  if (valid_) {
    if (header_.point_count && command_capacity_ != header_.command_count)
      std::memmove(GeometryPoints(geometry_, header_.command_count),
                   GeometryPoints(geometry_, command_capacity_),
                   header_.point_count * sizeof(PathPoint));
    result = fml::AdoptRef(new Path(PathSource(header_, geometry_)));
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
}  // namespace lynx::gfx
