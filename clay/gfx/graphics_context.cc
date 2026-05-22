// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/graphics_context.h"

#include <memory>

#include "clay/fml/logging.h"
#include "clay/gfx/image/image_resource.h"
#include "clay/gfx/rendering_backend.h"

namespace clay {

GraphicsContext::AutoRestore::AutoRestore(GraphicsContext* context,
                                          bool do_save)
    : canvas_(context->Canvas()) {
  if (canvas_) {
    save_count_ = canvas_->GetSaveCount();
    if (do_save) {
      canvas_->Save();
    }
  }
}

GraphicsContext::AutoRestore::~AutoRestore() {
  if (canvas_) {
    canvas_->RestoreToCount(save_count_);
  }
}

void GraphicsContext::AutoRestore::Restore() {
  if (canvas_) {
    canvas_->RestoreToCount(save_count_);
    canvas_ = nullptr;
  }
}

static const int dash_ratio = 3;  // Ratio of the length of a dash to its width.

bool GraphicsContext::BeginRecording(const skity::Rect& bounds) {
  return paint_recorder_.BeginRecording(bounds) != nullptr;
}

#ifndef ENABLE_SKITY
bool GraphicsContext::BeginRecording(const SkBitmap& bitmap) {
  return paint_recorder_.BeginRecording(bitmap) != nullptr;
}
#endif  // ENABLE_SKITY

bool GraphicsContext::IsRecording() const {
  return paint_recorder_.IsRecording();
}

std::unique_ptr<Picture> GraphicsContext::FinishRecording() {
  return paint_recorder_.FinishRecordingAsPicture();
}

void GraphicsContext::DrawNinePatch(const GraphicsImage* image,
                                    const std::array<float, 4>& cap_insets,
                                    float cap_insets_scale, float pixel_ratio,
                                    const skity::Rect& dst_rect) {
  TRACE_EVENT("clay", "DrawNinePatch");
  FML_DCHECK(IsRecording());
  FML_DCHECK(image);

  // Apply CapInsetsScale:
  if (cap_insets_scale <= 0) {
    FML_DLOG(ERROR) << "Get invalid cap-insets-scale value.";
    cap_insets_scale = 1.f;
  }
  float border_left_w = cap_insets[3];
  float border_top_h = cap_insets[0];
  float center_rect_scale_left = border_left_w * cap_insets_scale;
  float center_rect_scale_top = border_top_h * cap_insets_scale;

  float border_right_w = cap_insets[1];
  float border_bottom_h = cap_insets[2];
  float center_rect_scale_right =
      image->width() - border_right_w * cap_insets_scale;
  float center_rect_scale_bottom =
      image->height() - border_bottom_h * cap_insets_scale;

  skity::Rect center_rect_scale =
      skity::Rect::MakeLTRB(center_rect_scale_left, center_rect_scale_top,
                            center_rect_scale_right, center_rect_scale_bottom);

  // Failed to apply |cap_insets_scale|, ignore it.
  if (!center_rect_scale.IsSorted()) {
    center_rect_scale = skity::Rect::MakeWH(image->width(), image->height());
  }

  center_rect_scale.RoundIn();
  if (center_rect_scale ==
      skity::Rect::MakeWH(image->width(), image->height())) {
    Canvas()->DrawImageRect(image, dst_rect,
                            SAMPLING_OPTIONS(FilterMode::kLinear, 0), nullptr);
  } else {
    // 'SkCanvas->drawImageNine' can only handle images which would be divided
    // into 9 parts. And images expected to divided into 3 or less parts would
    // be paint as normal images through 'SkCanvas->drawImageRect'. To handle
    // all cases, we draw parts individually by ourselves.
    DrawNinePatchIndividually(image, pixel_ratio, cap_insets, center_rect_scale,
                              dst_rect);
  }
}

void GraphicsContext::DrawNinePatchIndividually(
    const GraphicsImage* image, float pixel_ratio,
    const std::array<float, 4>& cap_insets, const skity::Rect& center,
    const skity::Rect& dst_rect) {
  skity::Rect src0 = skity::Rect::MakeLTRB(0, 0, center.Left(), center.Top());
  skity::Rect src1 = skity::Rect::MakeLTRB(src0.Right(), src0.Top(),
                                           center.Right(), src0.Bottom());
  skity::Rect src2 = skity::Rect::MakeLTRB(src1.Right(), src0.Top(),
                                           image->width(), src0.Bottom());
  skity::Rect src3 = skity::Rect::MakeLTRB(src0.Left(), src0.Bottom(),
                                           src0.Right(), center.Bottom());
  skity::Rect src4 = skity::Rect::MakeLTRB(src3.Right(), src3.Top(),
                                           src1.Right(), src3.Bottom());
  skity::Rect src5 = skity::Rect::MakeLTRB(src4.Right(), src3.Top(),
                                           src2.Right(), src3.Bottom());
  skity::Rect src6 = skity::Rect::MakeLTRB(src3.Left(), src3.Bottom(),
                                           src3.Right(), image->height());
  skity::Rect src7 = skity::Rect::MakeLTRB(src6.Right(), src6.Top(),
                                           src4.Right(), src6.Bottom());
  skity::Rect src8 = skity::Rect::MakeLTRB(src7.Right(), src6.Top(),
                                           src5.Right(), src6.Bottom());

  float offset_x = dst_rect.Left();
  float offset_y = dst_rect.Top();
  skity::Rect dst0 = skity::Rect::MakeLTRB(0, 0, cap_insets[3] * pixel_ratio,
                                           cap_insets[0] * pixel_ratio);
  skity::Rect dst1 = skity::Rect::MakeLTRB(
      dst0.Right(), dst0.Top(), dst_rect.Width() - cap_insets[1] * pixel_ratio,
      dst0.Bottom());
  skity::Rect dst2 = skity::Rect::MakeLTRB(dst1.Right(), dst0.Top(),
                                           dst_rect.Width(), dst0.Bottom());
  skity::Rect dst3 =
      skity::Rect::MakeLTRB(dst0.Left(), dst0.Bottom(), dst0.Right(),
                            dst_rect.Height() - cap_insets[2] * pixel_ratio);
  skity::Rect dst4 = skity::Rect::MakeLTRB(dst3.Right(), dst3.Top(),
                                           dst1.Right(), dst3.Bottom());
  skity::Rect dst5 = skity::Rect::MakeLTRB(dst4.Right(), dst2.Bottom(),
                                           dst2.Right(), dst3.Bottom());
  skity::Rect dst6 = skity::Rect::MakeLTRB(dst0.Left(), dst3.Bottom(),
                                           dst3.Right(), dst_rect.Height());
  skity::Rect dst7 = skity::Rect::MakeLTRB(dst6.Right(), dst4.Bottom(),
                                           dst4.Right(), dst6.Bottom());
  skity::Rect dst8 = skity::Rect::MakeLTRB(dst7.Right(), dst5.Bottom(),
                                           dst5.Right(), dst7.Bottom());

  dst0.Offset(offset_x, offset_y);
  dst1.Offset(offset_x, offset_y);
  dst2.Offset(offset_x, offset_y);
  dst3.Offset(offset_x, offset_y);
  dst4.Offset(offset_x, offset_y);
  dst5.Offset(offset_x, offset_y);
  dst6.Offset(offset_x, offset_y);
  dst7.Offset(offset_x, offset_y);
  dst8.Offset(offset_x, offset_y);

  Canvas()->DrawImageRect(image, src0, dst0,
                          SAMPLING_OPTIONS(FilterMode::kLinear, 0), nullptr);
  Canvas()->DrawImageRect(image, src1, dst1,
                          SAMPLING_OPTIONS(FilterMode::kLinear, 0), nullptr);
  Canvas()->DrawImageRect(image, src2, dst2,
                          SAMPLING_OPTIONS(FilterMode::kLinear, 0), nullptr);
  Canvas()->DrawImageRect(image, src3, dst3,
                          SAMPLING_OPTIONS(FilterMode::kLinear, 0), nullptr);
  Canvas()->DrawImageRect(image, src4, dst4,
                          SAMPLING_OPTIONS(FilterMode::kLinear, 0), nullptr);
  Canvas()->DrawImageRect(image, src5, dst5,
                          SAMPLING_OPTIONS(FilterMode::kLinear, 0), nullptr);
  Canvas()->DrawImageRect(image, src6, dst6,
                          SAMPLING_OPTIONS(FilterMode::kLinear, 0), nullptr);
  Canvas()->DrawImageRect(image, src7, dst7,
                          SAMPLING_OPTIONS(FilterMode::kLinear, 0), nullptr);
  Canvas()->DrawImageRect(image, src8, dst8,
                          SAMPLING_OPTIONS(FilterMode::kLinear, 0), nullptr);
}

void GraphicsContext::ApplyBlurEffect(float blur, Paint* paint) const {
  TRACE_EVENT("clay", "ApplyBlurEffect");

  if (blur <= 0.0f) {
    return;
  }

  // float sigma = GraphicsContext::ConvertRadiusToSigma(blur);

  // In <image> widget May be not necessary to convert blur_radius
  // to sigma, or the sigma value is apparently smaller than wanted.
  // This is probably related to the skia api behavior.

  auto blur_filter =
      std::make_shared<BlurImageFilter>(blur, blur, TileMode::kClamp);

  paint->setImageFilter(blur_filter);
}

void GraphicsContext::ApplyDropShadowEffect(Paint* paint,
                                            const ImageData& image_data) {
  if (image_data.drop_shadow_blur_radius == 0.0f &&
      image_data.drop_shadow_offset_x == 0.0f &&
      image_data.drop_shadow_offset_y == 0.0f) {
    return;
  }

  auto image_filter = std::make_shared<DropShadowImageFilter>(
      image_data.drop_shadow_offset_x, image_data.drop_shadow_offset_y,
      image_data.drop_shadow_blur_radius, image_data.drop_shadow_blur_radius,
      image_data.drop_shadow_color);

  paint->setImageFilter(image_filter);
}

// TODO(yudingqian): Decouple incompletely
bool GraphicsContext::DrawImageWithDropShadow(
    fml::RefPtr<GraphicsImage> image, const ImageData& image_data,
    const skity::Rect& src_rect, const skity::Rect& dst_rect,
    const skity::Rect& padding_rect, float padding_left, float padding_top) {
  TRACE_EVENT("clay", "DrawWithDropShadow");
  FML_DCHECK(image);

  if (image_data.drop_shadow_blur_radius == 0.0f &&
      image_data.drop_shadow_offset_x == 0.0f &&
      image_data.drop_shadow_offset_y == 0.0f) {
    return false;
  }

  float sigma =
      GraphicsContext::ConvertRadiusToSigma(image_data.drop_shadow_blur_radius);

  if (sigma < 0.5f) {
    return false;
  }

#ifndef ENABLE_SKITY
  // Resize the image before drop shadow on it.
  auto resize_filter =
      std::make_shared<UnknownImageFilter>(SkImageFilters::Image(
          image->gr_image(), ConvertSkityRectToSkRect(src_rect),
          ConvertSkityRectToSkRect(dst_rect),
          SAMPLING_OPTIONS(FilterMode::kLinear, 0)));
  SkIRect out_image_rect = SkIRect::MakeEmpty();
  SkIPoint out_image_offset = SkIPoint::Make(0, 0);
  auto src_round = src_rect;
  src_round.Round();
  auto dst_round = dst_rect;
  dst_round.Round();
  fml::RefPtr<GraphicsImage> resized_img = image->makeWithFilter(
      Canvas()->RecordingContext(), resize_filter.get(),
      ConvertSkityRectToSkIRect(src_round),
      ConvertSkityRectToSkIRect(dst_round), &out_image_rect, &out_image_offset);

  if (!resized_img) {
    return false;
  }

  // Drop Shadow on image by shadow_filter.
  auto shadow_filter = std::make_shared<DropShadowImageFilter>(
      image_data.drop_shadow_offset_x, image_data.drop_shadow_offset_y, sigma,
      sigma, image_data.drop_shadow_color);

  // We don't know how large the shadow-image will be, just tell filter
  // to clip by padding size. It is in the coordinate system of the
  // |resized_img|, where (0,0) corresponds to the |resized_img|'s top left
  // corner, so |padding_clip_x| and |padding_clip_y| is less or equal to zero.
  int32_t padding_clip_x = -(padding_rect.Width() - resized_img->width()) * 0.5;
  int32_t padding_clip_y =
      -(padding_rect.Height() - resized_img->height()) * 0.5;

  FML_DCHECK(padding_clip_x <= 0);
  FML_DCHECK(padding_clip_y <= 0);

  SkIRect padding_clip =
      SkIRect::MakeXYWH(padding_clip_x, padding_clip_y, padding_rect.Width(),
                        padding_rect.Height());
  out_image_rect = SkIRect::MakeEmpty();
  out_image_offset = SkIPoint::Make(0, 0);
  fml::RefPtr<GraphicsImage> shadow_img = resized_img->makeWithFilter(
      Canvas()->RecordingContext(), shadow_filter.get(),
      SkIRect::MakeXYWH(resized_img->bounds().X(), resized_img->bounds().Y(),
                        resized_img->bounds().Width(),
                        resized_img->bounds().Height()),
      padding_clip, &out_image_rect, &out_image_offset);

  if (!shadow_img) {
    return false;
  }

  FML_DCHECK(out_image_rect.width() <= padding_rect.Width());
  FML_DCHECK(out_image_rect.height() <= padding_rect.Height());

  // Calculate the |shadow_img| position, and draw it.
  float dest_x = padding_rect.X() + (out_image_offset.x() - padding_clip_x);
  float dest_y = padding_rect.Y() + (out_image_offset.y() - padding_clip_y);
  // Offset has contained padding left/top, we should remove them.
  Canvas()->Translate(-padding_left, -padding_top);
  Canvas()->DrawImage(shadow_img.get(), dest_x, dest_y);
#endif
  return true;
}

void GraphicsContext::DrawImageRect(const fml::RefPtr<GraphicsImage>& image,
                                    const skity::Rect& src,
                                    const skity::Rect& dst,
                                    const GrSamplingOptions& sampling,
                                    const Paint* paint) {
  Canvas()->DrawImageRect(image, src, dst, sampling, paint);
}

void GraphicsContext::DrawImageRect(const fml::RefPtr<GraphicsImage>& image,
                                    const skity::Rect& dst,
                                    const GrSamplingOptions& sampling,
                                    const Paint* paint) {
  Canvas()->DrawImageRect(image, dst, sampling, paint);
}

void GraphicsContext::DrawLine(Paint stroke_paint, const FloatPoint& point1,
                               const FloatPoint& point2, float thickness,
                               BorderStyleType style) {
  FloatPoint p1 = point1;
  FloatPoint p2 = point2;
  bool is_vertical_line = (p1.x() == p2.x());
  int width = roundf(thickness);

  // We know these are vertical or horizontal lines, so the length will just
  // be the sum of the displacement component vectors give or take 1 -
  // probably worth the speed up of no square root, which also won't be exact.
  FloatPoint delta = p2 - p1;
  FloatSize disp(delta.x(), delta.y());

  if (style == BorderStyleType::kDotted || style == BorderStyleType::kDashed) {
    // Do a rect fill of our endpoints.  This ensures we always have the
    // appearance of being a border.  We then draw the actual dotted/dashed
    // line.
    skity::Rect r1 =
        skity::Rect::MakeLTRB(p1.x(), p1.y(), p1.x() + width, p1.y() + width);
    skity::Rect r2 =
        skity::Rect::MakeLTRB(p2.x(), p2.y(), p2.x() + width, p2.y() + width);

    if (is_vertical_line) {
      r1.Offset(-width / 2, 0);
      r2.Offset(-width / 2, -width);
    } else {
      r1.Offset(0, -width / 2);
      r2.Offset(-width, -width / 2);
    }
    Paint fill_paint;
    fill_paint.setColor(stroke_paint.getColor());
    DrawRect(fill_paint, r1);
    DrawRect(fill_paint, r2);
  }

  AdjustLineToPixelBoundaries(&p1, &p2, width, style);
  Canvas()->DrawLine(p1.x(), p1.y(), p2.x(), p2.y(), stroke_paint);
}

void GraphicsContext::DrawRect(Paint paint, const skity::Rect& rect) {
  if (rect.IsEmpty()) {
    return;
  }

  Canvas()->DrawRect(rect, paint);
}

void GraphicsContext::FillPolygon(size_t num_points, const FloatPoint* points,
                                  const Color& color, bool should_antialias) {
  clay::GrPath path;
  SetPathFromPoints(&path, num_points, points);

  Paint paint;
  paint.setAntiAlias(should_antialias);
  paint.setColor(color);

  Canvas()->DrawPath(path, paint);
}

void GraphicsContext::SetupPaintDashPathEffect(Paint* paint, int length,
                                               float thickness,
                                               BorderStyleType style) {
  float width = thickness;
  switch (style) {
    case BorderStyleType::kNone:
    case BorderStyleType::kSolid:
    case BorderStyleType::kDouble:
      paint->setPathEffect(nullptr);
      return;
    case BorderStyleType::kDashed:
      width = dash_ratio * width;
      // Fall through.
    case BorderStyleType::kDotted: {
      // Truncate the width, since we don't want fuzzy dots or dashes.
      int dash_length = static_cast<int>(width);
      // Subtract off the endcaps, since they're rendered separately.
      int distance = length - 2 * static_cast<int>(thickness);
      int phase = 1;
      if (dash_length > 1) {
        // Determine how many dashes or dots we should have.
        int num_dashes = distance / dash_length;
        int remainder = distance % dash_length;
        // Adjust the phase to center the dashes within the line.
        if (num_dashes % 2) {
          // Odd: shift right a full dash, minus half the remainder.
          phase = dash_length - remainder / 2;
        } else {
          // Even: shift right half a dash, minus half the remainder.
          phase = (dash_length - remainder) / 2;
        }
      }
      float dash_length_sk = dash_length;
      float intervals[2] = {dash_length_sk, dash_length_sk};
      paint->setPathEffect(
          DashPathEffect::Make(intervals, 2, static_cast<float>(phase)));
      return;
    }
    default:
      return;
  }
}

float GraphicsContext::GetPathLength(const GrPath& path) {
  float length = 0;
  GrPathMeasure measure(path, false);

  do {
    length += PATH_MEASURE_GET_LENGTH(measure);
  } while (PATH_MEASURE_NEXT_CONTOUR(measure));

  return length;
}

void GraphicsContext::SetPathFromPoints(clay::GrPath* path, size_t num_points,
                                        const FloatPoint* points) {
#ifndef ENABLE_SKITY
  path->incReserve(num_points);
#else
  // TODO(zhangxiao.ninja) should skity support incReserve function?
  //  path->incReserve(num_points);
  FML_UNIMPLEMENTED();
#endif  // ENABLE_SKITY
  // path->moveTo(points[0].x(), points[0].y());
  PATH_MOVE_TO((*path), points[0].x(), points[0].y());
  for (size_t i = 1; i < num_points; ++i) {
    PATH_LINE_TO((*path), points[i].x(), points[i].y());
  }
}

void GraphicsContext::AdjustLineToPixelBoundaries(FloatPoint* p1,
                                                  FloatPoint* p2,
                                                  float stroke_width,
                                                  BorderStyleType style) {
  FML_DCHECK(p1 && p2);
  // For odd widths, we add in 0.5 to the appropriate x/y so that the float
  // arithmetic works out.  For example, with a border width of 3, WebKit will
  // pass us (y1+y2)/2, e.g., (50+53)/2 = 103/2 = 51 when we want 51.5.  It is
  // always true that an even width gave us a perfect position, but an odd width
  // gave us a position that is off by exactly 0.5.
  if (style == BorderStyleType::kDotted || style == BorderStyleType::kDashed) {
    if (p1->x() == p2->x()) {
      p1->SetY(p1->y() + stroke_width);
      p2->SetY(p2->y() - stroke_width);
    } else {
      p1->SetX(p1->x() + stroke_width);
      p2->SetX(p2->x() - stroke_width);
    }
  }

  if (static_cast<int>(stroke_width) % 2) {  // odd
    if (p1->x() == p2->x()) {
      // We're a vertical line.  Adjust our x.
      p1->SetX(p1->x() + 0.5f);
      p2->SetX(p2->x() + 0.5f);
    } else {
      // We're a horizontal line. Adjust our y.
      p1->SetY(p1->y() + 0.5f);
      p2->SetY(p2->y() + 0.5f);
    }
  }
}

// Copy from third_party/skia/src/core/SkBlurMask.cpp.
// Converts a blur radius in pixels to sigma.
float GraphicsContext::ConvertRadiusToSigma(float radius) {
  return radius > 0.f ? radius * 0.57735f + 0.5f : 0.f;
}

}  // namespace clay
