// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_STYLE_IMAGE_FILTER_H_
#define CLAY_GFX_STYLE_IMAGE_FILTER_H_

#include <algorithm>
#include <memory>
#include <utility>

#include "clay/gfx/attributes.h"
#include "clay/gfx/comparable.h"
#include "clay/gfx/rendering_backend.h"
#ifndef ENABLE_SKITY
#include "clay/gfx/skity_to_skia_utils.h"
#endif
#include "clay/gfx/style/color_filter.h"
#include "clay/gfx/style/sampling_options.h"
#include "clay/gfx/style/tile_mode.h"
#include "skity/geometry/matrix.hpp"
#include "skity/geometry/rect.hpp"

namespace clay {
// The DisplayList ImageFilter class. This class implements all of the
// facilities and adheres to the design goals of the |Attribute| base
// class.
//
// The objects here define operations that can take a location and one or
// more input pixels and produce a color for that output pixel

// An enumerated type for the recognized ImageFilter operations.
// If a custom ImageFilter outside of the recognized types is needed
// then a |kUnknown| type that simply defers to an SkImageFilter is
// provided as a fallback.
enum class ImageFilterType {
  kBlur,
  kDilate,
  kErode,
  kMatrix,
  kComposeFilter,
  kColorFilter,
  kLocalMatrixFilter,
  kDropShadow,
  kUnknown
};

class BlurImageFilter;
class DropShadowImageFilter;
class DilateImageFilter;
class ErodeImageFilter;
class MatrixImageFilter;
class LocalMatrixImageFilter;
class ComposeImageFilter;
class ColorFilterImageFilter;
class ImageImageFilter;

class ImageFilter
    : public Attribute<ImageFilter, GrImageFilter, ImageFilterType> {
 public:
  enum class MatrixCapability {
    kTranslate,
    kScaleTranslate,
    kComplex,
  };

  static std::shared_ptr<ImageFilter> MakeBlur(float sigma_x, float sigma_y,
                                               TileMode tile_mode);
  static std::shared_ptr<ImageFilter> MakeDilate(float radius_x,
                                                 float radius_y);
  static std::shared_ptr<ImageFilter> MakeErode(float radius_x, float radius_y);
  static std::shared_ptr<ImageFilter> MakeMatrix(const skity::Matrix& matrix,
                                                 ImageSampling sampling);
  static std::shared_ptr<ImageFilter> MakeColorFilter(
      const std::shared_ptr<ColorFilter>& filter);
  static std::shared_ptr<ImageFilter> MakeCompose(
      const std::shared_ptr<ImageFilter>& outer,
      const std::shared_ptr<ImageFilter>& inner);

  // Return a BlurImageFilter pointer to this object iff it is a Blur
  // type of ImageFilter, otherwise return nullptr.
  virtual const BlurImageFilter* asBlur() const { return nullptr; }

  // Return a DropShadowFilter pointer to this object iff it is a DropShadow
  // type of ImageFilter, otherwise return nullptr.
  virtual const DropShadowImageFilter* asDropShadow() const { return nullptr; }

  // Return a DilateImageFilter pointer to this object iff it is a Dilate
  // type of ImageFilter, otherwise return nullptr.
  virtual const DilateImageFilter* asDilate() const { return nullptr; }

  // Return a ErodeImageFilter pointer to this object iff it is an Erode
  // type of ImageFilter, otherwise return nullptr.
  virtual const ErodeImageFilter* asErode() const { return nullptr; }

  // Return a MatrixImageFilter pointer to this object iff it is a Matrix
  // type of ImageFilter, otherwise return nullptr.
  virtual const MatrixImageFilter* asMatrix() const { return nullptr; }

  virtual const LocalMatrixImageFilter* asLocalMatrix() const {
    return nullptr;
  }

  virtual std::shared_ptr<ImageFilter> makeWithLocalMatrix(
      const skity::Matrix& matrix) const;

  // Return a ComposeImageFilter pointer to this object iff it is a Compose
  // type of ImageFilter, otherwise return nullptr.
  virtual const ComposeImageFilter* asCompose() const { return nullptr; }

  // Return a ColorFilterImageFilter pointer to this object iff it is a
  // ColorFilter type of ImageFilter, otherwise return nullptr.
  virtual const ColorFilterImageFilter* asColorFilter() const {
    return nullptr;
  }

  virtual const ImageImageFilter* asImage() const { return nullptr; }

  // Return a boolean indicating whether the image filtering operation will
  // modify transparent black. This is typically used to determine if applying
  // the ImageFilter to a temporary saveLayer buffer will turn the surrounding
  // pixels non-transparent and therefore expand the bounds.
  virtual bool modifies_transparent_black() const = 0;

  // Return the bounds of the output for this image filtering operation
  // based on the supplied input bounds where both are measured in the local
  // (untransformed) coordinate space.
  //
  // The method will return a pointer to the output_bounds parameter if it
  // can successfully compute the output bounds of the filter, otherwise the
  // method will return a nullptr and the output_bounds will be filled with
  // a best guess for the answer, even if just a copy of the input_bounds.
  virtual skity::Rect* map_local_bounds(const skity::Rect& input_bounds,
                                        skity::Rect& output_bounds) const = 0;

  // Return the device bounds of the output for this image filtering operation
  // based on the supplied input device bounds where both are measured in the
  // pixel coordinate space and relative to the given rendering ctm. The
  // transform matrix is used to adjust the filter parameters for when it
  // is used in a rendering operation (for example, the blur radius of a
  // Blur filter will expand based on the ctm).
  //
  // The method will return a pointer to the output_bounds parameter if it
  // can successfully compute the output bounds of the filter, otherwise the
  // method will return a nullptr and the output_bounds will be filled with
  // a best guess for the answer, even if just a copy of the input_bounds.
  virtual skity::Rect* map_device_bounds(const skity::Rect& input_bounds,
                                         const skity::Matrix& ctm,
                                         skity::Rect& output_bounds) const = 0;

  // Return the input bounds that will be needed in order for the filter to
  // properly fill the indicated output_bounds under the specified
  // transformation matrix. Both output_bounds and input_bounds are taken to
  // be relative to the transformed coordinate space of the provided |ctm|.
  //
  // The method will return a pointer to the input_bounds parameter if it
  // can successfully compute the required input bounds, otherwise the
  // method will return a nullptr and the input_bounds will be filled with
  // a best guess for the answer, even if just a copy of the output_bounds.
  virtual skity::Rect* get_input_device_bounds(
      const skity::Rect& output_bounds, const skity::Matrix& ctm,
      skity::Rect& input_bounds) const = 0;

  virtual MatrixCapability matrix_capability() const {
    return MatrixCapability::kScaleTranslate;
  }

 protected:
  static skity::Vec2 map_vectors_affine(const skity::Matrix& ctm, float x,
                                        float y) {
    FML_DCHECK(x >= 0);
    FML_DCHECK(y >= 0);
    FML_DCHECK(ctm.IsFinite() && !ctm.HasPersp());

    // The x and y scalars would have been used to expand a local space
    // rectangle which is then transformed by ctm. In order to do the
    // expansion correctly, we should look at the relevant math. The
    // 4 corners will be moved outward by the following vectors:
    //     (UL,UR,LR,LL) = ((-x, -y), (+x, -y), (+x, +y), (-x, +y))
    // After applying the transform, each of these vectors could be
    // pointing in any direction so we need to examine each transformed
    // delta vector and how it affected the bounds.
    // Looking at just the affine 2x3 entries of the CTM we can delta
    // transform these corner offsets and get the following:
    //     UL = dCTM(-x, -y) = (- x*m00 - y*m01, - x*m10 - y*m11)
    //     UR = dCTM(+x, -y) = (  x*m00 - y*m01,   x*m10 - y*m11)
    //     LR = dCTM(+x, +y) = (  x*m00 + y*m01,   x*m10 + y*m11)
    //     LL = dCTM(-x, +y) = (- x*m00 + y*m01, - x*m10 + y*m11)
    // The X vectors are all some variation of adding or subtracting
    // the sum of x*m00 and y*m01 or their difference. Similarly the Y
    // vectors are +/- the associated sum/difference of x*m10 and y*m11.
    // The largest displacements, both left/right or up/down, will
    // happen when the signs of the m00/m01/m10/m11 matrix entries
    // coincide with the signs of the scalars, i.e. are all positive.
    return {x * abs(ctm[0][0]) + y * abs(ctm[1][0]),
            x * abs(ctm[0][1]) + y * abs(ctm[1][1])};
  }

  static skity::Rect* inset_device_bounds(const skity::Rect& input_bounds,
                                          float radius_x, float radius_y,
                                          const skity::Matrix& ctm,
                                          skity::Rect& output_bounds) {
    if (ctm.IsFinite()) {
      if (ctm.HasPersp()) {
        skity::Matrix inverse;
        if (ctm.Invert(&inverse)) {
          skity::Rect local_bounds;
          inverse.MapRect(&local_bounds, input_bounds);
          local_bounds.Inset(radius_x, radius_y);
          ctm.MapRect(&output_bounds, local_bounds);
          output_bounds.RoundOut();
          return &output_bounds;
        }
      } else {
        skity::Vec2 device_radius = map_vectors_affine(ctm, radius_x, radius_y);
        output_bounds = input_bounds;
        output_bounds.Inset(floor(device_radius.x),  //
                            floor(device_radius.y));
        return &output_bounds;
      }
    }
    output_bounds = input_bounds;
    return nullptr;
  }
  static skity::Rect* outset_device_bounds(const skity::Rect& input_bounds,
                                           float radius_x, float radius_y,
                                           const skity::Matrix& ctm,
                                           skity::Rect& output_bounds) {
    if (ctm.IsFinite()) {
      if (ctm.HasPersp()) {
        skity::Matrix inverse;
        if (ctm.Invert(&inverse)) {
          skity::Rect local_bounds;
          inverse.MapRect(&local_bounds, input_bounds);
          local_bounds.Outset(radius_x, radius_y);
          ctm.MapRect(&output_bounds, local_bounds);
          output_bounds.RoundOut();
          return &output_bounds;
        }
      } else {
        skity::Vec2 device_radius = map_vectors_affine(ctm, radius_x, radius_y);
        output_bounds = input_bounds;
        output_bounds.Outset(ceil(device_radius.x),  //
                             ceil(device_radius.y));
        return &output_bounds;
      }
    }
    output_bounds = input_bounds;
    return nullptr;
  }

  static skity::Rect* outset_device_bounds(const skity::Rect& input_bounds,
                                           float dx, float dy, float radius_x,
                                           float radius_y,
                                           const skity::Matrix& ctm,
                                           skity::Rect& output_bounds) {
    if (ctm.IsFinite()) {
      if (ctm.HasPersp()) {
        skity::Matrix inverse;
        if (ctm.Invert(&inverse)) {
          skity::Rect local_bounds;
          inverse.MapRect(&local_bounds, input_bounds);
          skity::Rect shadow_bounds = local_bounds;
          shadow_bounds.Offset(dx, dy);
          shadow_bounds.Outset(radius_x, radius_y);
          local_bounds.Join(shadow_bounds);
          ctm.MapRect(&output_bounds, local_bounds);
          output_bounds.RoundOut();
          return &output_bounds;
        }
      } else {
        skity::Vec2 device_offsets = map_vectors_affine(ctm, dx, dy);
        skity::Vec2 device_radius = map_vectors_affine(ctm, radius_x, radius_y);
        skity::Rect shadow_bounds = input_bounds;
        shadow_bounds.Offset(ceil(device_offsets.x),  //
                             ceil(device_offsets.y));
        shadow_bounds.Offset(ceil(device_radius.x),  //
                             ceil(device_radius.y));
        shadow_bounds.Join(input_bounds);
        output_bounds = shadow_bounds;
        return &output_bounds;
      }
    }
    output_bounds = input_bounds;
    return nullptr;
  }
};

class BlurImageFilter final : public ImageFilter {
 public:
  BlurImageFilter(float sigma_x, float sigma_y, TileMode tile_mode)
      : sigma_x_(sigma_x), sigma_y_(sigma_y), tile_mode_(tile_mode) {}
  explicit BlurImageFilter(const BlurImageFilter* filter)
      : BlurImageFilter(filter->sigma_x_, filter->sigma_y_,
                        filter->tile_mode_) {}
  BlurImageFilter(const BlurImageFilter& filter) : BlurImageFilter(&filter) {}

  std::shared_ptr<ImageFilter> shared() const override {
    return std::make_shared<BlurImageFilter>(this);
  }

  ImageFilterType type() const override { return ImageFilterType::kBlur; }
  size_t size() const override { return sizeof(*this); }

  const BlurImageFilter* asBlur() const override { return this; }

  bool modifies_transparent_black() const override { return false; }

  skity::Rect* map_local_bounds(const skity::Rect& input_bounds,
                                skity::Rect& output_bounds) const override {
    output_bounds = input_bounds;
    output_bounds.Outset(sigma_x_ * 3, sigma_y_ * 3);
    return &output_bounds;
  }

  skity::Rect* map_device_bounds(const skity::Rect& input_bounds,
                                 const skity::Matrix& ctm,
                                 skity::Rect& output_bounds) const override {
    return outset_device_bounds(input_bounds, sigma_x_ * 3.0, sigma_y_ * 3.0,
                                ctm, output_bounds);
  }

  skity::Rect* get_input_device_bounds(
      const skity::Rect& output_bounds, const skity::Matrix& ctm,
      skity::Rect& input_bounds) const override {
    // Blurs are symmetric in terms of output-for-input and input-for-output
    return map_device_bounds(output_bounds, ctm, input_bounds);
  }

  float sigma_x() const { return sigma_x_; }
  float sigma_y() const { return sigma_y_; }
  TileMode tile_mode() const { return tile_mode_; }

  GrImageFilterPtr gr_object() const override {
    return IMAGE_FILTERS_BLUR(sigma_x_, sigma_y_, ToSk(tile_mode_), nullptr);
  }

  bool equals_(const ImageFilter& other) const override {
    FML_DCHECK(other.type() == ImageFilterType::kBlur);
    auto that = static_cast<const BlurImageFilter*>(&other);
    return (sigma_x_ == that->sigma_x_ && sigma_y_ == that->sigma_y_ &&
            tile_mode_ == that->tile_mode_);
  }

 private:
  float sigma_x_;
  float sigma_y_;
  TileMode tile_mode_;
};

class DropShadowImageFilter final : public ImageFilter {
 public:
  DropShadowImageFilter(float dx, float dy, float sigma_x, float sigma_y,
                        Color color)
      : dx_(dx), dy_(dy), sigma_x_(sigma_x), sigma_y_(sigma_y), color_(color) {}
  explicit DropShadowImageFilter(const DropShadowImageFilter* filter)
      : DropShadowImageFilter(filter->dx_, filter->dy_, filter->sigma_x_,
                              filter->sigma_y_, filter->color_) {}
  DropShadowImageFilter(const DropShadowImageFilter& filter)
      : DropShadowImageFilter(&filter) {}

  std::shared_ptr<ImageFilter> shared() const override {
    return std::make_shared<DropShadowImageFilter>(this);
  }

  ImageFilterType type() const override { return ImageFilterType::kDropShadow; }
  size_t size() const override { return sizeof(*this); }

  const DropShadowImageFilter* asDropShadow() const override { return this; }

  bool modifies_transparent_black() const override { return false; }

  skity::Rect* map_local_bounds(const skity::Rect& input_bounds,
                                skity::Rect& output_bounds) const override {
    skity::Rect shadow_bounds = input_bounds;
    shadow_bounds.Offset(dx_, dy_);
    shadow_bounds.Outset(sigma_x_ * 3.0, sigma_x_ * 3.0);
    shadow_bounds.Join(input_bounds);
    output_bounds = shadow_bounds;
    return &output_bounds;
  }

  skity::Rect* map_device_bounds(const skity::Rect& input_bounds,
                                 const skity::Matrix& ctm,
                                 skity::Rect& output_bounds) const override {
    return outset_device_bounds(input_bounds, dx_, dy_, sigma_x_ * 3.0,
                                sigma_y_ * 3.0, ctm, output_bounds);
  }

  skity::Rect* get_input_device_bounds(
      const skity::Rect& output_bounds, const skity::Matrix& ctm,
      skity::Rect& input_bounds) const override {
    // TODO(zhangzhijian): Figure out how to calculate input bounds from output
    // bounds
    input_bounds = output_bounds;
    return nullptr;
  }
  float dx() const { return dx_; }
  float dy() const { return dy_; }
  float sigma_x() const { return sigma_x_; }
  float sigma_y() const { return sigma_y_; }
  Color color() const { return color_; }

  GrImageFilterPtr gr_object() const override {
    return GrImageFilters::DropShadow(dx_, dy_, sigma_x_, sigma_y_, color_,
                                      nullptr);
  }

 protected:
  bool equals_(const ImageFilter& other) const override {
    FML_DCHECK(other.type() == ImageFilterType::kDropShadow);
    auto that = static_cast<const DropShadowImageFilter*>(&other);
    return (dx_ == that->dx_ && dy_ == that->dy_ &&
            sigma_x_ == that->sigma_x_ && sigma_y_ == that->sigma_y_ &&
            color_ == that->color_);
  }

 private:
  float dx_;
  float dy_;
  float sigma_x_;
  float sigma_y_;
  Color color_;
};

class DilateImageFilter final : public ImageFilter {
 public:
  DilateImageFilter(float radius_x, float radius_y)
      : radius_x_(radius_x), radius_y_(radius_y) {}
  explicit DilateImageFilter(const DilateImageFilter* filter)
      : DilateImageFilter(filter->radius_x_, filter->radius_y_) {}
  DilateImageFilter(const DilateImageFilter& filter)
      : DilateImageFilter(&filter) {}

  std::shared_ptr<ImageFilter> shared() const override {
    return std::make_shared<DilateImageFilter>(this);
  }

  ImageFilterType type() const override { return ImageFilterType::kDilate; }
  size_t size() const override { return sizeof(*this); }

  const DilateImageFilter* asDilate() const override { return this; }

  bool modifies_transparent_black() const override { return false; }

  skity::Rect* map_local_bounds(const skity::Rect& input_bounds,
                                skity::Rect& output_bounds) const override {
    output_bounds = input_bounds;
    output_bounds.Outset(radius_x_, radius_y_);
    return &output_bounds;
  }

  skity::Rect* map_device_bounds(const skity::Rect& input_bounds,
                                 const skity::Matrix& ctm,
                                 skity::Rect& output_bounds) const override {
    return outset_device_bounds(input_bounds, radius_x_, radius_y_, ctm,
                                output_bounds);
  }

  skity::Rect* get_input_device_bounds(
      const skity::Rect& output_bounds, const skity::Matrix& ctm,
      skity::Rect& input_bounds) const override {
    return inset_device_bounds(output_bounds, radius_x_, radius_y_, ctm,
                               input_bounds);
  }

  float radius_x() const { return radius_x_; }
  float radius_y() const { return radius_y_; }

  GrImageFilterPtr gr_object() const override {
    return IMAGE_FILTERS_DILATE(radius_x_, radius_y_, nullptr);
  }

 protected:
  bool equals_(const ImageFilter& other) const override {
    FML_DCHECK(other.type() == ImageFilterType::kDilate);
    auto that = static_cast<const DilateImageFilter*>(&other);
    return (radius_x_ == that->radius_x_ && radius_y_ == that->radius_y_);
  }

 private:
  float radius_x_;
  float radius_y_;
};

class ErodeImageFilter final : public ImageFilter {
 public:
  ErodeImageFilter(float radius_x, float radius_y)
      : radius_x_(radius_x), radius_y_(radius_y) {}
  explicit ErodeImageFilter(const ErodeImageFilter* filter)
      : ErodeImageFilter(filter->radius_x_, filter->radius_y_) {}
  ErodeImageFilter(const ErodeImageFilter& filter)
      : ErodeImageFilter(&filter) {}

  std::shared_ptr<ImageFilter> shared() const override {
    return std::make_shared<ErodeImageFilter>(this);
  }

  ImageFilterType type() const override { return ImageFilterType::kErode; }
  size_t size() const override { return sizeof(*this); }

  const ErodeImageFilter* asErode() const override { return this; }

  bool modifies_transparent_black() const override { return false; }

  skity::Rect* map_local_bounds(const skity::Rect& input_bounds,
                                skity::Rect& output_bounds) const override {
    output_bounds = input_bounds;
    output_bounds.Inset(radius_x_, radius_y_);
    return &output_bounds;
  }

  skity::Rect* map_device_bounds(const skity::Rect& input_bounds,
                                 const skity::Matrix& ctm,
                                 skity::Rect& output_bounds) const override {
    return inset_device_bounds(input_bounds, radius_x_, radius_y_, ctm,
                               output_bounds);
  }

  skity::Rect* get_input_device_bounds(
      const skity::Rect& output_bounds, const skity::Matrix& ctm,
      skity::Rect& input_bounds) const override {
    return outset_device_bounds(output_bounds, radius_x_, radius_y_, ctm,
                                input_bounds);
  }

  float radius_x() const { return radius_x_; }
  float radius_y() const { return radius_y_; }

  GrImageFilterPtr gr_object() const override {
    return IMAGE_FILTERS_ERODE(radius_x_, radius_y_, nullptr);
  }

 protected:
  bool equals_(const ImageFilter& other) const override {
    FML_DCHECK(other.type() == ImageFilterType::kErode);
    auto that = static_cast<const ErodeImageFilter*>(&other);
    return (radius_x_ == that->radius_x_ && radius_y_ == that->radius_y_);
  }

 private:
  float radius_x_;
  float radius_y_;
};

class MatrixImageFilter final : public ImageFilter {
 public:
  MatrixImageFilter(const skity::Matrix& matrix, ImageSampling sampling)
      : matrix_(matrix), sampling_(sampling) {}
  explicit MatrixImageFilter(const MatrixImageFilter* filter)
      : MatrixImageFilter(filter->matrix_, filter->sampling_) {}
  MatrixImageFilter(const MatrixImageFilter& filter)
      : MatrixImageFilter(&filter) {}

  std::shared_ptr<ImageFilter> shared() const override {
    return std::make_shared<MatrixImageFilter>(this);
  }

  ImageFilterType type() const override { return ImageFilterType::kMatrix; }
  size_t size() const override { return sizeof(*this); }

  const skity::Matrix& matrix() const { return matrix_; }
  ImageSampling sampling() const { return sampling_; }

  const MatrixImageFilter* asMatrix() const override { return this; }

  bool modifies_transparent_black() const override { return false; }

  skity::Rect* map_local_bounds(const skity::Rect& input_bounds,
                                skity::Rect& output_bounds) const override {
    matrix_.MapRect(&output_bounds, input_bounds);
    return &output_bounds;
  }

  skity::Rect* map_device_bounds(const skity::Rect& input_bounds,
                                 const skity::Matrix& ctm,
                                 skity::Rect& output_bounds) const override {
    skity::Matrix matrix;
    if (!ctm.Invert(&matrix)) {
      output_bounds = input_bounds;
      return nullptr;
    }
    matrix.PostConcat(matrix_);
    matrix.PostConcat(ctm);
    skity::Rect device_rect;
    matrix.MapRect(&device_rect, input_bounds);
    output_bounds = device_rect;
    output_bounds.RoundOut();
    return &output_bounds;
  }

  skity::Rect* get_input_device_bounds(
      const skity::Rect& output_bounds, const skity::Matrix& ctm,
      skity::Rect& input_bounds) const override {
    skity::Matrix matrix = ctm * matrix_;
    skity::Matrix inverse;
    if (!matrix.Invert(&inverse)) {
      input_bounds = output_bounds;
      return nullptr;
    }
    inverse.PostConcat(ctm);
    skity::Rect bounds = output_bounds;
    inverse.MapRect(&bounds, bounds);
    input_bounds = bounds;
    input_bounds.RoundOut();
    return &input_bounds;
  }

  GrImageFilterPtr gr_object() const override {
    return IMAGE_FILTERS_MATRIX_TRANSFORM(matrix_, ToSk(sampling_), nullptr);
  }

 protected:
  bool equals_(const ImageFilter& other) const override {
    FML_DCHECK(other.type() == ImageFilterType::kMatrix);
    auto that = static_cast<const MatrixImageFilter*>(&other);
    return (matrix_ == that->matrix_ && sampling_ == that->sampling_);
  }

 private:
  skity::Matrix matrix_;
  ImageSampling sampling_;
};

class ComposeImageFilter final : public ImageFilter {
 public:
  ComposeImageFilter(std::shared_ptr<ImageFilter> outer,
                     std::shared_ptr<ImageFilter> inner)
      : outer_(std::move(outer)), inner_(std::move(inner)) {}
  ComposeImageFilter(const ImageFilter* outer, const ImageFilter* inner)
      : outer_(outer->shared()), inner_(inner->shared()) {}
  ComposeImageFilter(const ImageFilter& outer, const ImageFilter& inner)
      : ComposeImageFilter(&outer, &inner) {}
  explicit ComposeImageFilter(const ComposeImageFilter* filter)
      : ComposeImageFilter(filter->outer_, filter->inner_) {}
  ComposeImageFilter(const ComposeImageFilter& filter)
      : ComposeImageFilter(&filter) {}

  std::shared_ptr<ImageFilter> shared() const override {
    return std::make_shared<ComposeImageFilter>(this);
  }

  ImageFilterType type() const override {
    return ImageFilterType::kComposeFilter;
  }
  size_t size() const override { return sizeof(*this); }

  std::shared_ptr<ImageFilter> outer() const { return outer_; }
  std::shared_ptr<ImageFilter> inner() const { return inner_; }

  const ComposeImageFilter* asCompose() const override { return this; }

  bool modifies_transparent_black() const override {
    if (inner_ && inner_->modifies_transparent_black()) {
      return true;
    }
    if (outer_ && outer_->modifies_transparent_black()) {
      return true;
    }
    return false;
  }

  skity::Rect* map_local_bounds(const skity::Rect& input_bounds,
                                skity::Rect& output_bounds) const override;

  skity::Rect* map_device_bounds(const skity::Rect& input_bounds,
                                 const skity::Matrix& ctm,
                                 skity::Rect& output_bounds) const override;

  skity::Rect* get_input_device_bounds(
      const skity::Rect& output_bounds, const skity::Matrix& ctm,
      skity::Rect& input_bounds) const override;

  GrImageFilterPtr gr_object() const override {
    return GrImageFilters::Compose(outer_->gr_object(), inner_->gr_object());
  }

  MatrixCapability matrix_capability() const override {
    return std::min(outer_->matrix_capability(), inner_->matrix_capability());
  }

 protected:
  bool equals_(const ImageFilter& other) const override {
    FML_DCHECK(other.type() == ImageFilterType::kComposeFilter);
    auto that = static_cast<const ComposeImageFilter*>(&other);
    return (Equals(outer_, that->outer_) && Equals(inner_, that->inner_));
  }

 private:
  std::shared_ptr<ImageFilter> outer_;
  std::shared_ptr<ImageFilter> inner_;
};

class ColorFilterImageFilter final : public ImageFilter {
 public:
  explicit ColorFilterImageFilter(std::shared_ptr<ColorFilter> filter)
      : color_filter_(std::move(filter)) {}
  explicit ColorFilterImageFilter(const ColorFilter* filter)
      : color_filter_(filter->shared()) {}
  explicit ColorFilterImageFilter(const ColorFilter& filter)
      : color_filter_(filter.shared()) {}
  explicit ColorFilterImageFilter(const ColorFilterImageFilter* filter)
      : ColorFilterImageFilter(filter->color_filter_) {}
  ColorFilterImageFilter(const ColorFilterImageFilter& filter)
      : ColorFilterImageFilter(&filter) {}

  std::shared_ptr<ImageFilter> shared() const override {
    return std::make_shared<ColorFilterImageFilter>(color_filter_);
  }

  ImageFilterType type() const override {
    return ImageFilterType::kColorFilter;
  }
  size_t size() const override { return sizeof(*this); }

  const std::shared_ptr<ColorFilter> color_filter() const {
    return color_filter_;
  }

  const ColorFilterImageFilter* asColorFilter() const override { return this; }

  bool modifies_transparent_black() const override {
    if (color_filter_) {
      return color_filter_->modifies_transparent_black();
    }
    return false;
  }

  skity::Rect* map_local_bounds(const skity::Rect& input_bounds,
                                skity::Rect& output_bounds) const override {
    output_bounds = input_bounds;
    return modifies_transparent_black() ? nullptr : &output_bounds;
  }

  skity::Rect* map_device_bounds(const skity::Rect& input_bounds,
                                 const skity::Matrix& ctm,
                                 skity::Rect& output_bounds) const override {
    output_bounds = input_bounds;
    return modifies_transparent_black() ? nullptr : &output_bounds;
  }

  skity::Rect* get_input_device_bounds(
      const skity::Rect& output_bounds, const skity::Matrix& ctm,
      skity::Rect& input_bounds) const override {
    return map_device_bounds(output_bounds, ctm, input_bounds);
  }

  GrImageFilterPtr gr_object() const override {
    return IMAGE_FILTERS_COLOR_FILTER(color_filter_->gr_object(), nullptr);
  }

  MatrixCapability matrix_capability() const override {
    return MatrixCapability::kComplex;
  }

  std::shared_ptr<ImageFilter> makeWithLocalMatrix(
      const skity::Matrix& matrix) const override {
    return shared();
  }

 protected:
  bool equals_(const ImageFilter& other) const override {
    FML_DCHECK(other.type() == ImageFilterType::kColorFilter);
    auto that = static_cast<const ColorFilterImageFilter*>(&other);
    return Equals(color_filter_, that->color_filter_);
  }

 private:
  std::shared_ptr<ColorFilter> color_filter_;
};

class LocalMatrixImageFilter final : public ImageFilter {
 public:
  explicit LocalMatrixImageFilter(const skity::Matrix& matrix,
                                  std::shared_ptr<ImageFilter> filter)
      : matrix_(matrix), image_filter_(filter) {}
  explicit LocalMatrixImageFilter(const LocalMatrixImageFilter* filter)
      : LocalMatrixImageFilter(filter->matrix_, filter->image_filter_) {}
  LocalMatrixImageFilter(const LocalMatrixImageFilter& filter)
      : LocalMatrixImageFilter(&filter) {}
  std::shared_ptr<ImageFilter> shared() const override {
    return std::make_shared<LocalMatrixImageFilter>(this);
  }

  ImageFilterType type() const override {
    return ImageFilterType::kLocalMatrixFilter;
  }
  size_t size() const override { return sizeof(*this); }

  const skity::Matrix& matrix() const { return matrix_; }

  const std::shared_ptr<ImageFilter> image_filter() const {
    return image_filter_;
  }

  const LocalMatrixImageFilter* asLocalMatrix() const override { return this; }

  bool modifies_transparent_black() const override {
    if (!image_filter_) {
      return false;
    }
    return image_filter_->modifies_transparent_black();
  }

  skity::Rect* map_local_bounds(const skity::Rect& input_bounds,
                                skity::Rect& output_bounds) const override {
    if (!image_filter_) {
      return nullptr;
    }
    return image_filter_->map_local_bounds(input_bounds, output_bounds);
  }

  skity::Rect* map_device_bounds(const skity::Rect& input_bounds,
                                 const skity::Matrix& ctm,
                                 skity::Rect& output_bounds) const override {
    if (!image_filter_) {
      return nullptr;
    }
    return image_filter_->map_device_bounds(input_bounds, ctm * matrix_,
                                            output_bounds);
  }

  skity::Rect* get_input_device_bounds(
      const skity::Rect& output_bounds, const skity::Matrix& ctm,
      skity::Rect& input_bounds) const override {
    if (!image_filter_) {
      return nullptr;
    }
    return image_filter_->get_input_device_bounds(output_bounds, ctm * matrix_,
                                                  input_bounds);
  }

  GrImageFilterPtr gr_object() const override {
#ifndef ENABLE_SKITY
    if (!image_filter_) {
      return nullptr;
    }
    sk_sp<SkImageFilter> gr_object = image_filter_->gr_object();
    if (!gr_object) {
      return nullptr;
    }
    return gr_object->makeWithLocalMatrix(
        ConvertSkityMatrixToSkMatrix(matrix_));

#else
    FML_UNIMPLEMENTED();
    return nullptr;
#endif  // ENABLE_SKITY
  }

 protected:
  bool equals_(const ImageFilter& other) const override {
    FML_DCHECK(other.type() == ImageFilterType::kMatrix);
    auto that = static_cast<const LocalMatrixImageFilter*>(&other);
    return (matrix_ == that->matrix_ &&
            Equals(image_filter_, that->image_filter_));
  }

 private:
  skity::Matrix matrix_;
  std::shared_ptr<ImageFilter> image_filter_;
};

class UnknownImageFilter final : public ImageFilter {
 public:
  explicit UnknownImageFilter(GrImageFilterPtr sk_filter)
      : sk_filter_(std::move(sk_filter)) {}
  explicit UnknownImageFilter(const GrImageFilter* sk_filter)
#ifndef ENABLE_SKITY
      : sk_filter_(sk_ref_sp(sk_filter)){}
#else
      : sk_filter_((const_cast<skity::ImageFilter*>(sk_filter))) {
  }
#endif  // ENABLE_SKITY
        explicit UnknownImageFilter(const UnknownImageFilter* filter)
      : UnknownImageFilter(filter->sk_filter_) {
  }
  UnknownImageFilter(const UnknownImageFilter& filter)
      : UnknownImageFilter(&filter) {}

  ImageFilterType type() const override { return ImageFilterType::kUnknown; }
  size_t size() const override { return sizeof(*this); }

  std::shared_ptr<ImageFilter> shared() const override {
    return std::make_shared<UnknownImageFilter>(this);
  }

  bool modifies_transparent_black() const override {
    if (!sk_filter_) {
      return false;
    }
#ifndef ENABLE_SKITY
    return !sk_filter_->canComputeFastBounds();
#else
    return false;
#endif  // ENABLE_SKITY
  }

  skity::Rect* map_local_bounds(const skity::Rect& input_bounds,
                                skity::Rect& output_bounds) const override {
    if (!sk_filter_ || modifies_transparent_black()) {
      output_bounds = input_bounds;
      return nullptr;
    }
#ifndef ENABLE_SKITY
    output_bounds =
        clay::ConvertSkRectToSkityRect(sk_filter_->computeFastBounds(
            clay::ConvertSkityRectToSkRect(input_bounds)));
#endif  // ENABLE_SKITY
    return &output_bounds;
  }

  skity::Rect* map_device_bounds(const skity::Rect& input_bounds,
                                 const skity::Matrix& ctm,
                                 skity::Rect& output_bounds) const override {
    if (!sk_filter_ || modifies_transparent_black()) {
      output_bounds = input_bounds;
      return nullptr;
    }
#ifndef ENABLE_SKITY
    output_bounds = ConvertSkIRectToSkityRect(
        sk_filter_->filterBounds(ConvertSkityRectToSkIRect(input_bounds),
                                 ConvertSkityMatrixToSkMatrix(ctm),
                                 SkImageFilter::kForward_MapDirection));
#endif  // ENABLE_SKITY
    return &output_bounds;
  }

  skity::Rect* get_input_device_bounds(
      const skity::Rect& output_bounds, const skity::Matrix& ctm,
      skity::Rect& input_bounds) const override {
    if (!sk_filter_ || modifies_transparent_black()) {
      input_bounds = output_bounds;
      return nullptr;
    }
#ifndef ENABLE_SKITY
    input_bounds = ConvertSkIRectToSkityRect(
        sk_filter_->filterBounds(ConvertSkityRectToSkIRect(output_bounds),
                                 ConvertSkityMatrixToSkMatrix(ctm),
                                 SkImageFilter::kReverse_MapDirection));
#endif  // ENABLE_SKITY
    return &input_bounds;
  }

  GrImageFilterPtr gr_object() const override { return sk_filter_; }

  virtual ~UnknownImageFilter() = default;

 protected:
  bool equals_(const ImageFilter& other) const override {
    FML_DCHECK(other.type() == ImageFilterType::kUnknown);
    auto that = static_cast<UnknownImageFilter const*>(&other);
    return sk_filter_ == that->sk_filter_;
  }

 private:
  GrImageFilterPtr sk_filter_;
};

}  // namespace clay

#endif  // CLAY_GFX_STYLE_IMAGE_FILTER_H_
