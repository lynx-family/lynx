// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/style/image_filter.h"

namespace clay {

std::shared_ptr<ImageFilter> ImageFilter::MakeBlur(float sigma_x, float sigma_y,
                                                   TileMode tile_mode) {
  return std::make_shared<BlurImageFilter>(sigma_x, sigma_y, tile_mode);
}
std::shared_ptr<ImageFilter> ImageFilter::MakeDilate(float radius_x,
                                                     float radius_y) {
  return std::make_shared<DilateImageFilter>(radius_x, radius_y);
}
std::shared_ptr<ImageFilter> ImageFilter::MakeErode(float radius_x,
                                                    float radius_y) {
  return std::make_shared<ErodeImageFilter>(radius_x, radius_y);
}
std::shared_ptr<ImageFilter> ImageFilter::MakeMatrix(
    const skity::Matrix& matrix, ImageSampling sampling) {
  return std::make_shared<MatrixImageFilter>(matrix, sampling);
}
std::shared_ptr<ImageFilter> ImageFilter::MakeColorFilter(
    const std::shared_ptr<ColorFilter>& filter) {
  return std::make_shared<ColorFilterImageFilter>(filter);
}
std::shared_ptr<ImageFilter> ImageFilter::MakeCompose(
    const std::shared_ptr<ImageFilter>& outer,
    const std::shared_ptr<ImageFilter>& inner) {
  return std::make_shared<ComposeImageFilter>(outer, inner);
}

std::shared_ptr<ImageFilter> ImageFilter::makeWithLocalMatrix(
    const skity::Matrix& matrix) const {
  if (matrix.IsIdentity()) {
    return shared();
  }
  // Matrix
  switch (this->matrix_capability()) {
    case MatrixCapability::kTranslate: {
      if (!matrix.OnlyTranslate()) {
        // Nothing we can do at this point
        return nullptr;
      }
      break;
    }
    case MatrixCapability::kScaleTranslate: {
      if (!matrix.OnlyScaleAndTranslate()) {
        // Nothing we can do at this point
        return nullptr;
      }
      break;
    }
    default:
      break;
  }
  return std::make_shared<LocalMatrixImageFilter>(matrix, shared());
}

skity::Rect* ComposeImageFilter::map_local_bounds(
    const skity::Rect& input_bounds, skity::Rect& output_bounds) const {
  skity::Rect cur_bounds = input_bounds;
  skity::Rect* ret = &output_bounds;
  // We set this result in case neither filter is present.
  output_bounds = input_bounds;
  if (inner_) {
    if (!inner_->map_local_bounds(cur_bounds, output_bounds)) {
      ret = nullptr;
    }
    cur_bounds = output_bounds;
  }
  if (outer_) {
    if (!outer_->map_local_bounds(cur_bounds, output_bounds)) {
      ret = nullptr;
    }
  }
  return ret;
}

skity::Rect* ComposeImageFilter::map_device_bounds(
    const skity::Rect& input_bounds, const skity::Matrix& ctm,
    skity::Rect& output_bounds) const {
  skity::Rect cur_bounds = input_bounds;
  skity::Rect* ret = &output_bounds;
  // We set this result in case neither filter is present.
  output_bounds = input_bounds;
  if (inner_) {
    if (!inner_->map_device_bounds(cur_bounds, ctm, output_bounds)) {
      ret = nullptr;
    }
    cur_bounds = output_bounds;
  }
  if (outer_) {
    if (!outer_->map_device_bounds(cur_bounds, ctm, output_bounds)) {
      ret = nullptr;
    }
  }
  return ret;
}

skity::Rect* ComposeImageFilter::get_input_device_bounds(
    const skity::Rect& output_bounds, const skity::Matrix& ctm,
    skity::Rect& input_bounds) const {
  skity::Rect cur_bounds = output_bounds;
  skity::Rect* ret = &input_bounds;
  // We set this result in case neither filter is present.
  input_bounds = output_bounds;
  if (outer_) {
    if (!outer_->get_input_device_bounds(cur_bounds, ctm, input_bounds)) {
      ret = nullptr;
    }
    cur_bounds = input_bounds;
  }
  if (inner_) {
    if (!inner_->get_input_device_bounds(cur_bounds, ctm, input_bounds)) {
      ret = nullptr;
    }
  }
  return ret;
}

}  // namespace clay
