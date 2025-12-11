// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_STYLE_COLOR_FILTER_H_
#define CLAY_GFX_STYLE_COLOR_FILTER_H_

#include <cstring>
#include <memory>
#include <utility>

#include "clay/fml/logging.h"
#include "clay/gfx/attributes.h"
#include "clay/gfx/rendering_backend.h"
#include "clay/gfx/style/blend_mode.h"
#include "clay/gfx/style/color.h"
namespace clay {

class BlendColorFilter;
class MatrixColorFilter;

enum class ColorFilterType {
  kBlend,
  kMatrix,
  kSrgbToLinearGamma,
  kLinearToSrgbGamma,
  kUnknown
};

class ColorFilter
    : public Attribute<ColorFilter, GrColorFilter, ColorFilterType> {
 public:
  static std::shared_ptr<ColorFilter> MakeBlend(Color color, BlendMode mode);

  static std::shared_ptr<ColorFilter> MakeMatrix(const float matrix[20]);

  static std::shared_ptr<ColorFilter> MakeSrgbToLinearGamma();

  static std::shared_ptr<ColorFilter> MakeLinearToSrgbGamma();

  // Return a boolean indicating whether the color filtering operation will
  // modify transparent black. This is typically used to determine if applying
  // the ColorFilter to a temporary saveLayer buffer will turn the surrounding
  // pixels non-transparent and therefore expand the bounds.
  virtual bool modifies_transparent_black() const = 0;

  // Return a boolean indicating whether the color filtering operation can
  // be applied either before or after modulating the pixels with an opacity
  // value without changing the operation.
  virtual bool can_commute_with_opacity() const { return false; }

  // Return a BlendColorFilter pointer to this object iff it is a Blend
  // type of ColorFilter, otherwise return nullptr.
  virtual const BlendColorFilter* asBlend() const { return nullptr; }

  // Return a MatrixColorFilter pointer to this object iff it is a Matrix
  // type of ColorFilter, otherwise return nullptr.
  virtual const MatrixColorFilter* asMatrix() const { return nullptr; }

  // asSrgb<->Linear and asUnknown are not needed because they
  // have no properties to query. Their type fully specifies their
  // operation or can be accessed via the common gr_object() method.
};

// The Blend type of ColorFilter which specifies modifying the
// colors as if the color specified in the Blend filter is the
// source color and the color drawn by the rendering operation
// is the destination color. The mode parameter of the Blend
// filter is then used to combine those colors.
class BlendColorFilter final : public ColorFilter {
 public:
  BlendColorFilter(Color color, BlendMode mode) : color_(color), mode_(mode) {}
  BlendColorFilter(const BlendColorFilter& filter)
      : BlendColorFilter(filter.color_, filter.mode_) {}
  explicit BlendColorFilter(const BlendColorFilter* filter)
      : BlendColorFilter(filter->color_, filter->mode_) {}

  ColorFilterType type() const override { return ColorFilterType::kBlend; }
  size_t size() const override { return sizeof(*this); }
  bool modifies_transparent_black() const override {
    // Look at blend and color to make a faster determination?
    auto filter = gr_object();
    return filter && COLOR_FILTER_FILTER_COLOR(filter, Color::kTransparent()) !=
                         ToSk(Color::kTransparent());
  }

  std::shared_ptr<ColorFilter> shared() const override {
    return std::make_shared<BlendColorFilter>(this);
  }

  GrColorFilterPtr gr_object() const override {
    return GrColorFilters::Blend(color_, ToSk(mode_));
  }

  const BlendColorFilter* asBlend() const override { return this; }

  Color color() const { return color_; }
  BlendMode mode() const { return mode_; }

 protected:
  bool equals_(ColorFilter const& other) const override {
    FML_DCHECK(other.type() == ColorFilterType::kBlend);
    auto that = static_cast<BlendColorFilter const*>(&other);
    return color_ == that->color_ && mode_ == that->mode_;
  }

 private:
  Color color_;
  BlendMode mode_;
};

// The Matrix type of ColorFilter which runs every pixel drawn by
// the rendering operation [iR,iG,iB,iA] through a vector/matrix
// multiplication, as in:
//
//  [ oR ]   [ m[ 0] m[ 1] m[ 2] m[ 3] m[ 4] ]   [ iR ]
//  [ oG ]   [ m[ 5] m[ 6] m[ 7] m[ 8] m[ 9] ]   [ iG ]
//  [ oB ] = [ m[10] m[11] m[12] m[13] m[14] ] x [ iB ]
//  [ oA ]   [ m[15] m[16] m[17] m[18] m[19] ]   [ iA ]
//                                               [  1 ]
//
// The resulting color [oR,oG,oB,oA] is then clamped to the range of
// valid pixel components before storing in the output.
class MatrixColorFilter final : public ColorFilter {
 public:
  explicit MatrixColorFilter(const float matrix[20]) {
    memcpy(matrix_, matrix, sizeof(matrix_));
  }
  MatrixColorFilter(const MatrixColorFilter& filter)
      : MatrixColorFilter(filter.matrix_) {}
  explicit MatrixColorFilter(const MatrixColorFilter* filter)
      : MatrixColorFilter(filter->matrix_) {}

  ColorFilterType type() const override { return ColorFilterType::kMatrix; }
  size_t size() const override { return sizeof(*this); }
  bool modifies_transparent_black() const override {
    // Look at the matrix to make a faster determination?
    // Basically, are the translation components all 0?
    auto filter = gr_object();
    return filter && COLOR_FILTER_FILTER_COLOR(filter, Color::kTransparent()) !=
                         ToSk(Color::kTransparent());
  }

  bool can_commute_with_opacity() const override {
    return matrix_[3] == 0 && matrix_[8] == 0 && matrix_[13] == 0 &&
           matrix_[15] == 0 && matrix_[16] == 0 && matrix_[17] == 0 &&
           (matrix_[18] >= 0.0 && matrix_[18] <= 1.0) && matrix_[19] == 0;
  }

  std::shared_ptr<ColorFilter> shared() const override {
    return std::make_shared<MatrixColorFilter>(this);
  }

  GrColorFilterPtr gr_object() const override {
    return GrColorFilters::Matrix(matrix_);
  }

  const MatrixColorFilter* asMatrix() const override { return this; }

  const float& operator[](int index) const { return matrix_[index]; }
  void get_matrix(float matrix[20]) const {
    memcpy(matrix, matrix_, sizeof(matrix_));
  }

 protected:
  bool equals_(const ColorFilter& other) const override {
    FML_DCHECK(other.type() == ColorFilterType::kMatrix);
    auto that = static_cast<MatrixColorFilter const*>(&other);
    return memcmp(matrix_, that->matrix_, sizeof(matrix_)) == 0;
  }

 private:
  float matrix_[20];
};

// The SrgbToLinear type of ColorFilter that applies the inverse of the sRGB
// gamma curve to the rendered pixels.
class SrgbToLinearGammaColorFilter final : public ColorFilter {
 public:
  static const std::shared_ptr<SrgbToLinearGammaColorFilter> instance;

  SrgbToLinearGammaColorFilter() = default;
  SrgbToLinearGammaColorFilter(const SrgbToLinearGammaColorFilter& filter)
      : SrgbToLinearGammaColorFilter() {}
  explicit SrgbToLinearGammaColorFilter(
      const SrgbToLinearGammaColorFilter* filter)
      : SrgbToLinearGammaColorFilter() {}

  ColorFilterType type() const override {
    return ColorFilterType::kSrgbToLinearGamma;
  }
  size_t size() const override { return sizeof(*this); }
  bool modifies_transparent_black() const override { return false; }
  bool can_commute_with_opacity() const override { return true; }

  std::shared_ptr<ColorFilter> shared() const override { return instance; }

  GrColorFilterPtr gr_object() const override { return sk_filter_; }

 protected:
  bool equals_(const ColorFilter& other) const override {
    FML_DCHECK(other.type() == ColorFilterType::kSrgbToLinearGamma);
    return true;
  }

 private:
  static const GrColorFilterPtr sk_filter_;
  friend class ColorFilter;
};

// The LinearToSrgb type of ColorFilter that applies the sRGB gamma curve
// to the rendered pixels.
class LinearToSrgbGammaColorFilter final : public ColorFilter {
 public:
  static const std::shared_ptr<LinearToSrgbGammaColorFilter> instance;

  LinearToSrgbGammaColorFilter() {}
  LinearToSrgbGammaColorFilter(const LinearToSrgbGammaColorFilter& filter)
      : LinearToSrgbGammaColorFilter() {}
  explicit LinearToSrgbGammaColorFilter(
      const LinearToSrgbGammaColorFilter* filter)
      : LinearToSrgbGammaColorFilter() {}

  ColorFilterType type() const override {
    return ColorFilterType::kLinearToSrgbGamma;
  }
  size_t size() const override { return sizeof(*this); }
  bool modifies_transparent_black() const override { return false; }
  bool can_commute_with_opacity() const override { return true; }

  std::shared_ptr<ColorFilter> shared() const override { return instance; }

  GrColorFilterPtr gr_object() const override { return sk_filter_; }

 protected:
  bool equals_(const ColorFilter& other) const override {
    FML_DCHECK(other.type() == ColorFilterType::kLinearToSrgbGamma);
    return true;
  }

 private:
  static const GrColorFilterPtr sk_filter_;
  friend class ColorFilter;
};

class UnknownColorFilter final : public ColorFilter {
 public:
  explicit UnknownColorFilter(GrColorFilterPtr sk_filter)
      : sk_filter_(std::move(sk_filter)) {}
  explicit UnknownColorFilter(const UnknownColorFilter& filter)
      : UnknownColorFilter(filter.sk_filter_) {}
  explicit UnknownColorFilter(const UnknownColorFilter* filter)
      : UnknownColorFilter(filter->sk_filter_) {}

  ColorFilterType type() const override { return ColorFilterType::kUnknown; }
  size_t size() const override { return sizeof(*this); }
  bool modifies_transparent_black() const override {
    return sk_filter_ &&
           COLOR_FILTER_FILTER_COLOR(sk_filter_, Color::kTransparent()) !=
               ToSk(Color::kTransparent());
  }

  std::shared_ptr<ColorFilter> shared() const override {
    return std::make_shared<UnknownColorFilter>(this);
  }

  GrColorFilterPtr gr_object() const override { return sk_filter_; }

  virtual ~UnknownColorFilter() = default;

 protected:
  bool equals_(const ColorFilter& other) const override {
    FML_DCHECK(other.type() == ColorFilterType::kUnknown);
    auto that = static_cast<UnknownColorFilter const*>(&other);
    return sk_filter_ == that->sk_filter_;
  }

 private:
  GrColorFilterPtr sk_filter_;
};

}  // namespace clay

#endif  // CLAY_GFX_STYLE_COLOR_FILTER_H_
