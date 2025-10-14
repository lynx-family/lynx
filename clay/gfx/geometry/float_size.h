/*
 * Copyright (C) 2003, 2006 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2005 Nokia.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Copyright 2021 The Lynx Authors. All rights reserved.
 * Licensed under the Apache License Version 2.0 that can be found in the
 * LICENSE file in the root directory of this source tree.
 */

#ifndef CLAY_GFX_GEOMETRY_FLOAT_SIZE_H_
#define CLAY_GFX_GEOMETRY_FLOAT_SIZE_H_

#include <math.h>

#include <cmath>
#include <limits>
#include <sstream>
#include <string>

#include "clay/gfx/geometry/size.h"

namespace clay {

class FloatSize {
 public:
  FloatSize() : width_(0.0f), height_(0.0f) {}

  FloatSize(float width, float height) : width_(width), height_(height) {}
  explicit FloatSize(const Size& size)
      : width_(size.width()), height_(size.height()) {}

  float width() const { return width_; }
  float height() const { return height_; }

  float distance() const { return hypotf(width(), height()); }

  void SetWidth(float width) { width_ = width; }
  void SetHeight(float height) { height_ = height; }

  bool IsEmpty() const { return width_ <= 0.0f || height_ <= 0.0f; }

  bool IsZero() const {
    return fabs(width_) < std::numeric_limits<float>::epsilon() &&
           fabs(height_) < std::numeric_limits<float>::epsilon();
  }

  void Expand(float width, float height) {
    width_ += width;
    height_ += height;
  }

  void ExpandByRatio(float ratio) {
    width_ *= ratio;
    height_ *= ratio;
  }

  std::string ToString() const {
    std::stringstream ss;
    ss << "(width: " << width() << ", height: " << height() << ")";
    return ss.str();
  }

  void ScaleSize(float factor_x, float factor_y) {
    width_ *= factor_x;
    height_ *= factor_y;
  }

  void ScaleSize(float factor) { ScaleSize(factor, factor); }

 private:
  float width_;
  float height_;
};

inline FloatSize& operator+=(FloatSize& a, const FloatSize& b) {
  a.SetWidth(a.width() + b.width());
  a.SetHeight(a.height() + b.height());
  return a;
}

inline FloatSize& operator-=(FloatSize& a, const FloatSize& b) {
  a.SetWidth(a.width() - b.width());
  a.SetHeight(a.height() - b.height());
  return a;
}

inline FloatSize operator+(const FloatSize& a, const FloatSize& b) {
  return FloatSize(a.width() + b.width(), a.height() + b.height());
}

inline FloatSize operator-(const FloatSize& a, const FloatSize& b) {
  return FloatSize(a.width() - b.width(), a.height() - b.height());
}

inline FloatSize operator-(const FloatSize& size) {
  return FloatSize(-size.width(), -size.height());
}

inline FloatSize operator*(const FloatSize& a, float factor) {
  return FloatSize(a.width() * factor, a.height() * factor);
}

inline bool operator==(const FloatSize& a, const FloatSize& b) {
  return a.width() == b.width() && a.height() == b.height();
}

inline bool operator!=(const FloatSize& a, const FloatSize& b) {
  return a.width() != b.width() || a.height() != b.height();
}

}  // namespace clay

#endif  // CLAY_GFX_GEOMETRY_FLOAT_SIZE_H_
