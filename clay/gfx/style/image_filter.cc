// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/style/image_filter.h"

namespace clay {

std::shared_ptr<ImageFilter> ImageFilter::From(const GrImageFilter* sk_filter) {
  if (sk_filter == nullptr) {
    return nullptr;
  }
#ifndef ENABLE_SKITY
  {
    SkColorFilter* color_filter;
    if (sk_filter->isColorFilterNode(&color_filter)) {
      FML_DCHECK(color_filter != nullptr);
      // If |isColorFilterNode| succeeds, the pointer it sets into color_filter
      // will be ref'd already so we do not use sk_ref_sp() here as that would
      // double-ref the color filter object. Instead we use a bare sk_sp
      // constructor to adopt this reference into an sk_sp<SkCF> without
      // reffing it and let the compiler manage the refs.
      return std::make_shared<ColorFilterImageFilter>(
          ColorFilter::From(sk_sp<SkColorFilter>(color_filter)));
    }
  }
  return std::make_shared<UnknownImageFilter>(sk_ref_sp(sk_filter));
#else
  FML_UNIMPLEMENTED();
  return std::make_shared<UnknownImageFilter>(sk_filter);
#endif  // ENABLE_SKITY
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
