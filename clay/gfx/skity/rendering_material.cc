// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/skity/rendering_material.h"

#include <utility>
#include <vector>

#include "clay/gfx/style/color.h"
#include "skity/graphic/color.hpp"

namespace clay {

#define SKITY_UNIMPLEMENTED(name) \
  FML_DLOG(ERROR) << "Unimplemented with skity in " << name;

static skity::Color4f to_color_4f(Color const& color) {
  return {color.RedF(), color.GreenF(), color.BlueF(), color.AlphaF()};
}

static skity::Color to_color(Color const& color) {
  return skity::ColorSetARGB(color.Alpha(), color.Red(), color.Green(),
                             color.Blue());
}

skity::TileMode RenderingMaterial::ToSkityTileMode(TileMode tile_mode) {
  switch (tile_mode) {
    case TileMode::kClamp:
      return skity::TileMode::kClamp;
    case TileMode::kRepeat:
      return skity::TileMode::kRepeat;
    case TileMode::kMirror:
      return skity::TileMode::kMirror;
    case TileMode::kDecal:
      return skity::TileMode::kDecal;
  }
}

std::shared_ptr<skity::Shader> RenderingMaterial::ToSkityLinearGradient(
    const LinearGradientColorSource* linear_gradient_source) {
  std::vector<skity::Point> pts{
      {linear_gradient_source->start_point().x,
       linear_gradient_source->start_point().y, 0.f, 1.f},
      {linear_gradient_source->end_point().x,
       linear_gradient_source->end_point().y, 0.f, 1.f},
  };
  int stop_count = linear_gradient_source->stop_count();
  std::vector<skity::Vec4> colors(stop_count);
  std::vector<float> stops(stop_count);
  for (size_t i = 0; i < static_cast<size_t>(stop_count); i++) {
    colors[i] = to_color_4f(linear_gradient_source->colors()[i]);
    stops[i] = linear_gradient_source->stops()[i];
  }
  auto shader = skity::Shader::MakeLinear(
      pts.data(), colors.data(), stops.empty() ? nullptr : stops.data(),
      colors.size(), ToSkityTileMode(linear_gradient_source->tile_mode()));
  if (linear_gradient_source->matrix_ptr()) {
    shader->SetLocalMatrix(linear_gradient_source->matrix());
  }
  return shader;
}

std::shared_ptr<skity::Shader> RenderingMaterial::ToSkityRadialGradient(
    const RadialGradientColorSource* radial_gradient_source) {
  skity::Point center{
      radial_gradient_source->center().x,
      radial_gradient_source->center().y,
      0.f,
      1.f,
  };
  int stop_count = radial_gradient_source->stop_count();
  std::vector<skity::Vec4> colors(stop_count);
  std::vector<float> stops(stop_count);
  for (size_t i = 0; i < static_cast<size_t>(stop_count); i++) {
    colors[i] = to_color_4f(radial_gradient_source->colors()[i]);
    stops[i] = radial_gradient_source->stops()[i];
  }
  auto shader = skity::Shader::MakeRadial(
      center, radial_gradient_source->radius(), colors.data(),
      stops.empty() ? nullptr : stops.data(), colors.size(),
      ToSkityTileMode(radial_gradient_source->tile_mode()));
  if (radial_gradient_source->matrix_ptr()) {
    shader->SetLocalMatrix(radial_gradient_source->matrix());
  }
  return shader;
}

std::shared_ptr<skity::PathEffect> RenderingMaterial::ToSkityPathEffect(
    const PathEffect* effect) {
  const DashPathEffect* dash_effect = effect->asDash();
  FML_DCHECK(dash_effect);
  const int interval_count = dash_effect->count_;
  FML_DCHECK(interval_count > 0);
  std::vector<float> pattern(interval_count);
  for (size_t i = 0; i < static_cast<size_t>(interval_count); i++) {
    pattern[i] = dash_effect->intervals()[i];
  }
  return skity::PathEffect::MakeDashPathEffect(
      pattern.data(), dash_effect->count_, dash_effect->phase_);
}

std::shared_ptr<skity::ColorFilter> RenderingMaterial::ToSkityColorFilter(
    const ColorFilter* color_filter) {
  switch (color_filter->type()) {
    case ColorFilterType::kBlend: {
      const BlendColorFilter* blend_color_filter = color_filter->asBlend();
      return skity::ColorFilters::Blend(
          to_color(blend_color_filter->color()),
          ToSkityBlendMode(blend_color_filter->mode()));
    }
    case ColorFilterType::kMatrix: {
      const MatrixColorFilter* matrix_color_filter = color_filter->asMatrix();
      float matrix[20] = {0};
      matrix_color_filter->get_matrix(matrix);
      return skity::ColorFilters::Matrix(matrix);
    }
    case ColorFilterType::kSrgbToLinearGamma: {
      return skity::ColorFilters::SRGBToLinearGamma();
    }
    case ColorFilterType::kLinearToSrgbGamma: {
      return skity::ColorFilters::LinearToSRGBGamma();
    }
    default:
      return nullptr;
  }
}

skity::BlendMode RenderingMaterial::ToSkityBlendMode(BlendMode mode) {
  switch (mode) {
    case BlendMode::kClear:
      return skity::BlendMode::kClear;
    case BlendMode::kSrc:
      return skity::BlendMode::kSrc;
    case BlendMode::kDst:
      return skity::BlendMode::kDst;
    case BlendMode::kSrcOver:
      return skity::BlendMode::kSrcOver;
    case BlendMode::kDstOver:
      return skity::BlendMode::kDstOver;
    case BlendMode::kSrcIn:
      return skity::BlendMode::kSrcIn;
    case BlendMode::kDstIn:
      return skity::BlendMode::kDstIn;
    case BlendMode::kSrcOut:
      return skity::BlendMode::kSrcOut;
    case BlendMode::kDstOut:
      return skity::BlendMode::kDstOut;
    case BlendMode::kSrcATop:
      return skity::BlendMode::kSrcATop;
    case BlendMode::kDstATop:
      return skity::BlendMode::kDstATop;
    case BlendMode::kXor:
      return skity::BlendMode::kXor;
    case BlendMode::kPlus:
      return skity::BlendMode::kPlus;
    case BlendMode::kModulate:
      return skity::BlendMode::kModulate;
    case BlendMode::kScreen:
      return skity::BlendMode::kScreen;
    case BlendMode::kOverlay:
      return skity::BlendMode::kOverlay;
    case BlendMode::kDarken:
      return skity::BlendMode::kDarken;
    case BlendMode::kLighten:
      return skity::BlendMode::kLighten;
    case BlendMode::kColorDodge:
      return skity::BlendMode::kColorDodge;
    case BlendMode::kColorBurn:
      return skity::BlendMode::kColorBurn;
    default:
      // TODO(tangruiwen) support other blend mode
      return skity::BlendMode::kSrcOut;
  }
}

std::shared_ptr<skity::ImageFilter> RenderingMaterial::ToSkityImageFilter(
    const clay::ImageFilter* image_filter) {
  switch (image_filter->type()) {
    case clay::ImageFilterType::kBlur: {
      const clay::BlurImageFilter* blur_image_filter = image_filter->asBlur();
      return skity::ImageFilters::Blur(blur_image_filter->sigma_x(),
                                       blur_image_filter->sigma_y());
    }
    case clay::ImageFilterType::kDropShadow: {
      const clay::DropShadowImageFilter* drop_shadow =
          image_filter->asDropShadow();
      return skity::ImageFilters::DropShadow(
          drop_shadow->dx(), drop_shadow->dy(), drop_shadow->sigma_x(),
          drop_shadow->sigma_y(), drop_shadow->color(), nullptr);
    }
    case clay::ImageFilterType::kDilate: {
      const clay::DilateImageFilter* dilate_image_filter =
          image_filter->asDilate();
      return skity::ImageFilters::Dilate(dilate_image_filter->radius_x(),
                                         dilate_image_filter->radius_y());
    }
    case clay::ImageFilterType::kErode: {
      const clay::ErodeImageFilter* erode_image_filter =
          image_filter->asErode();
      return skity::ImageFilters::Erode(erode_image_filter->radius_x(),
                                        erode_image_filter->radius_y());
    }
    case clay::ImageFilterType::kMatrix: {
      // const clay::DlMatrixImageFilter* matrix_image_filter =
      //     image_filter->asMatrix();
      // return skity::ImageFilters::Matrix();
      SKITY_UNIMPLEMENTED("ImageFilter-Matrix");
      return nullptr;
    }
    case clay::ImageFilterType::kComposeFilter: {
      SKITY_UNIMPLEMENTED("ImageFilter-ComposeFilter");
      return nullptr;
    }
    case clay::ImageFilterType::kColorFilter: {
      SKITY_UNIMPLEMENTED("ImageFilter-ColorFilter");
      return nullptr;
    }
    default:
      return nullptr;
  }
}

}  // namespace clay
