// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CLAY_SHELL_COMMON_SCREENSHOT_UTILS_H_
#define CLAY_SHELL_COMMON_SCREENSHOT_UTILS_H_

#include <memory>

#include "clay/flow/layers/layer_tree.h"
#include "clay/flow/surface.h"
#include "clay/gfx/paint_image.h"
#include "clay/gfx/rendering_backend.h"
#include "skity/geometry/vector.hpp"

namespace clay {

struct ScreenshotData {
  enum class ScreenshotType {
    SkiaPicture,
    UncompressedImage,
    CompressedImage,
  };

  GrDataPtr data;

  skity::Vec2 frame_size = {0, 0};

  ScreenshotData();
  ScreenshotData(GrDataPtr p_data, const skity::Vec2& p_size);
  ScreenshotData(const ScreenshotData& other);

  ~ScreenshotData();
};

GrImagePtr TakeScreenshotWithOpaque(
    clay::LayerTree* tree, Surface* surface,
    clay::CompositorContext& compositor_context, bool opaque,
    uint32_t background_color = Color::kTransparent().Value());

ScreenshotData TakeScreenshotWithBase64(
    LayerTree* layer_tree, ScreenshotData::ScreenshotType type,
    Surface* surface, CompositorContext* compositor_context, bool base64_encode,
    uint32_t background_color = Color::kTransparent().Value());

fml::RefPtr<PaintImage> TakeScreenshot(
    std::unique_ptr<clay::LayerTree> layer_tree, Surface* surface,
    CompositorContext* context);
fml::RefPtr<PaintImage> TakeScreenshot(GrPicturePtr picture, skity::Vec2 size);
};  // namespace clay

#endif  // CLAY_SHELL_COMMON_SCREENSHOT_UTILS_H_
