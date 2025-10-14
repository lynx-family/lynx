// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_STYLE_TILE_MODE_H_
#define CLAY_GFX_STYLE_TILE_MODE_H_

#include "clay/gfx/rendering_backend.h"

namespace clay {

// An enum to define how to repeat, fold, or omit colors outside of the
// typically defined range of the source of the colors (such as the
// bounds of an image or the defining geoetry of a gradient).
enum class TileMode {
  // Replicate the edge color if the |DlColorSource| draws outside of the
  // defined bounds.
  kClamp,

  // Repeat the |DlColorSource|'s defined colors both horizontally and
  // vertically (or both along and perpendicular to a gradient's geometry).
  kRepeat,

  // Repeat the |DlColorSource|'s colors horizontally and vertically,
  // alternating mirror images so that adjacent images always seam.
  kMirror,

  // Only draw within the original domain, return transparent-black everywhere
  // else.
  kDecal,
};

#ifndef ENABLE_SKITY
inline TileMode ToClay(SkTileMode sk_mode) {
  return static_cast<TileMode>(sk_mode);
}

inline SkTileMode ToSk(TileMode dl_mode) {
  return static_cast<SkTileMode>(dl_mode);
}
#endif  // ENABLE_SKITY

}  // namespace clay

#endif  // CLAY_GFX_STYLE_TILE_MODE_H_
