// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/style/color_source.h"

#include "clay/fml/logging.h"

namespace clay {

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

std::shared_ptr<ImageColorSource> ColorSource::MakeImage(
    fml::RefPtr<PaintImage> image, TileMode tile_mode_x, TileMode tile_mode_y,
    ImageSampling sampling, const skity::Matrix* matrix) {
  return std::make_shared<ImageColorSource>(image, tile_mode_x, tile_mode_y,
                                            sampling, matrix);
}

std::shared_ptr<ColorColorSource> ColorSource::MakeColor(Color color) {
  return std::make_shared<ColorColorSource>(color);
}

}  // namespace clay
