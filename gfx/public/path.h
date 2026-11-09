// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef GFX_PUBLIC_PATH_H_
#define GFX_PUBLIC_PATH_H_

#include "base/include/fml/memory/ref_counted.h"
#include "gfx/public/path_impl.h"
namespace lynx::gfx {
// Retaining the facade keeps its immutable source views and impl key alive
// across the layout/UI display-list handoff.
class Path final : public fml::RefCountedThreadSafe<Path> {
 public:
  static constexpr size_t kMaxPoints = 1u << 20;
  GFX_EXPORT static fml::RefPtr<Path> MakeRect(float width, float height);
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

}  // namespace lynx::gfx

#endif  // GFX_PUBLIC_PATH_H_
