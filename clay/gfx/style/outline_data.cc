// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/style/outline_data.h"

#include "clay/gfx/style/color.h"

namespace clay {

OutlineData::OutlineData()
    : width_(0.f),
      offset_(0.f),
      color_(Color::kBlack().Value()),
      style_(BorderStyleType::kSolid) {}

void OutlineData::Reset() {
  width_ = 0.f;
  offset_ = 0.f;
  color_ = Color::kBlack().Value();
  style_ = BorderStyleType::kSolid;
}

}  // namespace clay
