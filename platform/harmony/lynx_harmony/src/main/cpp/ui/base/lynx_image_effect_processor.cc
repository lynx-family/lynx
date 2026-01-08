// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/lynx_image_effect_processor.h"

#include <native_drawing/drawing_bitmap.h>
#include <native_drawing/drawing_brush.h>
#include <native_drawing/drawing_canvas.h>
#include <native_drawing/drawing_color.h>
#include <native_drawing/drawing_color_filter.h>
#include <native_drawing/drawing_filter.h>
#include <native_drawing/drawing_image_filter.h>
#include <native_drawing/drawing_pixel_map.h>
#include <native_drawing/drawing_rect.h>
#include <native_drawing/drawing_sampling_options.h>

namespace lynx {
namespace tasm {
namespace harmony {
OH_PixelmapNative* LynxImageEffectProcessor::Process(
    OH_PixelmapNative* pixel_map) const {
  return CustomProcessor(effect_, params_, pixel_map);
}

OH_PixelmapNative* LynxImageEffectProcessor::CustomProcessor(
    const ImageEffect& effect, const EffectParams& params,
    OH_PixelmapNative* pixel_map) const {
  if (effect == ImageEffect::kDropShadow) {
    const auto& shadow_params = std::get<DropShadowParams>(params);
    return ApplyDropShadowToBitmap(shadow_params, pixel_map);
  } else if (effect == ImageEffect::kCapInsets) {
    const auto& cap_insets_params = std::get<CapInsetParams>(params);
    return ApplyCapInsetsToBitmap(cap_insets_params, pixel_map);
  } else {
    return nullptr;
  }
}

OH_PixelmapNative* LynxImageEffectProcessor::ApplyDropShadowToBitmap(
    const LynxImageEffectProcessor::DropShadowParams& shadow_params,
    OH_PixelmapNative* pixel_map) const {
  OH_Pixelmap_ImageInfo* pixel_map_info;
  OH_PixelmapImageInfo_Create(&pixel_map_info);
  OH_PixelmapNative_GetImageInfo(pixel_map, pixel_map_info);
  uint32_t width{0};
  uint32_t height{0};
  OH_PixelmapImageInfo_GetWidth(pixel_map_info, &width);
  OH_PixelmapImageInfo_GetHeight(pixel_map_info, &height);
  OH_PixelmapImageInfo_Release(pixel_map_info);

  float radius{0.f};
  float offset_x{0.f};
  float offset_y{0.f};
  uint32_t color{0};
  const auto& common_params = shadow_params.common_params;
  float scale_density = common_params.scale_density;
  radius = shadow_params.radius * scale_density;
  offset_x = shadow_params.offset_x * scale_density;
  offset_y = shadow_params.offset_y * scale_density;
  color = shadow_params.color;
  float view_width = common_params.view_width * scale_density;
  float view_height = common_params.view_height * scale_density;
  float padding_left = common_params.padding_left * scale_density;
  float padding_right = common_params.padding_right * scale_density;
  float padding_top = common_params.padding_top * scale_density;
  float padding_bottom = common_params.padding_bottom * scale_density;

  uint32_t dst_width = view_width;
  uint32_t dst_height = view_height;

  OH_Drawing_Bitmap* bitmap = OH_Drawing_BitmapCreate();
  OH_Drawing_BitmapFormat format{COLOR_FORMAT_BGRA_8888, ALPHA_FORMAT_PREMUL};
  OH_Drawing_BitmapBuild(bitmap, dst_width, dst_height, &format);
  OH_Drawing_Canvas* bitmap_canvas = OH_Drawing_CanvasCreate();
  OH_Drawing_CanvasBind(bitmap_canvas, bitmap);
  OH_Drawing_CanvasTranslate(bitmap_canvas, padding_left, padding_top);
  OH_Drawing_CanvasClear(bitmap_canvas, 0);
  OH_Drawing_SamplingOptions* sample_options =
      OH_Drawing_SamplingOptionsCreate(FILTER_MODE_LINEAR, MIPMAP_MODE_LINEAR);
  float available_width = view_width - padding_left - padding_right;
  float available_height = view_height - padding_top - padding_bottom;
  OH_Drawing_Rect* dst_rect =
      OH_Drawing_RectCreate(0, 0, available_width, available_height);
  OH_Drawing_Rect* src_rect = OH_Drawing_RectCreate(0, 0, width, height);

  OH_Drawing_PixelMap* draw_pixel =
      OH_Drawing_PixelMapGetFromOhPixelMapNative(pixel_map);
  OH_Drawing_Brush* brush = OH_Drawing_BrushCreate();
  OH_Drawing_BrushSetAntiAlias(brush, true);
  OH_Drawing_ColorFilter* color_filter = OH_Drawing_ColorFilterCreateBlendMode(
      static_cast<uint32_t>(color), BLEND_MODE_SRC_IN);

  OH_Drawing_Filter* draw_filter = OH_Drawing_FilterCreate();
  OH_Drawing_ImageFilter* image_filter = OH_Drawing_ImageFilterCreateBlur(
      radius, radius, OH_Drawing_TileMode::CLAMP, nullptr);

  OH_Drawing_FilterSetImageFilter(draw_filter, image_filter);
  OH_Drawing_FilterSetColorFilter(draw_filter, color_filter);

  OH_Drawing_BrushSetFilter(brush, draw_filter);

  OH_Drawing_CanvasSave(bitmap_canvas);

  OH_Drawing_CanvasTranslate(bitmap_canvas, offset_x, offset_y);
  OH_Drawing_CanvasAttachBrush(bitmap_canvas, brush);

  OH_Drawing_CanvasDrawPixelMapRect(bitmap_canvas, draw_pixel, src_rect,
                                    dst_rect, sample_options);

  OH_Drawing_CanvasDetachBrush(bitmap_canvas);
  OH_Drawing_CanvasRestore(bitmap_canvas);

  OH_Drawing_CanvasDrawPixelMapRect(bitmap_canvas, draw_pixel, src_rect,
                                    dst_rect, sample_options);
  std::unique_ptr<uint8_t[]> new_pixels =
      std::make_unique<uint8_t[]>(dst_width * dst_height * 4);
  OH_Drawing_Image_Info image_info;
  OH_Drawing_BitmapGetImageInfo(bitmap, &image_info);
  OH_Drawing_BitmapReadPixels(bitmap, &image_info, new_pixels.get(),
                              dst_width * 4, 0, 0);

  OH_Pixelmap_InitializationOptions* create_ops = nullptr;
  OH_PixelmapInitializationOptions_Create(&create_ops);
  OH_PixelmapInitializationOptions_SetWidth(create_ops, dst_width);
  OH_PixelmapInitializationOptions_SetHeight(create_ops, dst_height);
  OH_PixelmapInitializationOptions_SetPixelFormat(create_ops,
                                                  PIXEL_FORMAT_BGRA_8888);
  OH_PixelmapInitializationOptions_SetAlphaType(create_ops,
                                                PIXELMAP_ALPHA_TYPE_OPAQUE);
  OH_PixelmapNative* new_pixelmap = nullptr;
  OH_PixelmapNative_CreatePixelmap(new_pixels.get(), dst_width * dst_height * 4,
                                   create_ops, &new_pixelmap);

  OH_Drawing_BitmapDestroy(bitmap);
  OH_Drawing_RectDestroy(src_rect);
  OH_Drawing_RectDestroy(dst_rect);
  OH_Drawing_ColorFilterDestroy(color_filter);
  OH_Drawing_ImageFilterDestroy(image_filter);
  OH_Drawing_FilterDestroy(draw_filter);
  OH_Drawing_CanvasDestroy(bitmap_canvas);
  OH_Drawing_SamplingOptionsDestroy(sample_options);
  OH_Drawing_PixelMapDissolve(draw_pixel);
  OH_Drawing_BrushDestroy(brush);
  OH_PixelmapInitializationOptions_Release(create_ops);
  return new_pixelmap;
}

OH_PixelmapNative* LynxImageEffectProcessor::ApplyCapInsetsToBitmap(
    const LynxImageEffectProcessor::CapInsetParams& params,
    OH_PixelmapNative* pixel_map) const {
  OH_Pixelmap_ImageInfo* pixel_map_info;
  OH_PixelmapImageInfo_Create(&pixel_map_info);
  OH_PixelmapNative_GetImageInfo(pixel_map, pixel_map_info);
  uint32_t source_width{0};
  uint32_t source_height{0};
  OH_PixelmapImageInfo_GetWidth(pixel_map_info, &source_width);
  OH_PixelmapImageInfo_GetHeight(pixel_map_info, &source_height);
  OH_PixelmapImageInfo_Release(pixel_map_info);

  const auto& common_params = params.common_params;

  float scale_density = common_params.scale_density;
  float view_width = common_params.view_width * scale_density;
  float view_height = common_params.view_height * scale_density;
  float padding_left = common_params.padding_left * scale_density;
  float padding_right = common_params.padding_right * scale_density;
  float padding_top = common_params.padding_top * scale_density;
  float padding_bottom = common_params.padding_bottom * scale_density;
  float cap_inset_scale = params.cap_inset_scale;

  float cap_inset_left = params.cap_inset_left * cap_inset_scale;
  float cap_inset_right = params.cap_inset_right * cap_inset_scale;
  float cap_inset_top = params.cap_inset_top * cap_inset_scale;
  float cap_inset_bottom = params.cap_inset_bottom * cap_inset_scale;

  uint32_t dst_width = view_width;
  uint32_t dst_height = view_height;

  OH_Drawing_Bitmap* bitmap = OH_Drawing_BitmapCreate();
  OH_Drawing_BitmapFormat format{COLOR_FORMAT_BGRA_8888, ALPHA_FORMAT_PREMUL};
  OH_Drawing_BitmapBuild(bitmap, dst_width, dst_height, &format);
  OH_Drawing_Canvas* bitmap_canvas = OH_Drawing_CanvasCreate();
  OH_Drawing_CanvasBind(bitmap_canvas, bitmap);

  float available_width = view_width - padding_left - padding_right;

  float available_height = view_height - padding_top - padding_bottom;

  OH_Drawing_Rect* src_0 =
      OH_Drawing_RectCreate(0, 0, cap_inset_left, cap_inset_top);

  OH_Drawing_Rect* src_1 = OH_Drawing_RectCreate(
      cap_inset_left, 0, source_width - cap_inset_right, cap_inset_top);

  OH_Drawing_Rect* src_2 = OH_Drawing_RectCreate(
      source_width - cap_inset_right, 0, source_width, cap_inset_top);

  OH_Drawing_Rect* src_3 = OH_Drawing_RectCreate(
      0, cap_inset_top, cap_inset_left, source_height - cap_inset_bottom);

  OH_Drawing_Rect* src_4 = OH_Drawing_RectCreate(
      cap_inset_left, cap_inset_top, source_width - cap_inset_right,
      source_height - cap_inset_bottom);

  OH_Drawing_Rect* src_5 =
      OH_Drawing_RectCreate(source_width - cap_inset_right, cap_inset_top,
                            source_width, source_height - cap_inset_bottom);

  OH_Drawing_Rect* src_6 = OH_Drawing_RectCreate(
      0, source_height - cap_inset_bottom, cap_inset_left, source_height);

  OH_Drawing_Rect* src_7 =
      OH_Drawing_RectCreate(cap_inset_left, source_height - cap_inset_bottom,
                            source_width - cap_inset_right, source_height);

  OH_Drawing_Rect* src_8 = OH_Drawing_RectCreate(
      source_width - cap_inset_right, source_height - cap_inset_bottom,
      source_width, source_height);

  float final_height = available_height;

  float final_width = available_width;

  float start_w = padding_left;

  float start_h = padding_top;

  float cap_inset_left_px = params.cap_inset_left * scale_density;

  float cap_inset_top_px = params.cap_inset_top * scale_density;

  float cap_inset_right_px = params.cap_inset_right * scale_density;

  float cap_inset_bottom_px = params.cap_inset_bottom * scale_density;

  OH_Drawing_Rect* dest_0 =
      OH_Drawing_RectCreate(start_w, start_h, start_w + cap_inset_left_px,
                            start_h + cap_inset_top_px);

  OH_Drawing_Rect* dest_1 = OH_Drawing_RectCreate(
      start_w + cap_inset_left_px, start_h,
      start_w + final_width - cap_inset_right_px, start_h + cap_inset_top_px);

  OH_Drawing_Rect* dest_2 =
      OH_Drawing_RectCreate(start_w + final_width - cap_inset_right_px, start_h,
                            start_w + final_width, start_h + cap_inset_top_px);

  OH_Drawing_Rect* dest_3 = OH_Drawing_RectCreate(
      start_w, start_h + cap_inset_top_px, start_w + cap_inset_left_px,
      start_h + final_height - cap_inset_bottom_px);

  OH_Drawing_Rect* dest_4 = OH_Drawing_RectCreate(
      start_w + cap_inset_left_px, start_h + cap_inset_top_px,
      start_w + final_width - cap_inset_right_px,
      start_h + final_height - cap_inset_bottom_px);

  OH_Drawing_Rect* dest_5 = OH_Drawing_RectCreate(
      start_w + final_width - cap_inset_right_px, start_h + cap_inset_top_px,
      start_w + final_width, start_h + final_height - cap_inset_bottom_px);

  OH_Drawing_Rect* dest_6 = OH_Drawing_RectCreate(
      start_w, start_h + final_height - cap_inset_bottom_px,
      start_w + cap_inset_left_px, start_h + final_height);

  OH_Drawing_Rect* dest_7 = OH_Drawing_RectCreate(
      start_w + cap_inset_left_px, start_h + final_height - cap_inset_bottom_px,
      start_w + final_width - cap_inset_right_px, start_h + final_height);

  OH_Drawing_Rect* dest_8 =
      OH_Drawing_RectCreate(start_w + final_width - cap_inset_right_px,
                            start_h + final_height - cap_inset_bottom_px,
                            start_w + final_width, start_h + final_height);

  OH_Drawing_SamplingOptions* sample_options =
      OH_Drawing_SamplingOptionsCreate(FILTER_MODE_LINEAR, MIPMAP_MODE_LINEAR);

  OH_Drawing_PixelMap* draw_pixel =
      OH_Drawing_PixelMapGetFromOhPixelMapNative(pixel_map);

  OH_Drawing_Brush* brush = OH_Drawing_BrushCreate();
  OH_Drawing_BrushSetAntiAlias(brush, false);
  OH_Drawing_CanvasAttachBrush(bitmap_canvas, brush);

  OH_Drawing_CanvasDrawPixelMapRect(bitmap_canvas, draw_pixel, src_0, dest_0,
                                    sample_options);
  OH_Drawing_CanvasDrawPixelMapRect(bitmap_canvas, draw_pixel, src_1, dest_1,
                                    sample_options);
  OH_Drawing_CanvasDrawPixelMapRect(bitmap_canvas, draw_pixel, src_2, dest_2,
                                    sample_options);
  OH_Drawing_CanvasDrawPixelMapRect(bitmap_canvas, draw_pixel, src_3, dest_3,
                                    sample_options);
  OH_Drawing_CanvasDrawPixelMapRect(bitmap_canvas, draw_pixel, src_4, dest_4,
                                    sample_options);
  OH_Drawing_CanvasDrawPixelMapRect(bitmap_canvas, draw_pixel, src_5, dest_5,
                                    sample_options);
  OH_Drawing_CanvasDrawPixelMapRect(bitmap_canvas, draw_pixel, src_6, dest_6,
                                    sample_options);
  OH_Drawing_CanvasDrawPixelMapRect(bitmap_canvas, draw_pixel, src_7, dest_7,
                                    sample_options);
  OH_Drawing_CanvasDrawPixelMapRect(bitmap_canvas, draw_pixel, src_8, dest_8,
                                    sample_options);

  std::unique_ptr<uint8_t[]> new_pixels =
      std::make_unique<uint8_t[]>(dst_width * dst_height * 4);
  OH_Drawing_Image_Info image_info;
  OH_Drawing_BitmapGetImageInfo(bitmap, &image_info);
  OH_Drawing_BitmapReadPixels(bitmap, &image_info, new_pixels.get(),
                              dst_width * 4, 0, 0);

  OH_Pixelmap_InitializationOptions* create_ops = nullptr;
  OH_PixelmapInitializationOptions_Create(&create_ops);
  OH_PixelmapInitializationOptions_SetWidth(create_ops, dst_width);
  OH_PixelmapInitializationOptions_SetHeight(create_ops, dst_height);
  OH_PixelmapInitializationOptions_SetPixelFormat(create_ops,
                                                  PIXEL_FORMAT_BGRA_8888);
  OH_PixelmapInitializationOptions_SetAlphaType(create_ops,
                                                PIXELMAP_ALPHA_TYPE_OPAQUE);
  OH_PixelmapNative* new_pixelmap = nullptr;
  OH_PixelmapNative_CreatePixelmap(new_pixels.get(), dst_width * dst_height * 4,
                                   create_ops, &new_pixelmap);

  OH_Drawing_RectDestroy(src_0);
  OH_Drawing_RectDestroy(src_1);
  OH_Drawing_RectDestroy(src_2);
  OH_Drawing_RectDestroy(src_3);
  OH_Drawing_RectDestroy(src_4);
  OH_Drawing_RectDestroy(src_5);
  OH_Drawing_RectDestroy(src_6);
  OH_Drawing_RectDestroy(src_7);
  OH_Drawing_RectDestroy(src_8);
  OH_Drawing_RectDestroy(dest_0);
  OH_Drawing_RectDestroy(dest_1);
  OH_Drawing_RectDestroy(dest_2);
  OH_Drawing_RectDestroy(dest_3);
  OH_Drawing_RectDestroy(dest_4);
  OH_Drawing_RectDestroy(dest_5);
  OH_Drawing_RectDestroy(dest_6);
  OH_Drawing_RectDestroy(dest_7);
  OH_Drawing_RectDestroy(dest_8);

  OH_Drawing_BitmapDestroy(bitmap);
  OH_Drawing_CanvasDestroy(bitmap_canvas);
  OH_Drawing_SamplingOptionsDestroy(sample_options);
  OH_Drawing_PixelMapDissolve(draw_pixel);
  OH_Drawing_BrushDestroy(brush);
  OH_PixelmapInitializationOptions_Release(create_ops);

  return new_pixelmap;
}
}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
