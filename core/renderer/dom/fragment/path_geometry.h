// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FRAGMENT_PATH_GEOMETRY_H_
#define CORE_RENDERER_DOM_FRAGMENT_PATH_GEOMETRY_H_

#include "base/include/fml/memory/ref_ptr.h"
#include "base/include/value/array.h"
#include "core/renderer/dom/fragment/path.h"

namespace lynx {
namespace tasm {

// Resolve the computed CSS basic shape in the element's border-box space.
// False means the declaration cannot be resolved; an empty outline is valid.
bool ResolveClipOutline(const lepus::CArray& shape, float width, float height,
                        float layouts_per_css_px, PathGeometry& out);

// Reuse a Fragment's CSS outline until its style, size or unit scale changes.
class ClipPathCache {
 public:
  const PathGeometry* Resolve(const fml::RefPtr<lepus::CArray>& style,
                              float width, float height, float css_scale);

 private:
  fml::RefPtr<lepus::CArray> style_;
  float width_ = 0, height_ = 0, css_scale_ = 0;
  PathGeometry path_;
  bool valid_ = false;
};
}  // namespace tasm
}  // namespace lynx
#endif  // CORE_RENDERER_DOM_FRAGMENT_PATH_GEOMETRY_H_
