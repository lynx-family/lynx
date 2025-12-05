// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_STYLE_COLOR_SOURCE_H_
#define CLAY_GFX_STYLE_COLOR_SOURCE_H_

#include <memory>
#include <utility>
#include <vector>

#include "clay/fml/logging.h"
#include "clay/gfx/attributes.h"
#ifndef ENABLE_SKITY
#include "clay/gfx/gfx_rendering_backend.h"
#endif
#include <cstring>

#include "clay/gfx/paint_image.h"
#include "clay/gfx/rendering_backend.h"
#include "clay/gfx/style/color.h"
#include "clay/gfx/style/runtime_effect.h"
#include "clay/gfx/style/sampling_options.h"
#include "clay/gfx/style/tile_mode.h"
#include "skity/geometry/matrix.hpp"

namespace clay {

class ColorColorSource;
class ImageColorSource;
class LinearGradientColorSource;
class RadialGradientColorSource;
class ConicalGradientColorSource;
class SweepGradientColorSource;
class RuntimeEffectColorSource;
class UnknownColorSource;

// The DisplayList ColorSource class. This class implements all of the
// facilities and adheres to the design goals of the |DlAttribute| base
// class.
//
// The role of the DlColorSource is to provide color information for
// the pixels of a rendering operation. The object is essentially the
// origin of all color being rendered, though its output may be
// modified or transformed by geometric coverage data, the filter
// attributes, and the final blend with the pixels in the destination.

enum class ColorSourceType {
  kColor,
  kImage,
  kLinearGradient,
  kRadialGradient,
  kConicalGradient,
  kSweepGradient,
  kRuntimeEffect,
  kUnknown
};

class ColorSource : public Attribute<ColorSource, GrShader, ColorSourceType> {
 public:
  static std::shared_ptr<ColorSource> MakeLinear(
      const skity::Vec2 start_point, const skity::Vec2 end_point,
      uint32_t stop_count, const Color* colors, const float* stops,
      TileMode tile_mode, const skity::Matrix* matrix = nullptr);

  static std::shared_ptr<ColorSource> MakeRadial(
      skity::Vec2 center, float radius, uint32_t stop_count,
      const Color* colors, const float* stops, TileMode tile_mode,
      const skity::Matrix* matrix = nullptr);

  static std::shared_ptr<ColorSource> MakeConical(
      skity::Vec2 start_center, float start_radius, skity::Vec2 end_center,
      float end_radius, uint32_t stop_count, const Color* colors,
      const float* stops, TileMode tile_mode,
      const skity::Matrix* matrix = nullptr);

  static std::shared_ptr<ColorSource> MakeSweep(
      skity::Vec2 center, float start, float end, uint32_t stop_count,
      const Color* colors, const float* stops, TileMode tile_mode,
      const skity::Matrix* matrix = nullptr);

  static std::shared_ptr<RuntimeEffectColorSource> MakeRuntimeEffect(
      fml::RefPtr<RuntimeEffect> runtime_effect,
      std::vector<std::shared_ptr<ColorSource>> samplers,
      std::shared_ptr<std::vector<uint8_t>> uniform_data);

  static std::shared_ptr<ImageColorSource> MakeImage(
      fml::RefPtr<PaintImage> image, TileMode tile_mode_x, TileMode tile_mode_y,
      ImageSampling sampling, const skity::Matrix* matrix = nullptr);

  static std::shared_ptr<ColorColorSource> MakeColor(Color color);

  virtual bool is_opaque() const = 0;

  virtual std::shared_ptr<ColorSource> with_sampling(
      ImageSampling options) const {
    return shared();
  }

  // Return a DlColorColorSource pointer to this object iff it is an Color
  // type of ColorSource, otherwise return nullptr.
  virtual const ColorColorSource* asColor() const { return nullptr; }

  // Return a DlImageColorSource pointer to this object iff it is an Image
  // type of ColorSource, otherwise return nullptr.
  virtual const ImageColorSource* asImage() const { return nullptr; }

  // Return a DlLinearGradientColorSource pointer to this object iff it is a
  // Linear Gradient type of ColorSource, otherwise return nullptr.
  virtual const LinearGradientColorSource* asLinearGradient() const {
    return nullptr;
  }

  // Return a DlRadialGradientColorSource pointer to this object iff it is a
  // Radial Gradient type of ColorSource, otherwise return nullptr.
  virtual const RadialGradientColorSource* asRadialGradient() const {
    return nullptr;
  }

  // Return a DlConicalGradientColorSource pointer to this object iff it is a
  // Conical Gradient type of ColorSource, otherwise return nullptr.
  virtual const ConicalGradientColorSource* asConicalGradient() const {
    return nullptr;
  }

  // Return a DlSweepGradientColorSource pointer to this object iff it is a
  // Sweep Gradient type of ColorSource, otherwise return nullptr.
  virtual const SweepGradientColorSource* asSweepGradient() const {
    return nullptr;
  }

  virtual const RuntimeEffectColorSource* asRuntimeEffect() const {
    return nullptr;
  }

  // If this filter contains images, specifies the owning context for those
  // images.
  // Images with a DlImage::OwningContext::kRaster must only call gr_object
  // on the raster task runner.
  // A nullopt return means there is no image.
  virtual std::optional<PaintImage::OwningContext> owning_context() const {
    return std::nullopt;
  }

 protected:
  ColorSource() = default;

 private:
  BASE_DISALLOW_COPY_ASSIGN_AND_MOVE(ColorSource);
};

class ColorColorSource final : public ColorSource {
 public:
  ColorColorSource(Color color) : color_(color) {}

  std::shared_ptr<ColorSource> shared() const override {
    return std::make_shared<ColorColorSource>(color_);
  }

  const ColorColorSource* asColor() const override { return this; }

  ColorSourceType type() const override { return ColorSourceType::kColor; }
  size_t size() const override { return sizeof(*this); }

  bool is_opaque() const override { return (color_ >> 24) == 255; }

  Color color() const { return color_; }

  GrShaderPtr gr_object() const override {
#ifndef ENABLE_SKITY
    return SkShaders::Color(color_);
#else
    FML_UNIMPLEMENTED();
    return nullptr;
#endif  // ENABLE_SKITY
  }

 protected:
  bool equals_(ColorSource const& other) const override {
    FML_DCHECK(other.type() == ColorSourceType::kColor);
    auto that = static_cast<ColorColorSource const*>(&other);
    return color_ == that->color_;
  }

 private:
  Color color_;

  BASE_DISALLOW_COPY_ASSIGN_AND_MOVE(ColorColorSource);
};

class MatrixColorSourceBase : public ColorSource {
 public:
  const skity::Matrix& matrix() const { return matrix_; }
  const skity::Matrix* matrix_ptr() const {
    return matrix_.IsIdentity() ? nullptr : &matrix_;
  }

 protected:
  MatrixColorSourceBase(const skity::Matrix* matrix)
      : matrix_(matrix ? *matrix : skity::Matrix()) {}

 private:
  const skity::Matrix matrix_;
};

class ImageColorSource final
    : public fml::RefCountedThreadSafe<ImageColorSource>,
      public MatrixColorSourceBase {
 public:
  ImageColorSource(fml::RefPtr<const PaintImage> image,
                   TileMode horizontal_tile_mode, TileMode vertical_tile_mode,
                   ImageSampling sampling = ImageSampling::kLinear,
                   const skity::Matrix* matrix = nullptr)
      : MatrixColorSourceBase(matrix),
        image_(image),
        horizontal_tile_mode_(horizontal_tile_mode),
        vertical_tile_mode_(vertical_tile_mode),
        sampling_(sampling) {}

  const ImageColorSource* asImage() const override { return this; }

  std::shared_ptr<ColorSource> shared() const override {
    return with_sampling(sampling_);
  }

  std::shared_ptr<ColorSource> with_sampling(
      ImageSampling sampling) const override {
    return std::make_shared<ImageColorSource>(image_, horizontal_tile_mode_,
                                              vertical_tile_mode_, sampling,
                                              matrix_ptr());
  }

  ColorSourceType type() const override { return ColorSourceType::kImage; }
  size_t size() const override { return sizeof(*this); }

  bool is_opaque() const override { return image_->isOpaque(); }

  std::optional<PaintImage::OwningContext> owning_context() const override {
    return image_->owning_context();
  }

  fml::RefPtr<const PaintImage> image() const { return image_; }
  TileMode horizontal_tile_mode() const { return horizontal_tile_mode_; }
  TileMode vertical_tile_mode() const { return vertical_tile_mode_; }
  ImageSampling sampling() const { return sampling_; }

  GrShaderPtr gr_object() const override {
#ifndef ENABLE_SKITY
    if (!image_->gr_image()) {
      return nullptr;
    }
    auto sk_matrix = clay::ConvertSkityMatrixToSkMatrix(matrix());
    return image_->gr_image()->makeShader(ToSk(horizontal_tile_mode_),
                                          ToSk(vertical_tile_mode_),
                                          ToSk(sampling_), &sk_matrix);
#else
    std::shared_ptr<skity::Image> skity_image = image_->gr_image();
    if (skity_image) {
      auto matrix = matrix_ptr() ? *matrix_ptr() : skity::Matrix();
      GrSamplingOptions sampling_options;
      switch (sampling_) {
        case ImageSampling::kNearestNeighbor:
          sampling_options = GrSamplingOptions(skity::FilterMode::kNearest,
                                               skity::MipmapMode::kNone);
          break;
        case ImageSampling::kLinear:
          sampling_options = GrSamplingOptions(skity::FilterMode::kLinear,
                                               skity::MipmapMode::kNone);
          break;
        case ImageSampling::kMipmapLinear:
          sampling_options = GrSamplingOptions(skity::FilterMode::kLinear,
                                               skity::MipmapMode::kLinear);
          break;
        case ImageSampling::kCubic:
          // Info: skity::FilterMode::kCubic is not supported yet.
          sampling_options = GrSamplingOptions(skity::FilterMode::kNearest,
                                               skity::MipmapMode::kNone);
          break;
      }
      return skity::Shader::MakeShader(
          skity_image, sampling_options,
          static_cast<skity::TileMode>(horizontal_tile_mode_),
          static_cast<skity::TileMode>(vertical_tile_mode_), matrix);
    }
    return nullptr;
#endif  // ENABLE_SKITY
  }

 protected:
  bool equals_(ColorSource const& other) const override {
    FML_DCHECK(other.type() == ColorSourceType::kImage);
    auto that = static_cast<ImageColorSource const*>(&other);
    return (image_->Equals(that->image_) && matrix() == that->matrix() &&
            horizontal_tile_mode_ == that->horizontal_tile_mode_ &&
            vertical_tile_mode_ == that->vertical_tile_mode_ &&
            sampling_ == that->sampling_);
  }

 private:
  fml::RefPtr<const PaintImage> image_;
  TileMode horizontal_tile_mode_;
  TileMode vertical_tile_mode_;
  ImageSampling sampling_;

  friend class ColorSource;

  BASE_DISALLOW_COPY_ASSIGN_AND_MOVE(ImageColorSource);
};

class GradientColorSourceBase : public MatrixColorSourceBase {
 public:
  bool is_opaque() const override {
    if (mode_ == TileMode::kDecal) {
      return false;
    }
    const Color* my_colors = colors();
    for (uint32_t i = 0; i < stop_count_; i++) {
      if ((my_colors[i] >> 24) < 255) {
        return false;
      }
    }
    return true;
  }

  TileMode tile_mode() const { return mode_; }
  int stop_count() const { return stop_count_; }
  const Color* colors() const { return reinterpret_cast<const Color*>(pod()); }
#ifdef ENABLE_SKITY
  std::vector<skity::Vec4> colors_vec4() const {
    std::vector<skity::Vec4> skity_colors;
    skity_colors.reserve(stop_count_);
    const Color* dl_colors = colors();
    for (auto i = 0u; i < stop_count_; i++) {
      float r = dl_colors[i].RedF();
      float g = dl_colors[i].GreenF();
      float b = dl_colors[i].BlueF();
      float a = dl_colors[i].AlphaF();
      skity_colors.emplace_back(r, g, b, a);
    }
    return skity_colors;
  }
#endif  // ENABLE_SKITY
  const float* stops() const {
    return reinterpret_cast<const float*>(colors() + stop_count());
  }

 protected:
  GradientColorSourceBase(uint32_t stop_count, TileMode tile_mode,
                          const skity::Matrix* matrix = nullptr)
      : MatrixColorSourceBase(matrix),
        mode_(tile_mode),
        stop_count_(stop_count) {}

  size_t vector_sizes() const {
    return stop_count_ * (sizeof(Color) + sizeof(float));
  }

  virtual const void* pod() const = 0;

  bool base_equals_(GradientColorSourceBase const* other_base) const {
    if (mode_ != other_base->mode_ || matrix() != other_base->matrix() ||
        stop_count_ != other_base->stop_count_) {
      return false;
    }
    static_assert(sizeof(colors()[0]) == 4);
    static_assert(sizeof(stops()[0]) == 4);
    int num_bytes = stop_count_ * 4;
    return (memcmp(colors(), other_base->colors(), num_bytes) == 0 &&
            memcmp(stops(), other_base->stops(), num_bytes) == 0);
  }

  void store_color_stops(void* pod, const Color* color_data,
                         const float* stop_data) {
    Color* color_storage = reinterpret_cast<Color*>(pod);
    memcpy(color_storage, color_data, stop_count_ * sizeof(*color_data));
    float* stop_storage = reinterpret_cast<float*>(color_storage + stop_count_);
    if (stop_data) {
      memcpy(stop_storage, stop_data, stop_count_ * sizeof(*stop_data));
    } else {
      float div = stop_count_ - 1;
      if (div <= 0) {
        div = 1;
      }
      for (uint32_t i = 0; i < stop_count_; i++) {
        stop_storage[i] = i / div;
      }
    }
  }

 private:
  TileMode mode_;
  uint32_t stop_count_;

  friend class ColorSource;

  BASE_DISALLOW_COPY_ASSIGN_AND_MOVE(GradientColorSourceBase);
};

class LinearGradientColorSource final : public GradientColorSourceBase {
 public:
  const LinearGradientColorSource* asLinearGradient() const override {
    return this;
  }

  ColorSourceType type() const override {
    return ColorSourceType::kLinearGradient;
  }
  size_t size() const override { return sizeof(*this) + vector_sizes(); }

  std::shared_ptr<ColorSource> shared() const override {
    return MakeLinear(start_point_, end_point_, stop_count(), colors(), stops(),
                      tile_mode(), matrix_ptr());
  }

  const skity::Vec2& start_point() const { return start_point_; }
  const skity::Vec2& end_point() const { return end_point_; }

  GrShaderPtr gr_object() const override {
#ifndef ENABLE_SKITY
    SkPoint pts[] = {SkPoint::Make(start_point_.x, start_point_.y),
                     SkPoint::Make(end_point_.x, end_point_.y)};
    const SkColor* sk_colors = reinterpret_cast<const SkColor*>(colors());
    auto sk_matrix = clay::ConvertSkityMatrixToSkMatrix(matrix());
    return SkGradientShader::MakeLinear(pts, sk_colors, stops(), stop_count(),
                                        ToSk(tile_mode()), 0, &sk_matrix);
#else
    skity::Point start_point =
        skity::Point(start_point_.x, start_point_.y, 0.f, 1.f);
    skity::Point end_point = skity::Point(end_point_.x, end_point_.y, 0.f, 1.f);
    skity::Point pts[] = {start_point, end_point};
    std::vector<skity::Vec4> skity_colors = colors_vec4();
    auto shader = skity::Shader::MakeLinear(
        pts, skity_colors.data(), stops(), stop_count(),
        static_cast<skity::TileMode>(tile_mode()));
    skity::Matrix skity_matrix = matrix();
    shader->SetLocalMatrix(skity_matrix);
    return shader;
#endif  // ENABLE_SKITY
  }

 protected:
  virtual const void* pod() const override { return this + 1; }

  bool equals_(ColorSource const& other) const override {
    FML_DCHECK(other.type() == ColorSourceType::kLinearGradient);
    auto that = static_cast<LinearGradientColorSource const*>(&other);
    return (start_point_ == that->start_point_ &&
            end_point_ == that->end_point_ && base_equals_(that));
  }

 private:
  LinearGradientColorSource(const skity::Vec2 start_point,
                            const skity::Vec2 end_point, uint32_t stop_count,
                            const Color* colors, const float* stops,
                            TileMode tile_mode,
                            const skity::Matrix* matrix = nullptr)
      : GradientColorSourceBase(stop_count, tile_mode, matrix),
        start_point_(start_point),
        end_point_(end_point) {
    store_color_stops(this + 1, colors, stops);
  }

  LinearGradientColorSource(const LinearGradientColorSource* source)
      : GradientColorSourceBase(source->stop_count(), source->tile_mode(),
                                source->matrix_ptr()),
        start_point_(source->start_point()),
        end_point_(source->end_point()) {
    store_color_stops(this + 1, source->colors(), source->stops());
  }

  skity::Vec2 start_point_;
  skity::Vec2 end_point_;

  friend class ColorSource;

  BASE_DISALLOW_COPY_ASSIGN_AND_MOVE(LinearGradientColorSource);
};

class RadialGradientColorSource final : public GradientColorSourceBase {
 public:
  const RadialGradientColorSource* asRadialGradient() const override {
    return this;
  }

  std::shared_ptr<ColorSource> shared() const override {
    return MakeRadial(center_, radius_, stop_count(), colors(), stops(),
                      tile_mode(), matrix_ptr());
  }

  ColorSourceType type() const override {
    return ColorSourceType::kRadialGradient;
  }
  size_t size() const override { return sizeof(*this) + vector_sizes(); }

  skity::Vec2 center() const { return center_; }
  float radius() const { return radius_; }

  GrShaderPtr gr_object() const override {
#ifndef ENABLE_SKITY
    const SkColor* sk_colors = reinterpret_cast<const SkColor*>(colors());
    auto sk_matrix = clay::ConvertSkityMatrixToSkMatrix(matrix());
    return SkGradientShader::MakeRadial(
        SkPoint::Make(center_.x, center_.y), radius_, sk_colors, stops(),
        stop_count(), ToSk(tile_mode()), 0, &sk_matrix);
#else
    skity::Point center = skity::Point(center_.x, center_.y, 0.f, 1.f);
    std::vector<skity::Vec4> skity_colors = colors_vec4();
    auto shader = skity::Shader::MakeRadial(
        center, radius_, skity_colors.data(), stops(), stop_count(),
        static_cast<skity::TileMode>(tile_mode()));
    skity::Matrix skity_matrix = matrix();
    shader->SetLocalMatrix(skity_matrix);
    return shader;
#endif  // ENABLE_SKITY
  }

 protected:
  virtual const void* pod() const override { return this + 1; }

  bool equals_(ColorSource const& other) const override {
    FML_DCHECK(other.type() == ColorSourceType::kRadialGradient);
    auto that = static_cast<RadialGradientColorSource const*>(&other);
    return (center_ == that->center_ && radius_ == that->radius_ &&
            base_equals_(that));
  }

 private:
  RadialGradientColorSource(skity::Vec2 center, float radius,
                            uint32_t stop_count, const Color* colors,
                            const float* stops, TileMode tile_mode,
                            const skity::Matrix* matrix = nullptr)
      : GradientColorSourceBase(stop_count, tile_mode, matrix),
        center_(center),
        radius_(radius) {
    store_color_stops(this + 1, colors, stops);
  }

  RadialGradientColorSource(const RadialGradientColorSource* source)
      : GradientColorSourceBase(source->stop_count(), source->tile_mode(),
                                source->matrix_ptr()),
        center_(source->center()),
        radius_(source->radius()) {
    store_color_stops(this + 1, source->colors(), source->stops());
  }

  skity::Vec2 center_;
  float radius_;

  friend class ColorSource;

  BASE_DISALLOW_COPY_ASSIGN_AND_MOVE(RadialGradientColorSource);
};

class ConicalGradientColorSource final : public GradientColorSourceBase {
 public:
  const ConicalGradientColorSource* asConicalGradient() const override {
    return this;
  }

  std::shared_ptr<ColorSource> shared() const override {
    return MakeConical(start_center_, start_radius_, end_center_, end_radius_,
                       stop_count(), colors(), stops(), tile_mode(),
                       matrix_ptr());
  }

  ColorSourceType type() const override {
    return ColorSourceType::kConicalGradient;
  }
  size_t size() const override { return sizeof(*this) + vector_sizes(); }

  skity::Vec2 start_center() const { return start_center_; }
  float start_radius() const { return start_radius_; }
  skity::Vec2 end_center() const { return end_center_; }
  float end_radius() const { return end_radius_; }

  GrShaderPtr gr_object() const override {
#ifndef ENABLE_SKITY
    const SkColor* sk_colors = reinterpret_cast<const SkColor*>(colors());
    auto sk_matrix = clay::ConvertSkityMatrixToSkMatrix(matrix());
    return SkGradientShader::MakeTwoPointConical(
        SkPoint::Make(start_center_.x, start_center_.y), start_radius_,
        SkPoint::Make(end_center_.x, end_center_.y), end_radius_, sk_colors,
        stops(), stop_count(), ToSk(tile_mode()), 0, &sk_matrix);
#else
    skity::Point start =
        skity::Point(start_center_.x, start_center_.y, 0.f, 1.f);
    skity::Point end = skity::Point(end_center_.x, end_center_.y, 0.f, 1.f);
    std::vector<skity::Vec4> skity_colors = colors_vec4();
    auto shader = skity::Shader::MakeTwoPointConical(
        start, start_radius_, end, end_radius_, skity_colors.data(), stops(),
        stop_count(), static_cast<skity::TileMode>(tile_mode()));
    skity::Matrix skity_matrix = matrix();
    shader->SetLocalMatrix(skity_matrix);
    return shader;
#endif  // ENABLE_SKITY
  }

 protected:
  virtual const void* pod() const override { return this + 1; }

  bool equals_(ColorSource const& other) const override {
    FML_DCHECK(other.type() == ColorSourceType::kConicalGradient);
    auto that = static_cast<ConicalGradientColorSource const*>(&other);
    return (start_center_ == that->start_center_ &&
            start_radius_ == that->start_radius_ &&
            end_center_ == that->end_center_ &&
            end_radius_ == that->end_radius_ && base_equals_(that));
  }

 private:
  ConicalGradientColorSource(skity::Vec2 start_center, float start_radius,
                             skity::Vec2 end_center, float end_radius,
                             uint32_t stop_count, const Color* colors,
                             const float* stops, TileMode tile_mode,
                             const skity::Matrix* matrix = nullptr)
      : GradientColorSourceBase(stop_count, tile_mode, matrix),
        start_center_(start_center),
        start_radius_(start_radius),
        end_center_(end_center),
        end_radius_(end_radius) {
    store_color_stops(this + 1, colors, stops);
  }

  ConicalGradientColorSource(const ConicalGradientColorSource* source)
      : GradientColorSourceBase(source->stop_count(), source->tile_mode(),
                                source->matrix_ptr()),
        start_center_(source->start_center()),
        start_radius_(source->start_radius()),
        end_center_(source->end_center()),
        end_radius_(source->end_radius()) {
    store_color_stops(this + 1, source->colors(), source->stops());
  }

  skity::Vec2 start_center_;
  float start_radius_;
  skity::Vec2 end_center_;
  float end_radius_;

  friend class ColorSource;

  BASE_DISALLOW_COPY_ASSIGN_AND_MOVE(ConicalGradientColorSource);
};

class SweepGradientColorSource final : public GradientColorSourceBase {
 public:
  const SweepGradientColorSource* asSweepGradient() const override {
    return this;
  }

  std::shared_ptr<ColorSource> shared() const override {
    return MakeSweep(center_, start_, end_, stop_count(), colors(), stops(),
                     tile_mode(), matrix_ptr());
  }

  ColorSourceType type() const override {
    return ColorSourceType::kSweepGradient;
  }
  size_t size() const override { return sizeof(*this) + vector_sizes(); }

  skity::Vec2 center() const { return center_; }
  float start() const { return start_; }
  float end() const { return end_; }

  GrShaderPtr gr_object() const override {
#ifndef ENABLE_SKITY
    const SkColor* sk_colors = reinterpret_cast<const SkColor*>(colors());
    auto sk_matrix = clay::ConvertSkityMatrixToSkMatrix(matrix());
    return SkGradientShader::MakeSweep(center_.x, center_.y, sk_colors, stops(),
                                       stop_count(), ToSk(tile_mode()), start_,
                                       end_, 0, &sk_matrix);
#else
    std::vector<skity::Vec4> skity_colors = colors_vec4();
    auto shader =
        skity::Shader::MakeSweep(center_.x, center_.y, start_, end_,
                                 skity_colors.data(), stops(), stop_count());
    skity::Matrix skity_matrix = matrix();
    shader->SetLocalMatrix(skity_matrix);
    return shader;
#endif  // ENABLE_SKITY
  }

 protected:
  virtual const void* pod() const override { return this + 1; }

  bool equals_(ColorSource const& other) const override {
    FML_DCHECK(other.type() == ColorSourceType::kSweepGradient);
    auto that = static_cast<SweepGradientColorSource const*>(&other);
    return (center_ == that->center_ && start_ == that->start_ &&
            end_ == that->end_ && base_equals_(that));
  }

 private:
  SweepGradientColorSource(skity::Vec2 center, float start, float end,
                           uint32_t stop_count, const Color* colors,
                           const float* stops, TileMode tile_mode,
                           const skity::Matrix* matrix = nullptr)
      : GradientColorSourceBase(stop_count, tile_mode, matrix),
        center_(center),
        start_(start),
        end_(end) {
    store_color_stops(this + 1, colors, stops);
  }

  SweepGradientColorSource(const SweepGradientColorSource* source)
      : GradientColorSourceBase(source->stop_count(), source->tile_mode(),
                                source->matrix_ptr()),
        center_(source->center()),
        start_(source->start()),
        end_(source->end()) {
    store_color_stops(this + 1, source->colors(), source->stops());
  }

  skity::Vec2 center_;
  float start_;
  float end_;

  friend class ColorSource;

  BASE_DISALLOW_COPY_ASSIGN_AND_MOVE(SweepGradientColorSource);
};

class RuntimeEffectColorSource final : public ColorSource {
 public:
  RuntimeEffectColorSource(fml::RefPtr<RuntimeEffect> runtime_effect,
                           std::vector<std::shared_ptr<ColorSource>> samplers,
                           std::shared_ptr<std::vector<uint8_t>> uniform_data)
      : runtime_effect_(std::move(runtime_effect)),
        samplers_(std::move(samplers)),
        uniform_data_(std::move(uniform_data)) {}

  const RuntimeEffectColorSource* asRuntimeEffect() const override {
    return this;
  }

  std::shared_ptr<ColorSource> shared() const override {
    return std::make_shared<RuntimeEffectColorSource>(runtime_effect_,
                                                      samplers_, uniform_data_);
  }

  ColorSourceType type() const override {
    return ColorSourceType::kRuntimeEffect;
  }
  size_t size() const override { return sizeof(*this); }

  bool is_opaque() const override { return false; }

  const fml::RefPtr<RuntimeEffect> runtime_effect() const {
    return runtime_effect_;
  }
  const std::vector<std::shared_ptr<ColorSource>> samplers() const {
    return samplers_;
  }
  const std::shared_ptr<std::vector<uint8_t>> uniform_data() const {
    return uniform_data_;
  }

  GrShaderPtr gr_object() const override {
#ifndef ENABLE_SKITY
    if (!runtime_effect_) {
      return nullptr;
    }
    if (!runtime_effect_->skia_runtime_effect()) {
      return nullptr;
    }
    std::vector<sk_sp<SkShader>> sk_samplers(samplers_.size());
    for (size_t i = 0; i < samplers_.size(); i++) {
      auto sampler = samplers_[i];
      if (sampler == nullptr) {
        return nullptr;
      }
      sk_samplers[i] = sampler->gr_object();
    }

    auto ref = new std::shared_ptr<std::vector<uint8_t>>(uniform_data_);
    auto uniform_data = SkData::MakeWithProc(
        uniform_data_->data(), uniform_data_->size(),
        [](const void* ptr, void* context) {
          delete reinterpret_cast<std::shared_ptr<std::vector<uint8_t>>*>(
              context);
        },
        ref);

    return runtime_effect_->skia_runtime_effect()->makeShader(
        uniform_data, sk_samplers.data(), sk_samplers.size());
#else
    FML_UNIMPLEMENTED();
    return nullptr;
#endif  // ENABLE_SKITY
  }

 protected:
  bool equals_(ColorSource const& other) const override {
    FML_DCHECK(other.type() == ColorSourceType::kRuntimeEffect);
    auto that = static_cast<RuntimeEffectColorSource const*>(&other);
    if (runtime_effect_ != that->runtime_effect_) {
      return false;
    }
    if (uniform_data_ != that->uniform_data_) {
      return false;
    }
    if (samplers_.size() != that->samplers_.size()) {
      return false;
    }
    for (size_t i = 0; i < samplers_.size(); i++) {
      if (samplers_[i] != that->samplers_[i]) {
        return false;
      }
    }
    return true;
  }

 private:
  fml::RefPtr<RuntimeEffect> runtime_effect_;
  std::vector<std::shared_ptr<ColorSource>> samplers_;
  std::shared_ptr<std::vector<uint8_t>> uniform_data_;

  BASE_DISALLOW_COPY_ASSIGN_AND_MOVE(RuntimeEffectColorSource);
};

class UnknownColorSource final : public ColorSource {
 public:
  UnknownColorSource(GrShaderPtr shader) : sk_shader_(std::move(shader)) {}

  std::shared_ptr<ColorSource> shared() const override {
    return std::make_shared<UnknownColorSource>(sk_shader_);
  }

  ColorSourceType type() const override { return ColorSourceType::kUnknown; }
  size_t size() const override { return sizeof(*this); }

  bool is_opaque() const override { return SHADER_IS_OPAQUE(sk_shader_); }

  GrShaderPtr gr_object() const override { return sk_shader_; }

 protected:
  bool equals_(ColorSource const& other) const override {
    FML_DCHECK(other.type() == ColorSourceType::kUnknown);
    auto that = static_cast<UnknownColorSource const*>(&other);
    return (sk_shader_ == that->sk_shader_);
  }

 private:
  GrShaderPtr sk_shader_;

  BASE_DISALLOW_COPY_ASSIGN_AND_MOVE(UnknownColorSource);
};

}  // namespace clay

#endif  // CLAY_GFX_STYLE_COLOR_SOURCE_H_
