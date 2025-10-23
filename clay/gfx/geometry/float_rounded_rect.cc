/*
 * Copyright 2021 The Lynx Authors. All rights reserved.
 * Licensed under the Apache License Version 2.0 that can be found in the
 * LICENSE file in the root directory of this source tree.
 *
 * Copyright (C) 2013 Adobe Systems Incorporated. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "clay/gfx/geometry/float_rounded_rect.h"

#include <algorithm>

namespace clay {

FloatRoundedRect::FloatRoundedRect(float x, float y, float width, float height)
    : rect_(x, y, width, height) {}

FloatRoundedRect::FloatRoundedRect(const FloatRect& rect, const Radii& radii)
    : rect_(rect), radii_(radii) {}

FloatRoundedRect::FloatRoundedRect(const FloatRect& rect,
                                   const FloatSize& top_left,
                                   const FloatSize& top_right,
                                   const FloatSize& bottom_left,
                                   const FloatSize& bottom_right)
    : rect_(rect), radii_(top_left, top_right, bottom_left, bottom_right) {}

bool FloatRoundedRect::Radii::IsZero() const {
  return top_left_.IsZero() && top_right_.IsZero() && bottom_left_.IsZero() &&
         bottom_right_.IsZero();
}

void FloatRoundedRect::SetOval(const FloatRect& rect) {
  rect_ = rect;
  FloatSize fs = {rect.width() / 2.0f, rect.height() / 2.0f};
  radii_ = {fs, fs, fs, fs};
}

void FloatRoundedRect::Radii::Expand(float top_width, float right_width,
                                     float bottom_width, float left_width) {
  if (top_left_.width() > 0 && top_left_.height() > 0) {
    top_left_.SetWidth(std::max<float>(0, top_left_.width() + left_width));
    top_left_.SetHeight(std::max<float>(0, top_left_.height() + top_width));
  }
  if (top_right_.width() > 0 && top_right_.height() > 0) {
    top_right_.SetWidth(std::max<float>(0, top_right_.width() + right_width));
    top_right_.SetHeight(std::max<float>(0, top_right_.height() + top_width));
  }
  if (bottom_left_.width() > 0 && bottom_left_.height() > 0) {
    bottom_left_.SetWidth(
        std::max<float>(0, bottom_left_.width() + left_width));
    bottom_left_.SetHeight(
        std::max<float>(0, bottom_left_.height() + bottom_width));
  }
  if (bottom_right_.width() > 0 && bottom_right_.height() > 0) {
    bottom_right_.SetWidth(
        std::max<float>(0, bottom_right_.width() + right_width));
    bottom_right_.SetHeight(
        std::max<float>(0, bottom_right_.height() + bottom_width));
  }
}

void FloatRoundedRect::Radii::Shrink(float top_width, float right_width,
                                     float bottom_width, float left_width) {
  top_left_.SetWidth(std::max<float>(0, top_left_.width() - left_width));
  top_left_.SetHeight(std::max<float>(0, top_left_.height() - top_width));

  top_right_.SetWidth(std::max<float>(0, top_right_.width() - right_width));
  top_right_.SetHeight(std::max<float>(0, top_right_.height() - top_width));

  bottom_left_.SetWidth(std::max<float>(0, bottom_left_.width() - left_width));
  bottom_left_.SetHeight(
      std::max<float>(0, bottom_left_.height() - bottom_width));

  bottom_right_.SetWidth(
      std::max<float>(0, bottom_right_.width() - right_width));
  bottom_right_.SetHeight(
      std::max<float>(0, bottom_right_.height() - bottom_width));
}

bool FloatRoundedRect::IsRenderable() const {
  return radii_.TopLeft().width() + radii_.TopRight().width() <=
             rect_.width() + 0.0001 &&
         radii_.BottomLeft().width() + radii_.BottomRight().width() <=
             rect_.width() + 0.0001 &&
         radii_.TopLeft().height() + radii_.BottomLeft().height() <=
             rect_.height() + 0.0001 &&
         radii_.TopRight().height() + radii_.BottomRight().height() <=
             rect_.height() + 0.0001;
}

void FloatRoundedRect::Radii::Scale(float factor) {
  if (factor == 1) {
    return;
  }

  top_left_.ScaleSize(factor);
  if (!top_left_.width() || !top_left_.height()) {
    top_left_ = FloatSize();
  }
  top_right_.ScaleSize(factor);
  if (!top_right_.width() || !top_right_.height()) {
    top_right_ = FloatSize();
  }
  bottom_left_.ScaleSize(factor);
  if (!bottom_left_.width() || !bottom_left_.height()) {
    bottom_left_ = FloatSize();
  }
  bottom_right_.ScaleSize(factor);
  if (!bottom_right_.width() || !bottom_right_.height()) {
    bottom_right_ = FloatSize();
  }
}

void FloatRoundedRect::ConstraintRadii() {
  // Consider precision problem here. When factor is float,
  // IsRenderable() may return false.
  float factor = 1;
  float horizontal_sum =
      std::max(radii().TopLeft().width() + radii().TopRight().width(),
               radii().BottomLeft().width() + radii().BottomRight().width());
  if (horizontal_sum > rect().width()) {
    factor = std::min(rect().width() / horizontal_sum, factor);
  }
  float vertical_sum =
      std::max(radii().TopLeft().height() + radii().BottomLeft().height(),
               radii().TopRight().height() + radii().BottomRight().height());
  if (vertical_sum > rect().height()) {
    factor = std::min(rect().height() / vertical_sum, factor);
  }
  int precision = 1e6 * factor;
  factor = precision / 1e6;
  radii_.Scale(factor);
}
}  // namespace clay
