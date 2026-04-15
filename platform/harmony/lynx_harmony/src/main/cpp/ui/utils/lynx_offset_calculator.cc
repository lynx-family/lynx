// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/utils/lynx_offset_calculator.h"

#include <native_drawing/drawing_path.h>
#include <native_drawing/drawing_point.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <memory>
#include <mutex>
#include <utility>

#include "base/include/lru_cache.h"

namespace lynx {
namespace tasm {
namespace harmony {

namespace {
constexpr size_t kOffsetPathCacheMaxEntries = 10;

struct OffsetPathCacheValue {
  std::shared_ptr<OH_Drawing_Path> path;
  float total_length{0.0f};
};

lynx::base::LRUCache<std::string, OffsetPathCacheValue> g_offset_path_cache(
    kOffsetPathCacheMaxEntries);
std::mutex g_offset_path_cache_mutex;

static inline float RadToDeg(float rad) { return rad * 180.0f / M_PI; }

static std::shared_ptr<OH_Drawing_Path> BuildPathFromSvgString(
    const std::string& svg_path_string, float* out_total_length) {
  OH_Drawing_Path* raw_path = OH_Drawing_PathCreate();
  if (raw_path == nullptr) {
    return nullptr;
  }

  auto path = std::shared_ptr<OH_Drawing_Path>(
      raw_path, [](OH_Drawing_Path* p) { OH_Drawing_PathDestroy(p); });

  OH_Drawing_PathReset(raw_path);
  if (!OH_Drawing_PathBuildFromSvgString(raw_path, svg_path_string.c_str())) {
    return nullptr;
  }

  *out_total_length = OH_Drawing_PathGetLength(raw_path, /*forceClosed*/ false);
  return path;
}

}  // namespace

void LynxOffsetCalculator::Reset() {
  cached_path_string_.clear();
  cached_path_.reset();
  cached_path_length_ = 0.0f;
}

bool LynxOffsetCalculator::UpdatePathString(
    const std::string& svg_path_string) {
  if (svg_path_string.empty()) {
    Reset();
    return false;
  }

  if (svg_path_string == cached_path_string_ && cached_path_) {
    std::lock_guard<std::mutex> lock(g_offset_path_cache_mutex);
    g_offset_path_cache.Get(svg_path_string);
    return true;
  }

  {
    std::lock_guard<std::mutex> lock(g_offset_path_cache_mutex);
    if (auto* cached = g_offset_path_cache.Get(svg_path_string)) {
      Reset();
      cached_path_ = cached->path;
      cached_path_length_ = cached->total_length;
      cached_path_string_ = svg_path_string;
      return true;
    }
  }

  Reset();

  float total_length = 0.0f;
  auto path = BuildPathFromSvgString(svg_path_string, &total_length);
  if (!path) {
    Reset();
    return false;
  }

  cached_path_string_ = svg_path_string;
  cached_path_ = std::move(path);
  cached_path_length_ = total_length;

  OffsetPathCacheValue cache_value;
  cache_value.path = cached_path_;
  cache_value.total_length = cached_path_length_;
  {
    std::lock_guard<std::mutex> lock(g_offset_path_cache_mutex);
    g_offset_path_cache.Put(svg_path_string, std::move(cache_value));
  }

  return true;
}

std::optional<OffsetMotionState> LynxOffsetCalculator::PointAtProgress(
    const std::string& svg_path_string, float progress) {
  if (!UpdatePathString(svg_path_string)) {
    return std::nullopt;
  }
  return GetMotionState(progress);
}

std::optional<OffsetMotionState> LynxOffsetCalculator::GetMotionState(
    float progress) const {
  if (!cached_path_) {
    return std::nullopt;
  }

  progress = std::clamp(progress, 0.0f, 1.0f);

  if (cached_path_length_ <= 0.0f) {
    return std::nullopt;
  }

  float distance = cached_path_length_ * progress;

  OH_Drawing_Point2D position{0.0f, 0.0f};
  OH_Drawing_Point2D tangent{0.0f, 0.0f};
  if (!OH_Drawing_PathGetPositionTangent(cached_path_.get(),
                                         /*forceClosed*/ false, distance,
                                         &position, &tangent)) {
    return std::nullopt;
  }

  float deg = RadToDeg(std::atan2(tangent.y, tangent.x));
  return OffsetMotionState{position.x, position.y, deg};
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
