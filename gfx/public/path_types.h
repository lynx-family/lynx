// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef GFX_PUBLIC_PATH_TYPES_H_
#define GFX_PUBLIC_PATH_TYPES_H_
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <new>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>

#include "base/include/value/base_string.h"
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

// In-memory metadata, exposed by reference through PathView. Rectangle kinds
// use x/y/width/height; command paths leave those fields zero.
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

static_assert(std::is_trivially_copyable<PathPoint>::value);
static_assert(alignof(PathVerb) == alignof(PathPoint));

// A borrowed view remains valid while its immutable Path is retained.
struct PathView {
  const PathHeader& header;
  const PathVerb* commands;
  const PathPoint* points;
};
struct SvgPathView {
  // Syntax is checked when a native consumer builds the path.
  const std::string& data;
  float source_to_layout_scale;
};
using PathSourceView = std::variant<PathView, SvgPathView>;

// Constructed only after a raw builder allocation becomes immutable. The
// command and point arrays follow this reference count in the same allocation.
class PathStorage final : public fml::RefCountedThreadSafe<PathStorage> {
 public:
  static void operator delete(void* storage) { std::free(storage); }

 private:
  friend class PathSource;
  friend class fml::RefCountedThreadSafe<PathStorage>;
  PathStorage() = default;
  ~PathStorage() = default;
};
static_assert(sizeof(PathStorage) % alignof(PathPoint) == 0);

// Geometry is validated and owns one packed command/point allocation. SVG
// retains the bounded original base string for parsing by a native consumer.
class PathSource {
 public:
  PathSource(const PathSource&) = default;
  PathSource& operator=(const PathSource&) = default;
  PathSource(PathSource&& other) noexcept
      : header_(other.header_),
        geometry_(std::move(other.geometry_)),
        svg_(std::move(other.svg_)),
        source_to_layout_scale_(other.source_to_layout_scale_) {
    other.header_ = {};
    other.source_to_layout_scale_ = 0;
  }
  PathSource& operator=(PathSource&& other) noexcept {
    if (this != &other) {
      header_ = other.header_;
      geometry_ = std::move(other.geometry_);
      svg_ = std::move(other.svg_);
      source_to_layout_scale_ = other.source_to_layout_scale_;
      other.header_ = {};
      other.source_to_layout_scale_ = 0;
    }
    return *this;
  }
  GFX_EXPORT PathSourceView GetSourceView() const;

 private:
  friend class Path;
  friend class PathBuilder;
  explicit PathSource(PathHeader header, void* geometry = nullptr)
      : header_(header),
        geometry_(geometry ? fml::AdoptRef(::new(geometry) PathStorage)
                           : nullptr) {}
  PathSource(base::String svg, float scale)
      : svg_(std::move(svg)), source_to_layout_scale_(scale) {}
  PathHeader header_{};
  fml::RefPtr<PathStorage> geometry_;
  base::String svg_;
  // Positive only for SVG; validated geometry has no source scale.
  float source_to_layout_scale_{0};
};
}  // namespace lynx::gfx
#endif  // GFX_PUBLIC_PATH_TYPES_H_
