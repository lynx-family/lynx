// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef GFX_PUBLIC_PATH_H_
#define GFX_PUBLIC_PATH_H_

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <vector>

#include "base/include/fml/memory/ref_counted.h"
#include "gfx/gfx_export.h"

namespace lynx::gfx {

enum class PathKind : uint32_t { kRect = 0, kRoundedRect = 1, kCommands = 2 };
enum class PathFillRule : int32_t { kNonzero = 0, kEvenOdd = 1 };
enum class PathVerb : uint32_t {
  kBegin = 0,
  kLine = 1,
  kQuadratic = 2,
  kCubic = 3,
  kClose = 4,
  kEnd = 5,
};

struct PathPoint {
  float x;
  float y;
};

// Read-only native-buffer ABI. Bounds include control points, not just
// endpoints.
struct PathHeader {
  PathKind kind;
  uint32_t command_count;
  uint32_t point_count;
  float x;
  float y;
  float width;
  float height;
  float radii[8];
};

static_assert(sizeof(PathHeader) == 60);
static_assert(offsetof(PathHeader, x) == 12);
static_assert(offsetof(PathHeader, radii) == 28);
static_assert(sizeof(PathPoint) == 8);
static_assert(sizeof(PathVerb) == 4);
static_assert(std::is_standard_layout<PathHeader>::value);
static_assert(std::is_trivially_copyable<PathHeader>::value);

// A borrowed view remains valid while its immutable Path is retained.
struct PathView {
  const PathHeader& header;
  const PathVerb* commands;
  const PathPoint* points;
};

class GFX_EXPORT Path final : public fml::RefCountedThreadSafe<Path> {
 public:
  static constexpr size_t kMaxPoints = 1u << 20;
  static fml::RefPtr<Path> MakeRect(float width, float height);
  static fml::RefPtr<Path> MakeRoundedRect(float width, float height,
                                           const float (&radii)[8]);
  static fml::RefPtr<Path> MakeCommands(std::vector<PathVerb> commands,
                                        std::vector<PathPoint> points);

  uint64_t GetKey() const { return key_; }
  const PathHeader& GetHeader() const { return header_; }
  const std::vector<PathVerb>& GetCommands() const { return commands_; }
  const std::vector<PathPoint>& GetPoints() const { return points_; }
  PathView GetView() const {
    return {header_, commands_.data(), points_.data()};
  }
  bool IsEmpty() const { return header_.width <= 0 || header_.height <= 0; }

 private:
  friend class fml::RefCountedThreadSafe<Path>;
  Path(PathHeader header, std::vector<PathVerb> commands,
       std::vector<PathPoint> points);
  ~Path() = default;

  const uint64_t key_;
  const PathHeader header_;
  const std::vector<PathVerb> commands_;
  const std::vector<PathPoint> points_;
};

class GFX_EXPORT PathBuilder {
 public:
  void Begin(float x, float y);
  void LineTo(float x, float y);
  void QuadraticTo(float cx, float cy, float x, float y);
  void CubicTo(float cx1, float cy1, float cx2, float cy2, float x, float y);
  void End(bool close = false);
  fml::RefPtr<Path> Build();

 private:
  std::vector<PathVerb> commands_;
  std::vector<PathPoint> points_;
  bool in_subpath_{false};
  bool valid_{true};
};

}  // namespace lynx::gfx

#endif  // GFX_PUBLIC_PATH_H_
