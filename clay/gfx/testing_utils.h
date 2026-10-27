// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CLAY_GFX_TESTING_UTILS_H_
#define CLAY_GFX_TESTING_UTILS_H_

#include "clay/gfx/gfx_rendering_backend.h"
#include "clay/gfx/paint.h"
#include "clay/gfx/style/color_source.h"
#include "clay/gfx/style/image_filter.h"
#include "clay/gfx/style/mask_filter.h"
#include "clay/gfx/style/path_effect.h"
#include "clay/gfx/style/vertices.h"
#include "clay/gfx/text_blob.h"

namespace clay {
using DlColor = clay::Color;
using DlPathEffect = clay::PathEffect;
using DlBlendMode = clay::BlendMode;
using DlBlurImageFilter = clay::BlurImageFilter;
using DlTileMode = clay::TileMode;
using DlMatrixColorFilter = clay::MatrixColorFilter;
using DlUnknownImageFilter = clay::UnknownImageFilter;
using DlDilateImageFilter = clay::DilateImageFilter;
using DlErodeImageFilter = clay::ErodeImageFilter;
using DlBlurMaskFilter = clay::BlurMaskFilter;
using DlColorSource = clay::ColorSource;
using DlDashPathEffect = clay::DashPathEffect;
using DlImageColorSource = clay::ImageColorSource;
using DlImageSampling = clay::ImageSampling;
using DlFilterMode = clay::FilterMode;
using DlColorFilter = clay::ColorFilter;
using DlBlendColorFilter = clay::BlendColorFilter;
using DlSrgbToLinearGammaColorFilter = clay::SrgbToLinearGammaColorFilter;
using DlLinearToSrgbGammaColorFilter = clay::LinearToSrgbGammaColorFilter;
using DlMatrixImageFilter = clay::MatrixImageFilter;
using DlImageFilter = clay::ImageFilter;
using DlColorFilterType = clay::ColorFilterType;
using DlUnknownColorFilter = clay::UnknownColorFilter;
using DlRuntimeEffect = clay::RuntimeEffect;
using DlColorSourceType = clay::ColorSourceType;
using DlColorColorSource = clay::ColorColorSource;
using DlUnknownColorSource = clay::UnknownColorSource;
using DlRuntimeEffectColorSource = clay::RuntimeEffectColorSource;
using DlImageFilterType = clay::ImageFilterType;
using DlColorFilterImageFilter = clay::ColorFilterImageFilter;
using DlMaskFilter = clay::MaskFilter;
using DlComposeImageFilter = clay::ComposeImageFilter;
using DlLocalMatrixImageFilter = clay::LocalMatrixImageFilter;
using DlPathEffectType = clay::PathEffectType;
using DlUnknownPathEffect = clay::UnknownPathEffect;
using DlMaskFilterType = clay::MaskFilterType;
using DlUnknownMaskFilter = clay::UnknownMaskFilter;
using DlLinearGradientColorSource = clay::LinearGradientColorSource;
using DlRadialGradientColorSource = clay::RadialGradientColorSource;
using DlConicalGradientColorSource = clay::ConicalGradientColorSource;
using DlSweepGradientColorSource = clay::SweepGradientColorSource;
using DlPaint = clay::Paint;
using DlDrawStyle = clay::DrawStyle;
using DlStrokeCap = clay::StrokeCap;
using DlStrokeJoin = clay::StrokeJoin;
using DlVertices = clay::Vertices;
using DlVertexMode = clay::VertexMode;
using DlTextBlob = clay::TextBlob;
using DlImage = clay::PaintImage;
}  // namespace clay

#endif  // CLAY_GFX_TESTING_UTILS_H_
