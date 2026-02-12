// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/painter/image_painter.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <vector>

#include "base/trace/native/trace_event.h"
#include "clay/fml/logging.h"
#include "clay/gfx/image/image_resource.h"
#include "clay/gfx/rendering_backend.h"
#include "clay/ui/common/background_data.h"
#include "clay/ui/painter/gradient_factory.h"
#include "clay/ui/rendering/decode_utils.h"
#include "clay/ui/rendering/render_box.h"

namespace clay {
namespace {

void CalculateSizeByMode(FillMode mode, const skity::Vec2& input_size,
                         const skity::Vec2& output_size, skity::Vec2& src_size,
                         skity::Vec2& dest_size) {
  switch (mode) {
    case FillMode::kScaleToFill:  // BoxFit.fill
      src_size = input_size;
      dest_size = output_size;
      break;
    case FillMode::kAspectFit:  // BoxFit.contain
      src_size = input_size;
      if (output_size.x / output_size.y > src_size.x / src_size.y) {
        dest_size =
            skity::Vec2(src_size.x * output_size.y / src_size.y, output_size.y);
      } else {
        dest_size =
            skity::Vec2(output_size.x, src_size.y * output_size.x / src_size.x);
      }
      break;
    case FillMode::kAspectFill:  // BoxFit.fitWidth or BoxFit.fitHeight
      // if output_size w/h radio is higher, stretch width and cut height, which
      // means "fitWidth".
      if (input_size.x / input_size.y < output_size.x / output_size.y) {
        src_size = skity::Vec2(input_size.x,
                               input_size.x * output_size.y / output_size.x);
      } else {
        src_size = skity::Vec2(input_size.y * output_size.x / output_size.y,
                               input_size.y);
      }
      // no need to calculate |dest_size| here.
      dest_size = output_size;
      break;
    case FillMode::kCenter:  // BoxFit.none
      src_size = skity::Vec2(std::min(input_size.x, output_size.x),
                             std::min(input_size.y, output_size.y));
      dest_size = src_size;
      break;
    default:
      break;
  }
}

std::vector<skity::Rect> GenerateImageTileRects(
    const skity::Rect& output_rect, const skity::Rect& fundamental_rect,
    ImageRepeat repeat) {
  int start_x = 0;
  int start_y = 0;
  int stop_x = 0;
  int stop_y = 0;
  float stride_x = fundamental_rect.Width();
  float stride_y = fundamental_rect.Height();

  if (repeat == ImageRepeat::kRepeat || repeat == ImageRepeat::kRepeatX) {
    start_x = floor((output_rect.Left() - fundamental_rect.Left()) / stride_x);
    stop_x = ceil((output_rect.Right() - fundamental_rect.Right()) / stride_x);
  }

  if (repeat == ImageRepeat::kRepeat || repeat == ImageRepeat::kRepeatY) {
    start_y = floor((output_rect.Top() - fundamental_rect.Top()) / stride_y);
    stop_y =
        ceil((output_rect.Bottom() - fundamental_rect.Bottom()) / stride_y);
  }

  std::vector<skity::Rect> tile_rects = {};
  for (int i = start_x; i <= stop_x; ++i) {
    for (int j = start_y; j <= stop_y; ++j) {
      auto rect = fundamental_rect;
      rect.Offset(i * stride_x, j * stride_y);
      tile_rects.push_back(rect);
    }
  }

  return tile_rects;
}

ImageRepeat BackgroundRepeatToImageRepeat(
    const ClayBackgroundRepeatType& bg_repeat) {
  ImageRepeat repeat = ImageRepeat::kRepeat;
  if (bg_repeat == ClayBackgroundRepeatType::kNoRepeat) {
    repeat = ImageRepeat::kNoRepeat;
  } else if (bg_repeat == ClayBackgroundRepeatType::kRepeat) {
    repeat = ImageRepeat::kRepeat;
  } else if (bg_repeat == ClayBackgroundRepeatType::kRepeatX) {
    repeat = ImageRepeat::kRepeatX;
  } else if (bg_repeat == ClayBackgroundRepeatType::kRepeatY) {
    repeat = ImageRepeat::kRepeatY;
  }
  return repeat;
}

FloatRect GetPaintingBox(RenderBox* render_box, const FloatRect& frame_rect,
                         ClayBackgroundOriginType origin) {
  float output_x = frame_rect.x();
  float output_y = frame_rect.y();
  float output_w = frame_rect.width();
  float output_h = frame_rect.height();
  if (origin == ClayBackgroundOriginType::kPaddingBox) {
    output_x += render_box->BorderLeft();
    output_y += render_box->BorderTop();
    output_w -= (render_box->BorderLeft() + render_box->BorderRight());
    output_h -= (render_box->BorderTop() + render_box->BorderBottom());
  } else if (origin == ClayBackgroundOriginType::kContentBox) {
    output_x += (render_box->BorderLeft() + render_box->PaddingLeft());
    output_y += (render_box->BorderTop() + render_box->PaddingTop());
    output_w -= (render_box->BorderLeft() + render_box->BorderRight() +
                 render_box->PaddingLeft() + render_box->PaddingRight());
    output_h -= (render_box->BorderTop() + render_box->BorderBottom() +
                 render_box->PaddingTop() + render_box->PaddingBottom());
  }
  return FloatRect(output_x, output_y, output_w, output_h);
}

FloatSize GetBackgroundSize(float width, float height,
                            const BackgroundSize& size_x,
                            const BackgroundSize& size_y, float output_w,
                            float output_h, bool is_image) {
  float aspect = width / height;
  if (size_x.IsCover()) {  // apply cover
    width = output_w;
    height = width / aspect;
    if (height < output_h) {
      height = output_h;
      width = aspect * height;
    }
  } else if (size_x.IsContain()) {  // apply contain
    width = output_w;
    height = width / aspect;

    if (height > output_h) {
      height = output_h;
      width = aspect * height;
    }
  } else {
    width = size_x.apply(output_w, width);
    height = size_y.apply(output_h, height);

    if (size_x.IsAuto()) {
      // gradient has no aspect
      width = is_image ? aspect * height : output_w;
    }
    if (size_y.IsAuto()) {
      // gradient has no aspect
      height = is_image ? width / aspect : output_h;
    }
  }
  return FloatSize(width, height);
}

inline float GetPixelRatio(Renderer* renderer) {
  if (!renderer) {
    return 1.0f;
  }
  return renderer->GetPixelRatio<kPixelTypeLogical, kPixelTypeClay>();
}
}  // namespace

skity::Vec2 ImagePainter::CalculateSize(FillMode mode,
                                        const skity::Vec2& input_size,
                                        const skity::Vec2& output_size) {
  skity::Vec2 src_size, dest_size;
  CalculateSizeByMode(mode, input_size, output_size, src_size, dest_size);
  float scale = std::max(dest_size.x / src_size.x, dest_size.y / src_size.y);
  return skity::Vec2(input_size.x * scale, input_size.y * scale);
}

skity::Vec2 ImagePainter::CalculateSize(RenderBox* render_box,
                                        const skity::Vec2& input_size,
                                        const FloatRect& frame_rect,
                                        ClayBackgroundOriginType origin,
                                        const BackgroundSize& size_x,
                                        const BackgroundSize& size_y) {
  auto rect = GetPaintingBox(render_box, frame_rect, origin);
  FloatSize size = GetBackgroundSize(input_size.x, input_size.y, size_x, size_y,
                                     rect.width(), rect.height(), false);
  float scale =
      std::max(size.width() / input_size.x, size.height() / input_size.y);
  return skity::Vec2(input_size.x * scale, input_size.y * scale);
}

ImagePainter::ImagePainter(RenderBox* render_box) : render_box_(render_box) {
  FML_DCHECK(render_box_);
}

void ImagePainter::PaintImage(GraphicsContext* context,
                              const ImageData& image_data) {
  TRACE_EVENT("clay", "PaintImage");
  FML_DCHECK(context);

  if (!image_data.image_resource) {
    return;
  }

  FloatRect content = render_box_->ContentRect();
  if (content.IsEmpty()) {
    return;
  }

#ifdef ENABLE_SKITY
  if (!image_data.image_resource->GetImage()) {
    return;
  }
  image_data.image_resource->GetImage()->Upload(
      context->GetUnrefQueue(),
      Size{static_cast<int>(
               render_box_->GetRenderer()->ConvertTo<kPixelTypePhysical>(
                   content.width())),
           static_cast<int>(
               render_box_->GetRenderer()->ConvertTo<kPixelTypePhysical>(
                   content.height()))});
#endif  // ENABLE_SKITY

#ifndef ENABLE_SKITY
  DecodePriority priority = DecodePriority::kImmediate;
  if (render_box_->ImageDecodeWithPriority() &&
      image_data.image_resource->GetImage()->NeedDecode()) {
    priority = DecodeUtils::GetDecodePriority(render_box_);
  }
  auto image = image_data.image_resource->GetGraphicsImage(priority);
#else
  auto image = image_data.image_resource->GetGraphicsImage();
#endif  // ENABLE_SKITY

  if (!image || image->width() == 0 || image->height() == 0) {
    return;
  }

  // output_rect's offset is (0.f, 0.f).
  skity::Rect output_rect =
      skity::Rect::MakeXYWH(0.f, 0.f, content.width(), content.height());

  // If it is a 9-patch image, Ignore FillMode.
  // Ignore FillMode and ImageRepeat when drawing 9-patch.
  if (image_data.has_cap_insets) {
    const float pixel_ratio = GetPixelRatio(render_box_->GetRenderer());
    std::array<float, 4> cap_insets_px;
    for (size_t i = 0; i < 4; ++i) {
      if (i == 0 || i == 2) {
        cap_insets_px[i] = image_data.cap_insets[i].GetValue(image->height());
      } else {
        cap_insets_px[i] = image_data.cap_insets[i].GetValue(image->width());
      }
    }
    context->DrawNinePatch(image.get(), cap_insets_px,
                           image_data.cap_insets_scale, pixel_ratio,
                           output_rect);
    return;
  }

  // If not a 9-patch image, calculate src and dst sizes by mode:
  skity::Vec2 input_size = skity::Vec2(image->width(), image->height());
  skity::Vec2 output_size =
      skity::Vec2(output_rect.Width(), output_rect.Height());
  skity::Vec2 src_size = skity::Vec2{0, 0};
  skity::Vec2 dest_size = skity::Vec2{0, 0};
  CalculateSizeByMode(image_data.mode, input_size, output_size, src_size,
                      dest_size);

  float output_x = output_rect.Left();
  float output_y = output_rect.Top();
  float dest_x = output_x + (output_size.x - dest_size.x) * 0.5f;
  float dest_y = output_y + (output_size.y - dest_size.y) * 0.5f;
  skity::Rect dst_rect =
      skity::Rect::MakeXYWH(dest_x, dest_y, dest_size.x, dest_size.y);

  float src_x = (input_size.x - src_size.x) * 0.5f;
  float src_y = (input_size.y - src_size.y) * 0.5f;
  skity::Rect src_rect =
      skity::Rect::MakeXYWH(src_x, src_y, src_size.x, src_size.y);

  ImageRepeat repeat = image_data.repeat;

  Paint paint;
  // Always disable anti-alias when paint image. On particular low-end devices,
  // 'anti-alias' may result in the failure to display the image.
  paint.setAntiAlias(false);
  paint.setOpacity(image_data.image_opacity);

  if (image_data.tint_color.has_value()) {
    std::shared_ptr<ColorFilter> tint_filter = ColorFilter::MakeBlend(
        image_data.tint_color.value(), BlendMode::kSrcIn);
    paint.setColorFilter(tint_filter);
  }

  skity::RRect round_rect = image_data.round_rect;
  if (repeat == ImageRepeat::kNoRepeat) {
    if (round_rect.IsEmpty() || round_rect.IsRect()) {
      context->DrawImageRect(image, src_rect, dst_rect, GetSamplingOptions(),
                             &paint);
    } else {
      Paint work_paint = paint;
      work_paint.setDrawStyle(DrawStyle::kFill);
      skity::Matrix local_matrix =
          skity::Matrix::Translate(dst_rect.Left(), dst_rect.Top()) *
          skity::Matrix::Scale(dst_rect.Width() / src_rect.Width(),
                               dst_rect.Height() / src_rect.Height()) *
          skity::Matrix::Translate(-src_rect.Left(), -src_rect.Top());
      auto shader = ColorSource::MakeImage(
          PaintImage::Make(image), TileMode::kDecal, TileMode::kDecal,
          render_box_->ImageFilterMode() == FilterMode::kLinear
              ? ImageSampling::kLinear
              : ImageSampling::kNearestNeighbor,
          &local_matrix);
      work_paint.setColorSource(shader);
      context->DrawRRect(round_rect, work_paint);
    }
  } else {
    GraphicsContext::AutoRestore saver(context, true);
    if (round_rect.IsEmpty() || round_rect.IsRect()) {
      context->ClipRect(output_rect, GrClipOp::kIntersect, false);
    } else {
      context->ClipRRect(round_rect, GrClipOp::kIntersect, false);
    }
    std::vector<skity::Rect> tiles =
        GenerateImageTileRects(output_rect, dst_rect, repeat);
    for (auto& tile : tiles) {
      context->DrawImageRect(image, src_rect, tile, GetSamplingOptions(),
                             &paint);
    }
  }
}

void ImagePainter::PaintBackgroundImage(
    GraphicsContext* context, const FloatRect& frame_rect,
    const skity::RRect& rounded_rect, const BackgroundImage& bg_image,
    ClayBackgroundOriginType origin, ClayBackgroundClipType clip,
    const BackgroundSize& size_x, const BackgroundSize& size_y,
    const BackgroundPosition& position_x, const BackgroundPosition& position_y,
    ClayBackgroundRepeatType repeat_x, ClayBackgroundRepeatType repeat_y) {
  FML_DCHECK(context);
  if (frame_rect.IsEmpty() || !render_box_ || !render_box_->CanDisplay()) {
    return;
  }
  if (bg_image.IsEmpty()) {
    return;
  }

  auto rect = GetPaintingBox(render_box_, frame_rect, origin);
  float output_x = rect.x();
  float output_y = rect.y();
  float output_w = rect.width();
  float output_h = rect.height();

  static const float pixel_ratio = GetPixelRatio(render_box_->GetRenderer());
  float image_width, image_height, width, height;
  if (bg_image.IsSkImage()) {
    image_width = bg_image.Width();
    image_height = bg_image.Height();
    width = image_width * pixel_ratio;
    height = image_height * pixel_ratio;
  } else {
    width = image_width = output_w;
    height = image_height = output_h;
  }

  FloatSize size = GetBackgroundSize(width, height, size_x, size_y, output_w,
                                     output_h, (bg_image.GetImageResource()));
  width = size.width();
  height = size.height();
  if (width < 1 || height < 1) {
    return;
  }

  // linear-gradient size should be updated by
  // the background-size
  if (!bg_image.GetImageResource()) {
    image_width = width;
    image_height = height;
  }
  output_x += position_x.apply(output_w - width);
  output_y += position_y.apply(output_h - height);

  if (output_x >= frame_rect.MaxX() || output_y >= frame_rect.MaxY()) {
    return;
  }

  // Draw with ClayBackgroundClipType and ClayBackgroundRepeatType:
  FloatRect clip_rect = frame_rect;
  // The canvas was translated in PaintBoxDecorationBackground(), now we
  // are in the scope of its |GraphicsContext::AutoRestore|. When calculate the
  // clip_rect, we have to consider the offset.
  FloatPoint offset(render_box_->location());
  if (clip == ClayBackgroundClipType::kPaddingBox) {
    clip_rect = render_box_->PaddingRect();
    // opposite to canvas.Translate() in PaintBoxDecorationBackground().
    clip_rect.Move(-offset.x(), -offset.y());
  } else if (clip == ClayBackgroundClipType::kContentBox) {
    clip_rect = render_box_->ContentRect();
    // opposite to canvas.Translate() in PaintBoxDecorationBackground().
    clip_rect.Move(-offset.x(), -offset.y());
  }

  if (!clip_rect.IsEmpty()) {
    context->ClipRect(clip_rect, GrClipOp::kIntersect, false);
  }
  if (clip == ClayBackgroundClipType::kBorderArea) {
    FloatRect padding_rect = render_box_->PaddingRect();
    padding_rect.Move(-offset.x(), -offset.y());
    context->ClipRect(padding_rect, GrClipOp::kDifference, false);
  }

  GraphicsContext::AutoRestore saver(context, true);
  skity::Rect src_rect = skity::Rect::MakeXYWH(0, 0, image_width, image_height);
  skity::Rect dst_rect = skity::Rect::MakeXYWH(0, 0, width, height);
  skity::RRect draw_rounded_rect = skity::RRect::MakeRect(dst_rect);
  if (!rounded_rect.IsEmpty()) {
    draw_rounded_rect.SetRectRadii(dst_rect, rounded_rect.Radii());
  }
  ImageRepeat image_repeat_x = BackgroundRepeatToImageRepeat(repeat_x);
  ImageRepeat image_repeat_y = BackgroundRepeatToImageRepeat(repeat_y);
  if (image_repeat_x == ImageRepeat::kNoRepeat &&
      image_repeat_y == ImageRepeat::kNoRepeat) {
    context->Translate(output_x, output_y);
    PaintBackgroundImage(context, bg_image, src_rect, draw_rounded_rect);
    return;
  }
  // Draw the repeating images
  float end_x = std::max<float>(output_x + output_w, clip_rect.MaxX());
  float end_y = std::max<float>(output_y + output_h, clip_rect.MaxY());
  float start_x = output_x;
  float start_y = output_y;
  if (image_repeat_x == ImageRepeat::kRepeat ||
      image_repeat_x == ImageRepeat::kRepeatX) {
    start_x = start_x - std::ceil(start_x / width) * width;
  }
  if (image_repeat_y == ImageRepeat::kRepeat ||
      image_repeat_y == ImageRepeat::kRepeatY) {
    start_y = start_y - std::ceil(start_y / height) * height;
  }

  // Calculate number of repetitions in X and Y directions
  int num_repeats_x = std::ceil((end_x - start_x) / width);
  int num_repeats_y = std::ceil((end_y - start_y) / height);

  // Clip to the target rect when there are multiple repeat objects
  if ((num_repeats_x > 1 || num_repeats_y > 1) && !rounded_rect.IsEmpty()) {
    context->ClipRRect(rounded_rect, GrClipOp::kIntersect, true);
    draw_rounded_rect.SetRect(dst_rect);
  }

  for (float x = start_x; x < end_x; x += width) {
    for (float y = start_y; y < end_y; y += height) {
      GraphicsContext::AutoRestore saver(context, true);
      context->Translate(x, y);
      PaintBackgroundImage(context, bg_image, src_rect, draw_rounded_rect);
      if (image_repeat_y == ImageRepeat::kNoRepeat) {
        break;
      }
    }
    if (image_repeat_x == ImageRepeat::kNoRepeat) {
      break;
    }
  }
}

void ImagePainter::PaintBackgroundImage(GraphicsContext* context,
                                        const BackgroundImage& bg_image,
                                        const skity::Rect& src_rect,
                                        const skity::RRect& dst_rrect) {
  FML_DCHECK(context);

  if (bg_image.IsEmpty()) {
    return;
  }

  auto image_resource = bg_image.GetImageResource();
  if (image_resource) {
#ifdef ENABLE_SKITY
    if (!image_resource->GetImage()) {
      return;
    }
    image_resource->GetImage()->Upload(
        context->GetUnrefQueue(),
        Size{static_cast<int>(
                 render_box_->GetRenderer()->ConvertTo<kPixelTypePhysical>(
                     src_rect.Width())),
             static_cast<int>(
                 render_box_->GetRenderer()->ConvertTo<kPixelTypePhysical>(
                     src_rect.Height()))});
#endif  // ENABLE_SKITY

#ifndef ENABLE_SKITY
    DecodePriority priority = DecodePriority::kImmediate;
    if (render_box_->ImageDecodeWithPriority() &&
        image_resource->GetImage()->NeedDecode()) {
      priority = DecodeUtils::GetDecodePriority(render_box_);
    }
    auto image = image_resource->GetGraphicsImage(priority);
#else
    auto image = image_resource->GetGraphicsImage();
#endif  // ENABLE_SKITY
    if (!image) {
      return;
    }
    auto dst_rect = dst_rrect.GetRect();
    Paint paint;
    skity::Matrix local_matrix =
        skity::Matrix::Translate(dst_rect.Left(), dst_rect.Top()) *
        skity::Matrix::Scale(dst_rect.Width() / src_rect.Width(),
                             dst_rect.Height() / src_rect.Height()) *
        skity::Matrix::Translate(-src_rect.Left(), -src_rect.Top());
    auto shader = ColorSource::MakeImage(
        PaintImage::Make(image), TileMode::kDecal, TileMode::kDecal,
        render_box_->ImageFilterMode() == FilterMode::kLinear
            ? ImageSampling::kLinear
            : ImageSampling::kNearestNeighbor,
        &local_matrix);
    paint.setColorSource(shader);
    context->DrawRRect(dst_rrect, paint);
  } else {
    auto color_source = GradientFactory::CreateShader(bg_image.GetGradient(),
                                                      FloatRect(src_rect));
    Paint paint;
    paint.setDither(true);
    paint.setColorSource(color_source);
    context->DrawRRect(dst_rrect, paint);
  }
}

GrSamplingOptions ImagePainter::GetSamplingOptions() const {
  return SAMPLING_OPTIONS(render_box_->ImageFilterMode(), 0);
}

void ImagePainter::PaintMaskImage(GrCanvas* canvas,
                                  fml::RefPtr<GPUUnrefQueue> unref_queue) {
  auto pixel_ratio = GetPixelRatio(render_box_->GetRenderer());
  const MaskData& mask = render_box_->Mask();

  auto frame_rect = render_box_->GetFrameRect();
  frame_rect.SetX(0);
  frame_rect.SetY(0);
  for (size_t index = 0; index < mask.images.size(); index++) {
    const MaskImage& mask_image = mask.images[index];
    if (mask_image.IsEmpty()) {
      continue;
    }

    // Consider origin.
    ClayMaskOriginType origin = ClayMaskOriginType::kBorderBox;
    if (!mask.origins.empty()) {
      auto origin_index = index % mask.origins.size();
      origin = mask.origins[origin_index];
    }
    auto painting_box = GetPaintingBox(render_box_, frame_rect, origin);
    float image_width, image_height, width, height;
    if (mask_image.IsSkImage()) {
      image_width = mask_image.Width();
      image_height = mask_image.Height();
      width = image_width * pixel_ratio;
      height = image_height * pixel_ratio;
    } else {
      image_width = width = painting_box.width();
      image_height = height = painting_box.height();
    }

    // Consider size.
    if (mask.sizes.size() >= 2) {
      auto size_index = index % mask.sizes.size();
      MaskSize size_x, size_y;
      if (index * 2 >= mask.sizes.size()) {
        size_x = mask.sizes[mask.sizes.size() - 2];
        size_y = mask.sizes[mask.sizes.size() - 1];
      } else {
        size_x = mask.sizes[size_index * 2];
        size_y = mask.sizes[size_index * 2 + 1];
      }
      FloatSize size =
          GetBackgroundSize(width, height, size_x, size_y, painting_box.width(),
                            painting_box.height(), mask_image.IsSkImage());
      width = size.width();
      height = size.height();
      if (width < 1 || height < 1) {
        return;
      }
      // linear-gradient size should be updated by mask-size.
      if (!mask_image.IsSkImage()) {
        image_width = width;
        image_height = height;
      }
    }

    // Consider clip.
    FloatRect clip_rect;
    FloatPoint offset(render_box_->location());
    ClayMaskClipType clip_type = ClayMaskClipType::kBorderBox;
    if (!mask.clips.empty()) {
      auto clip_index = index % mask.clips.size();
      clip_type = mask.clips[clip_index];
    }
    switch (clip_type) {
      case ClayMaskClipType::kBorderBox: {
        clip_rect = frame_rect;
        break;
      }
      case ClayMaskClipType::kPaddingBox: {
        clip_rect = render_box_->PaddingRect();
        clip_rect.Move(-offset.x(), -offset.y());
        break;
      }
      case ClayMaskClipType::kContentBox: {
        clip_rect = render_box_->ContentRect();
        clip_rect.Move(-offset.x(), -offset.y());
        break;
      }
    }

    // Consider positions.
    float offset_x = painting_box.x(), offset_y = painting_box.y();
    if (mask.positions.size() < 2) {
      // Default position for mask is 'center'
      offset_x += (painting_box.width() - width) * 0.5;
      offset_y += (painting_box.height() - height) * 0.5;
    } else {
      auto pos_index = index % (mask.positions.size() / 2);

      float delta_width = painting_box.width() - width;
      float delta_height = painting_box.height() - height;

      MaskPosition pos_x = mask.positions[pos_index * 2];
      MaskPosition pos_y = mask.positions[pos_index * 2 + 1];
      offset_x += pos_x.apply(delta_width);
      offset_y += pos_y.apply(delta_height);
    }

    // Consider repeat.
    ClayMaskRepeatType repeat_x = ClayMaskRepeatType::kRepeat,
                       repeat_y = ClayMaskRepeatType::kRepeat;
    if (mask.repeats.size() >= 2) {
      auto repeat_index = index % (mask.repeats.size() / 2);
      repeat_x = mask.repeats[repeat_index * 2];
      repeat_y = mask.repeats[repeat_index * 2 + 1];
    }

    FloatRect src_rect = FloatRect(0, 0, image_width, image_height);
    FloatRect dst_rect = FloatRect(0, 0, width, height);
    int save_count = CANVAS_GET_SAVE_COUNT(canvas);
    CANVAS_CLIP_RECT(canvas, clip_rect);
    if (repeat_x == ClayBackgroundRepeatType::kNoRepeat &&
        repeat_y == ClayBackgroundRepeatType::kNoRepeat) {
      CANVAS_SAVE(canvas);
      CANVAS_TRANSLATE(canvas, offset_x, offset_y);
      PaintSingleMaskImage(canvas, mask_image, src_rect, dst_rect, unref_queue);
      CANVAS_RESTORE(canvas);
    } else {
      float end_x = std::max(painting_box.x() + painting_box.width(),
                             clip_rect.x() + clip_rect.width());
      float end_y = std::max(painting_box.y() + painting_box.height(),
                             clip_rect.y() + clip_rect.height());
      float start_x = offset_x, start_y = offset_y;

      if (repeat_x == ClayBackgroundRepeatType::kRepeat ||
          repeat_x == ClayBackgroundRepeatType::kRepeatX) {
        start_x = start_x - ((int)std::ceil((double)start_x / width)) * width;
      }
      if (repeat_y == ClayBackgroundRepeatType::kRepeat ||
          repeat_y == ClayBackgroundRepeatType::kRepeatY) {
        start_y = start_y - ((int)std::ceil((double)start_y / height)) * height;
      }

      CANVAS_SAVE(canvas);
      for (float x = start_x; x < end_x; x += width) {
        for (float y = start_y; y < end_y; y += height) {
          CANVAS_SAVE(canvas);
          CANVAS_TRANSLATE(canvas, x, y);
          PaintSingleMaskImage(canvas, mask_image, src_rect, dst_rect,
                               unref_queue);
          CANVAS_RESTORE(canvas);
          if (repeat_y == ClayBackgroundRepeatType::kNoRepeat) {
            break;
          }
        }
        if (repeat_x == ClayBackgroundRepeatType::kNoRepeat) {
          break;
        }
      }
      CANVAS_RESTORE(canvas);
    }
    CANVAS_RESTORE_TO_COUNT(canvas, save_count);
  }
}

void ImagePainter::PaintSingleMaskImage(
    GrCanvas* canvas, const MaskImage& mask_image, const FloatRect& src_rect,
    const FloatRect& dst_rect, fml::RefPtr<GPUUnrefQueue> unref_queue) {
  auto image_resource = mask_image.GetImageResource();
  GrPaint mask_paint;
  PAINT_SET_BLEND_MODE(mask_paint, BlendMode::kPlus);

#ifdef ENABLE_SKITY
  if (image_resource) {
    FML_DCHECK(unref_queue);
    if (!image_resource->GetImage()) {
      return;
    }
    image_resource->GetImage()->Upload(
        unref_queue,
        Size{static_cast<int>(
                 render_box_->GetRenderer()->ConvertTo<kPixelTypePhysical>(
                     src_rect.width())),
             static_cast<int>(
                 render_box_->GetRenderer()->ConvertTo<kPixelTypePhysical>(
                     src_rect.height()))});
  }
#endif  // ENABLE_SKITY

  if (image_resource && image_resource->GetGraphicsImage() &&
      image_resource->GetGraphicsImage()->gr_image()) {
    auto image = image_resource->GetGraphicsImage()->gr_image();

    CANVAS_DRAW_IMAGE_SRC_RECT(canvas, image, src_rect, dst_rect,
                               GetSamplingOptions(), mask_paint);
  } else if (!mask_image.IsSkImage()) {
    auto color_source =
        GradientFactory::CreateShader(mask_image.GetGradient(), src_rect);
    auto shader = color_source->gr_object();
    PAINT_SET_SHADER(mask_paint, shader);
    CANVAS_DRAW_RECT(canvas, dst_rect, mask_paint);
  }
}

}  // namespace clay
