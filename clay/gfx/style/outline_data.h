// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_STYLE_OUTLINE_DATA_H_
#define CLAY_GFX_STYLE_OUTLINE_DATA_H_

#include <limits.h>

#include <array>
#include <tuple>

#include "clay/gfx/style/borders_data.h"

namespace clay {

class OutlineData {
 public:
  OutlineData();
  void Reset();

  float width_;
  float offset_;
  unsigned int color_;
  BorderStyleType style_;

  bool operator==(const OutlineData& rhs) const {
    return std::tie(width_, offset_, color_, style_) ==
           std::tie(rhs.width_, rhs.offset_, rhs.color_, rhs.style_);
  }
};

}  // namespace clay

#endif  // CLAY_GFX_STYLE_OUTLINE_DATA_H_
