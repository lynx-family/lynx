// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FRAGMENT_PATH_CACHE_H_
#define CORE_RENDERER_DOM_FRAGMENT_PATH_CACHE_H_

#include <array>
#include <cstddef>

#include "gfx/public/path.h"
#include "gfx/public/rounded_rectangle.h"

namespace lynx::tasm {

class FragmentPathCache {
 public:
  enum class Slot : size_t { kContent, kPadding, kBorder, kOverflow, kCount };

  fml::RefPtr<gfx::Path> Get(Slot slot, const RoundedRectangle& box) {
    const float radii[] = {
        box.GetRadiusXTopLeft(),     box.GetRadiusYTopLeft(),
        box.GetRadiusXTopRight(),    box.GetRadiusYTopRight(),
        box.GetRadiusXBottomRight(), box.GetRadiusYBottomRight(),
        box.GetRadiusXBottomLeft(),  box.GetRadiusYBottomLeft()};
    std::array<float, 10> shape{box.GetWidth(), box.GetHeight()};
    for (size_t i = 0; i < 8; ++i) shape[i + 2] = radii[i];
    auto& entry = entries_[static_cast<size_t>(slot)];
    if (entry.path && entry.shape == shape) return entry.path;
    entry.shape = shape;
    entry.path =
        box.HasRadius()
            ? gfx::Path::MakeRoundedRect(box.GetWidth(), box.GetHeight(), radii)
            : gfx::Path::MakeRect(box.GetWidth(), box.GetHeight());
    return entry.path;
  }

 private:
  struct Entry {
    std::array<float, 10> shape{};
    fml::RefPtr<gfx::Path> path;
  };
  std::array<Entry, static_cast<size_t>(Slot::kCount)> entries_;
};

}  // namespace lynx::tasm

#endif  // CORE_RENDERER_DOM_FRAGMENT_PATH_CACHE_H_
