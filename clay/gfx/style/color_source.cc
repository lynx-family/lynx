// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/style/color_source.h"

#include "clay/fml/logging.h"

namespace clay {

std::shared_ptr<ColorSource> ColorSource::From(GrShader* sk_shader) {
#ifndef ENABLE_SKITY
  if (sk_shader == nullptr) {
    return nullptr;
  }
  {
    SkMatrix local_matrix;
    SkTileMode xy[2];
    SkImage* image = sk_shader->isAImage(&local_matrix, xy);
    auto skity_matrix = clay::ConvertSkMatrixToSkityMatrix(local_matrix);
    if (image) {
      return std::make_shared<ImageColorSource>(
          PaintImage::Make(image), ToClay(xy[0]), ToClay(xy[1]),
          ImageSampling::kLinear, &skity_matrix);
    }
  }
  // Skia provides |SkShader->asAGradient(&info)| method to access the
  // parameters of a gradient, but the info object being filled has a number
  // of parameters which are missing, including the local matrix in every
  // gradient, and the sweep angles in the sweep gradients.
  //
  // Since we can't reproduce every Gradient, and customers rely on using
  // gradients with matrices in text code, we have to just use an Unknown
  // ColorSource to express all gradients.
  // (see: https://github.com/flutter/flutter/issues/102947)
  return std::make_shared<UnknownColorSource>(sk_ref_sp(sk_shader));
#else
  return nullptr;
#endif  // ENABLE_SKITY
}

static void DlGradientDeleter(void* p) {
  // Some of our target environments would prefer a sized delete,
  // but other target environments do not have that operator.
  // Use an unsized delete until we get better agreement in the
  // environments.
  // See https://github.com/flutter/flutter/issues/100327
  ::operator delete(p);
}

std::shared_ptr<ColorSource> ColorSource::MakeLinear(
    const skity::Vec2 start_point, const skity::Vec2 end_point,
    uint32_t stop_count, const Color* colors, const float* stops,
    TileMode tile_mode, const skity::Matrix* matrix) {
  size_t needed = sizeof(LinearGradientColorSource) +
                  (stop_count * (sizeof(uint32_t) + sizeof(float)));

  void* storage = ::operator new(needed);

  std::shared_ptr<LinearGradientColorSource> ret;
  ret.reset(new (storage)
                LinearGradientColorSource(start_point, end_point, stop_count,
                                          colors, stops, tile_mode, matrix),
            DlGradientDeleter);
  return std::move(ret);
}

std::shared_ptr<ColorSource> ColorSource::MakeRadial(
    skity::Vec2 center, float radius, uint32_t stop_count, const Color* colors,
    const float* stops, TileMode tile_mode, const skity::Matrix* matrix) {
  size_t needed = sizeof(RadialGradientColorSource) +
                  (stop_count * (sizeof(uint32_t) + sizeof(float)));

  void* storage = ::operator new(needed);

  std::shared_ptr<RadialGradientColorSource> ret;
  ret.reset(new (storage) RadialGradientColorSource(
                center, radius, stop_count, colors, stops, tile_mode, matrix),
            DlGradientDeleter);
  return std::move(ret);
}

std::shared_ptr<ColorSource> ColorSource::MakeConical(
    skity::Vec2 start_center, float start_radius, skity::Vec2 end_center,
    float end_radius, uint32_t stop_count, const Color* colors,
    const float* stops, TileMode tile_mode, const skity::Matrix* matrix) {
  size_t needed = sizeof(ConicalGradientColorSource) +
                  (stop_count * (sizeof(uint32_t) + sizeof(float)));

  void* storage = ::operator new(needed);

  std::shared_ptr<ConicalGradientColorSource> ret;
  ret.reset(new (storage) ConicalGradientColorSource(
                start_center, start_radius, end_center, end_radius, stop_count,
                colors, stops, tile_mode, matrix),
            DlGradientDeleter);
  return std::move(ret);
}

std::shared_ptr<ColorSource> ColorSource::MakeSweep(
    skity::Vec2 center, float start, float end, uint32_t stop_count,
    const Color* colors, const float* stops, TileMode tile_mode,
    const skity::Matrix* matrix) {
  size_t needed = sizeof(SweepGradientColorSource) +
                  (stop_count * (sizeof(uint32_t) + sizeof(float)));

  void* storage = ::operator new(needed);

  std::shared_ptr<SweepGradientColorSource> ret;
  ret.reset(new (storage)
                SweepGradientColorSource(center, start, end, stop_count, colors,
                                         stops, tile_mode, matrix),
            DlGradientDeleter);
  return std::move(ret);
}

std::shared_ptr<RuntimeEffectColorSource> ColorSource::MakeRuntimeEffect(
    fml::RefPtr<RuntimeEffect> runtime_effect,
    std::vector<std::shared_ptr<ColorSource>> samplers,
    std::shared_ptr<std::vector<uint8_t>> uniform_data) {
  FML_DCHECK(uniform_data != nullptr);
  return std::make_shared<RuntimeEffectColorSource>(
      std::move(runtime_effect), std::move(samplers), std::move(uniform_data));
}

}  // namespace clay
