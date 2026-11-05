// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "gfx/public/path.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <utility>

namespace lynx::gfx {
namespace {

uint64_t NextPathKey() {
  static std::atomic<uint64_t> next{1};
  const uint64_t key = next.fetch_add(1, std::memory_order_relaxed);
  if (key == 0) {
    std::abort();
  }
  return key;
}

bool ValidSize(float width, float height) {
  return std::isfinite(width) && std::isfinite(height) && width >= 0 &&
         height >= 0;
}

}  // namespace

Path::Path(PathHeader header, std::vector<PathVerb> commands,
           std::vector<PathPoint> points)
    : key_(NextPathKey()),
      header_(header),
      commands_(std::move(commands)),
      points_(std::move(points)) {}

fml::RefPtr<Path> Path::MakeRect(float width, float height) {
  if (!ValidSize(width, height)) {
    return nullptr;
  }
  PathHeader header{};
  header.kind = PathKind::kRect;
  header.width = width;
  header.height = height;
  return fml::AdoptRef(new Path(header, {}, {}));
}

fml::RefPtr<Path> Path::MakeRoundedRect(float width, float height,
                                        const float (&radii)[8]) {
  if (!ValidSize(width, height)) {
    return nullptr;
  }
  PathHeader header{};
  header.kind = PathKind::kRoundedRect;
  header.width = width;
  header.height = height;
  for (size_t i = 0; i < 8; ++i) {
    if (!std::isfinite(radii[i]) || radii[i] < 0) {
      return nullptr;
    }
    header.radii[i] = radii[i];
  }
  float factor = 1.f;
  const auto limit = [&factor](float extent, float a, float b) {
    const double sum = static_cast<double>(a) + b;
    if (sum > extent) {
      factor = std::min(factor, static_cast<float>(extent / sum));
    }
  };
  limit(width, radii[0], radii[2]);
  limit(width, radii[6], radii[4]);
  limit(height, radii[1], radii[7]);
  limit(height, radii[3], radii[5]);
  for (float& radius : header.radii) {
    radius *= factor;
  }
  return fml::AdoptRef(new Path(header, {}, {}));
}

fml::RefPtr<Path> Path::MakeCommands(std::vector<PathVerb> commands,
                                     std::vector<PathPoint> points) {
  if (points.size() > kMaxPoints || commands.size() > 2 * kMaxPoints) {
    return nullptr;
  }
  size_t consumed = 0;
  bool in_subpath = false;
  for (PathVerb verb : commands) {
    switch (verb) {
      case PathVerb::kBegin:
        if (in_subpath) return nullptr;
        in_subpath = true;
        ++consumed;
        break;
      case PathVerb::kLine:
      case PathVerb::kQuadratic:
      case PathVerb::kCubic:
        if (!in_subpath) return nullptr;
        consumed += verb == PathVerb::kLine
                        ? 1
                        : (verb == PathVerb::kQuadratic ? 2 : 3);
        break;
      case PathVerb::kClose:
      case PathVerb::kEnd:
        if (!in_subpath) return nullptr;
        in_subpath = false;
        break;
      default:
        return nullptr;
    }
    if (consumed > points.size()) return nullptr;
  }
  if (in_subpath || consumed != points.size()) return nullptr;

  PathHeader header{};
  header.kind = PathKind::kCommands;
  header.command_count = static_cast<uint32_t>(commands.size());
  header.point_count = static_cast<uint32_t>(points.size());
  if (!points.empty()) {
    float min_x = std::numeric_limits<float>::max();
    float min_y = min_x;
    float max_x = std::numeric_limits<float>::lowest();
    float max_y = max_x;
    for (const PathPoint& point : points) {
      if (!std::isfinite(point.x) || !std::isfinite(point.y)) return nullptr;
      min_x = std::min(min_x, point.x);
      min_y = std::min(min_y, point.y);
      max_x = std::max(max_x, point.x);
      max_y = std::max(max_y, point.y);
    }
    header.x = min_x;
    header.y = min_y;
    header.width = max_x - min_x;
    header.height = max_y - min_y;
    if (header.x + header.width < max_x) {
      header.width =
          std::nextafter(header.width, std::numeric_limits<float>::infinity());
    }
    if (header.y + header.height < max_y) {
      header.height =
          std::nextafter(header.height, std::numeric_limits<float>::infinity());
    }
    if (!ValidSize(header.width, header.height)) return nullptr;
  }
  return fml::AdoptRef(
      new Path(header, std::move(commands), std::move(points)));
}

void PathBuilder::Begin(float x, float y) {
  if (in_subpath_) End();
  commands_.push_back(PathVerb::kBegin);
  points_.push_back({x, y});
  in_subpath_ = true;
}

void PathBuilder::LineTo(float x, float y) {
  valid_ &= in_subpath_;
  commands_.push_back(PathVerb::kLine);
  points_.push_back({x, y});
}

void PathBuilder::QuadraticTo(float cx, float cy, float x, float y) {
  valid_ &= in_subpath_;
  commands_.push_back(PathVerb::kQuadratic);
  points_.push_back({cx, cy});
  points_.push_back({x, y});
}

void PathBuilder::CubicTo(float cx1, float cy1, float cx2, float cy2, float x,
                          float y) {
  valid_ &= in_subpath_;
  commands_.push_back(PathVerb::kCubic);
  points_.push_back({cx1, cy1});
  points_.push_back({cx2, cy2});
  points_.push_back({x, y});
}

void PathBuilder::End(bool close) {
  valid_ &= in_subpath_;
  commands_.push_back(close ? PathVerb::kClose : PathVerb::kEnd);
  in_subpath_ = false;
}

fml::RefPtr<Path> PathBuilder::Build() {
  if (in_subpath_) End();
  auto result =
      valid_ ? Path::MakeCommands(std::move(commands_), std::move(points_))
             : fml::RefPtr<Path>();
  commands_.clear();
  points_.clear();
  valid_ = true;
  return result;
}

}  // namespace lynx::gfx
