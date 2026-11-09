// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef GFX_PUBLIC_PATH_H_
#define GFX_PUBLIC_PATH_H_

#include "base/include/fml/memory/ref_counted.h"
#include "gfx/public/path_impl.h"
#include "gfx/public/path_parser.h"
namespace lynx::gfx {
// Retaining the facade keeps its immutable source views and impl key alive
// across the layout/UI display-list handoff.
class Path final : public fml::RefCountedThreadSafe<Path> {
 public:
  static constexpr size_t kMaxPoints = 1u << 20;
  GFX_EXPORT static fml::RefPtr<Path> MakeRect(float width, float height);
  GFX_EXPORT static fml::RefPtr<Path> MakeRoundedRect(float width, float height,
                                                      const float (&radii)[8]);
  GFX_EXPORT static fml::RefPtr<Path> MakeEllipse(float rx, float ry);
  GFX_EXPORT static fml::RefPtr<Path> MakeSuperEllipse(float rx, float ry,
                                                       float exponent_x,
                                                       float exponent_y);
  GFX_EXPORT static fml::RefPtr<Path> MakeSuperRoundedRect(
      float width, float height, const float (&radii)[8], float exponent_x,
      float exponent_y);
  static constexpr size_t kMaxSvgBytes = 1u << 20;
  // Retains bounded source bytes without parsing SVG syntax. Consumers must
  // check parser errors before publishing native geometry.
  GFX_EXPORT static fml::RefPtr<Path> MakeSvgPathData(
      base::String data, float source_to_layout_scale);

  uint64_t GetKey() const { return impl_.GetKey(); }
  PathSourceView GetSourceView() const { return impl_.GetSourceView(); }
  const PathImpl& GetImpl() const { return impl_; }

 private:
  friend class fml::RefCountedThreadSafe<Path>;
  friend class PathBuilder;
  explicit Path(PathSource source);
  GFX_EXPORT ~Path();
  const PathImpl impl_;
};

// Builds packed immutable geometry. Known-size producers
// can reserve once; Build transfers the allocation without rescanning geometry.
// Command paths do not record bounds.
class PathBuilder {
 public:
  GFX_EXPORT explicit PathBuilder(size_t command_capacity = 0,
                                  size_t point_capacity = 0);
  GFX_EXPORT ~PathBuilder();
  PathBuilder(const PathBuilder&) = delete;
  PathBuilder& operator=(const PathBuilder&) = delete;
  // MoveTo ends the previous open subpath with End before starting a new one.
  // Drawing requires a MoveTo; after Close it restarts at the previous start.
  GFX_EXPORT void MoveTo(float x, float y);
  GFX_EXPORT void LineTo(float x, float y);
  GFX_EXPORT void QuadTo(float cx, float cy, float x, float y);
  GFX_EXPORT void CubicTo(float cx1, float cy1, float cx2, float cy2, float x,
                          float y);
  GFX_EXPORT void Close();
  // Adapts the drawing methods to the shared C interface, reopening a closed
  // subpath at its start when drawing continues. Elliptic arcs are
  // unsupported. Check parser errors before publishing preceding segments.
  GFX_EXPORT LBSPathConsumer GetConsumer();
  // Ends the last open subpath and transfers immutable storage to the Path.
  GFX_EXPORT fml::RefPtr<Path> Build();

 private:
  void End();
  bool Reserve(size_t command_capacity, size_t point_capacity);
  bool Grow(uint32_t point_count);
  PathPoint* Append(PathVerb verb, uint32_t count);
  bool ResumePath();
  PathHeader header_{};
  // Point indices fit in 20 bits; the flags share the same storage word.
  static_assert(Path::kMaxPoints <= (1u << 20));
  uint32_t in_subpath_ : 1;
  uint32_t valid_ : 1;
  uint32_t start_point_index_ : 20;
  void* geometry_{nullptr};
  uint32_t command_capacity_{0};
  uint32_t point_capacity_{0};
};

}  // namespace lynx::gfx

#endif  // GFX_PUBLIC_PATH_H_
