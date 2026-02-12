// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_PAINTER_IMAGE_PAINTER_H_
#define CLAY_UI_PAINTER_IMAGE_PAINTER_H_

#include "clay/gfx/geometry/float_rounded_rect.h"
#include "clay/gfx/graphics_context.h"
#include "clay/gfx/image/image_data.h"
#include "clay/public/clay.h"
#include "clay/public/style_types.h"
#include "clay/ui/common/background_data.h"

namespace clay {

class BackgroundImage;
class ImageResource;
class RenderBox;
class BackgroundSize;
class BackgroundPosition;

class ImagePainter {
 public:
  static skity::Vec2 CalculateSize(FillMode mode, const skity::Vec2& input_size,
                                   const skity::Vec2& output_size);
  static skity::Vec2 CalculateSize(RenderBox* render_box,
                                   const skity::Vec2& input_size,
                                   const FloatRect& frame_rect,
                                   ClayBackgroundOriginType origin,
                                   const BackgroundSize& size_x,
                                   const BackgroundSize& size_y);

  explicit ImagePainter(RenderBox* render_box);
  ~ImagePainter() = default;

  void PaintImage(GraphicsContext* context, const ImageData& image_data);
  void PaintBackgroundImage(
      GraphicsContext* context, const FloatRect& frame_rect,
      const skity::RRect& rounded_rect, const BackgroundImage& bg_image,
      ClayBackgroundOriginType origin, ClayBackgroundClipType clip,
      const BackgroundSize& size_x, const BackgroundSize& size_y,
      const BackgroundPosition& position_x,
      const BackgroundPosition& position_y, ClayBackgroundRepeatType repeat_x,
      ClayBackgroundRepeatType repeat_y);

  void PaintMaskImage(GrCanvas* canvas,
                      fml::RefPtr<GPUUnrefQueue> unref_queue = nullptr);

 private:
  void PaintBackgroundImage(GraphicsContext* context,
                            const BackgroundImage& bg_image,
                            const skity::Rect& src_rect,
                            const skity::RRect& dst_rrect);
  GrSamplingOptions GetSamplingOptions() const;

  void PaintSingleMaskImage(GrCanvas* canvas, const MaskImage& mask_image,
                            const FloatRect& src_rect,
                            const FloatRect& dst_rect,
                            fml::RefPtr<GPUUnrefQueue> unref_queue);

  RenderBox* render_box_ = nullptr;
};

}  // namespace clay

#endif  // CLAY_UI_PAINTER_IMAGE_PAINTER_H_
