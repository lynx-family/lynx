// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FRAGMENT_CLIP_PATH_RESOLVER_H_
#define CORE_RENDERER_DOM_FRAGMENT_CLIP_PATH_RESOLVER_H_
#include "core/style/basic_shape_data.h"
#include "gfx/public/path.h"
namespace lynx::tasm {
// kValid means recordable source; SVG syntax is checked by the native consumer.
enum class ClipPathStatus { kNoClip, kValid, kInvalid };
struct ResolvedClipPath {
  ClipPathStatus status = ClipPathStatus::kNoClip;
  fml::RefPtr<gfx::Path> path;
  gfx::PathFillRule fill_rule = gfx::PathFillRule::kNonzero;
  float x = 0;
  float y = 0;
  bool depends_on_width = false;
  bool depends_on_height = false;
  bool depends_on_source_scale = false;
};
// Computed NUMBER lengths already use layout units. SVG user units carry a
// separate scale and remain in their original source representation.
ResolvedClipPath ResolveClipPath(const starlight::BasicShapeData* computed,
                                 float width, float height,
                                 float source_to_layout_scale);
}  // namespace lynx::tasm
#endif  // CORE_RENDERER_DOM_FRAGMENT_CLIP_PATH_RESOLVER_H_
