// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_RENDERING_BACKEND_H_
#define CLAY_GFX_RENDERING_BACKEND_H_

#include <memory>

#ifndef ENABLE_SKITY
#ifdef ENABLE_SVG
#include "third_party/skia/modules/svg/include/SkSVGDOM.h"
#include "third_party/skia/modules/svg/include/SkSVGRenderContext.h"
#include "third_party/skia/modules/svg/include/SkSVGSVG.h"
#endif  // ENABLE_SVG
#include "clay/flow/rtree.h"
#include "clay/gfx/skia/skia_concurrent_executor.h"
#include "clay/gfx/skity_to_skia_utils.h"
#include "third_party/skia/include/codec/SkEncodedImageFormat.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkBlendMode.h"
#include "third_party/skia/include/core/SkBlurTypes.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkColor.h"
#include "third_party/skia/include/core/SkColorFilter.h"
#include "third_party/skia/include/core/SkFont.h"
#include "third_party/skia/include/core/SkFontMgr.h"
#include "third_party/skia/include/core/SkGraphics.h"
#include "third_party/skia/include/core/SkImage.h"
#include "third_party/skia/include/core/SkImageInfo.h"
#include "third_party/skia/include/core/SkMaskFilter.h"
#include "third_party/skia/include/core/SkPaint.h"
#include "third_party/skia/include/core/SkPath.h"
#include "third_party/skia/include/core/SkPathEffect.h"
#include "third_party/skia/include/core/SkPathMeasure.h"
#include "third_party/skia/include/core/SkPathTypes.h"
#include "third_party/skia/include/core/SkPictureRecorder.h"
#include "third_party/skia/include/core/SkPixmap.h"
#include "third_party/skia/include/core/SkPoint.h"
#include "third_party/skia/include/core/SkPoint3.h"
#include "third_party/skia/include/core/SkPromiseImageTexture.h"
#include "third_party/skia/include/core/SkRRect.h"
#include "third_party/skia/include/core/SkSamplingOptions.h"
#include "third_party/skia/include/core/SkSerialProcs.h"
#include "third_party/skia/include/core/SkShader.h"
#include "third_party/skia/include/core/SkStream.h"
#include "third_party/skia/include/core/SkSurface.h"
#include "third_party/skia/include/core/SkTypeface.h"
#include "third_party/skia/include/core/SkVertices.h"
#include "third_party/skia/include/effects/SkDashPathEffect.h"
#include "third_party/skia/include/effects/SkGradientShader.h"
#include "third_party/skia/include/effects/SkImageFilters.h"
#include "third_party/skia/include/effects/SkRuntimeEffect.h"
#include "third_party/skia/include/encode/SkJpegEncoder.h"
#include "third_party/skia/include/encode/SkPngEncoder.h"
#include "third_party/skia/include/gpu/GrBackendSurface.h"
#include "third_party/skia/include/gpu/GrContextOptions.h"
#include "third_party/skia/include/gpu/GrContextThreadSafeProxy.h"
#include "third_party/skia/include/gpu/GrDirectContext.h"
#include "third_party/skia/include/gpu/ganesh/SkImageGanesh.h"
#include "third_party/skia/include/gpu/gl/GrGLInterface.h"
#include "third_party/skia/include/pathops/SkPathOps.h"
#include "third_party/skia/include/utils/SkShadowUtils.h"
#include "third_party/skia/src/gpu/ganesh/GrBackendUtils.h"
#else
#include "clay/gfx/skity/jpeg_codec_skity.h"
#include "clay/gfx/skity/skity_auto_canvas_save.h"
#include "clay/gfx/skity/skity_lazy_image_traveller.h"
#include "skity/effect/color_filter.hpp"
#include "skity/effect/image_filter.hpp"
#include "skity/effect/mask_filter.hpp"
#include "skity/effect/path_effect.hpp"
#include "skity/effect/shader.hpp"
#include "skity/geometry/point.hpp"
#include "skity/gpu/gpu_context.hpp"
#include "skity/gpu/gpu_render_target.hpp"
#include "skity/gpu/gpu_surface.hpp"
#include "skity/graphic/bitmap.hpp"
#include "skity/graphic/blend_mode.hpp"
#include "skity/graphic/color.hpp"
#include "skity/graphic/image.hpp"
#include "skity/graphic/paint.hpp"
#include "skity/graphic/path.hpp"
#include "skity/graphic/path_measure.hpp"
#include "skity/graphic/path_op.hpp"
#include "skity/graphic/sampling_options.hpp"
#include "skity/include/skity/io/data.hpp"
#include "skity/include/skity/text/font_manager.hpp"
#include "skity/recorder/picture_recorder.hpp"
#include "skity/render/canvas.hpp"
#include "skity/text/font.hpp"
#include "skity/text/text_blob.hpp"
#endif  // ENABLE_SKITY

namespace clay {

#ifndef ENABLE_SKITY
// PROTOTYPES
using GrAutoCanvasRestore = SkAutoCanvasRestore;
using GrBitmap = SkBitmap;
using GrContext = GrDirectContext;
using GrContextPtr = sk_sp<GrDirectContext>;
using GrCanvas = SkCanvas;
using GrPaint = SkPaint;
using GrSurface = SkSurface;
using GrSurfacePtr = sk_sp<SkSurface>;
using GrPath = SkPath;
using GrImage = SkImage;
using GrImagePtr = sk_sp<SkImage>;
using GrSamplingOptions = SkSamplingOptions;
using GrShaderPtr = sk_sp<SkShader>;
using GrColor = SkColor;
using GrColorFilterPtr = sk_sp<SkColorFilter>;
using GrPoint = SkPoint;
using GrTextBlobPtr = sk_sp<SkTextBlob>;
using GrGpuBackendType = GrBackendApi;
using GrImageInfo = SkImageInfo;
#ifdef ENABLE_SVG
using SVGDomPtr = sk_sp<SkSVGDOM>;
#endif  // ENABLE_SVG
using GrColorFilter = SkColorFilter;
using GrColorFilterPtr = sk_sp<SkColorFilter>;
using GrColorFilters = SkColorFilters;
using GrShader = SkShader;
using GrShaderPtr = sk_sp<SkShader>;
using GrImageFilter = SkImageFilter;
using GrImageFilterPtr = sk_sp<SkImageFilter>;
using GrImageFilters = SkImageFilters;
using GrMaskFilter = SkMaskFilter;
using GrMaskFilterPtr = sk_sp<SkMaskFilter>;
using GrPathEffect = SkPathEffect;
using GrPathEffectPtr = sk_sp<SkPathEffect>;
using GrData = SkData;
using GrDataPtr = sk_sp<SkData>;
using GrClipOp = SkClipOp;
using GrPathMeasure = SkPathMeasure;
using GrPathDirection = SkPathDirection;
using GrFontMgr = SkFontMgr;
using GrRect = SkRect;
using GrBlurStyle = SkBlurStyle;
using GrPicturePtr = sk_sp<SkPicture>;

// FUNCTIONS
// GrContext

// GrSurface
#define SURFACE_GET_CANVAS(surface, clear) surface->getCanvas()
#define SURFACE_GET_WIDTH(surface) surface->width()
#define SURFACE_GET_HEIGHT(surface) surface->height()

// GrCanvas
#define CANVAS_SAVE(canvas) canvas->save()
#define CANVAS_FLUSH(canvas) canvas->flush()
#define CANVAS_RESTORE(canvas) canvas->restore()
#define CANVAS_AUTO_RESTORE(canvas, do_save) \
  SkAutoCanvasRestore auto_restore(canvas, do_save)
#define CANVAS_RESET_MATRIX(canvas) canvas->resetMatrix()
#define CANVAS_DRAW_RECT(canvas, rect, paint) \
  canvas->drawRect(clay::ConvertSkityRectToSkRect(rect), paint)
#define CANVAS_DRAW_PATH(canvas, path, paint) canvas->drawPath(path, paint)
#define CANVAS_CLIP_RECT(canvas, rect) \
  canvas->clipRect(clay::ConvertSkityRectToSkRect(rect))
#define CANVAS_CLIP_RECT_WITH_OP(canvas, rect, op) \
  canvas->clipRect(clay::ConvertSkityRectToSkRect(rect), op)
#define CANVAS_DRAW_PAINT(canvas, paint) canvas->drawPaint(paint)
#define CANVAS_DRAW_IMAGE(canvas, image, bound, sampling_options, paint) \
  canvas->drawImage(image, bound.Left(), bound.Top(), sampling_options, paint)
#define CANVAS_DRAW_IMAGE_RECT(canvas, image, rect, sampling_options, paint) \
  canvas->drawImageRect(image, clay::ConvertSkityRectToSkRect(rect),         \
                        sampling_options, paint)
#define CANVAS_DRAW_IMAGE_SRC_RECT(canvas, image, src_rect, dst_rect,    \
                                   sampling_options, paint)              \
  canvas->drawImageRect(image, clay::ConvertSkityRectToSkRect(src_rect), \
                        clay::ConvertSkityRectToSkRect(dst_rect),        \
                        sampling_options, &paint,                        \
                        SkCanvas::kFast_SrcRectConstraint);
#define CANVAS_TRANSLATE(canvas, left, top) canvas->translate(left, top)
#define CANVAS_GET_TOTAL_MATRIX(canvas) \
  clay::ConvertSkMatrixToSkityMatrix(canvas->getTotalMatrix())
#define CANVAS_CLEAR(canvas, color) canvas->clear(clay::ToSk(color))
#define CANVAS_SCALE(canvas, x, y) canvas->scale(x, y)
#define CANVAS_CONCAT(canvas, matrix) \
  canvas->concat(clay::ConvertSkityMatrixToSkMatrix(matrix))
#define CANVAS_SAVELAYER(canvas, bounds, paint) \
  canvas->saveLayer(clay::ConvertSkityRectToSkRect(bounds), &paint)
#define CANVAS_DRAW_TEXTBLOB(blob, x, y, paint) \
  canvas->drawTextBlob(blob, x, y, paint)
#define CANVAS_GET_SAVE_COUNT(canvas) canvas->getSaveCount()
#define CANVAS_RESTORE_TO_COUNT(canvas, count) canvas->restoreToCount(count)

// GrPaint
#define PAINT_SET_COLOR(paint, color) paint.setColor(color)
#define PAINT_SET_STYLE(paint, style) \
  paint.setStyle(static_cast<SkPaint::Style>(style))
#define PAINT_SET_BLEND_MODE(paint, blend_mode) \
  paint.setBlendMode(ToSk(blend_mode))
#define PAINT_SET_STROKE_WIDTH(paint, width) paint.setStrokeWidth(width)
#define PAINT_SET_SHADER(paint, shader) paint.setShader(shader)
#define PAINT_SET_IMAGE_FILTER(paint, image_filter) \
  paint.setImageFilter(image_filter)
#define PAINT_SET_ALPHAF(paint, alphaf) paint.setAlphaf(alphaf)
#define PAINT_SET_COLOR_FILTER(paint, color_filter) \
  paint.setColorFilter(color_filter)
#define PAINT_SET_MASK_FILTER(paint, mask_filter) \
  paint.setMaskFilter(mask_filter)
#define PAINT_SET_ANTI_ALIAS(paint, aa) paint.setAntiAlias(aa)
#define PAINT_SET_STROKE_CAP(paint, cap) paint.setStrokeCap(cap)
#define PAINT_SET_STROKE_JOIN(paint, join) paint.setStrokeJoin(join)
#define PAINT_SET_STROKE_MITER(paint, miter) paint.setStrokeMiter(miter)
#define PAINT_SET_STROKE_WIDTH(paint, width) paint.setStrokeWidth(width)
#define PAINT_SET_PATH_EFFECT(paint, path_effect) \
  paint.setPathEffect(path_effect)
#define PAINT_CAN_COMPUTE_FAST_BOUNDS(paint) paint.canComputeFastBounds()
#define PAINT_COMPUTE_FAST_BOUNDS(paint, orig, storage) \
  paint.computeFastBounds(orig, &storage)

// GrPath
#define PATH_ADD_RECT(path, rect) \
  path.addRect(clay::ConvertSkityRectToSkRect(rect))
#define PATH_ADD_RRECT(path, round_rect) \
  path.addRRect(clay::ConvertSkityRRectToSkia(round_rect))
#define PATH_ADD_PATH(path, drawing_path) path.addPath(drawing_path)
#define PATH_IS_EMPTY(path) path.isEmpty()
#define PATH_IS_RECT(path, rect) path.isRect(rect)
#define PATH_SET_FILL_TYPE(path, fill_type) \
  path.setFillType(static_cast<SkPathFillType>(fill_type))
#define PATH_OP(hole_path, inner_path, path_op, intersect_path) \
  Op(hole_path, inner_path, static_cast<SkPathOp>(path_op), &intersect_path)
#define PATH_MOVE_TO(path, x, y) path.moveTo(x, y)
#define PATH_MOVE_TO_POINT(path, point) path.moveTo(point)
#define PATH_LINE_TO(path, x, y) path.lineTo(x, y)
#define PATH_LINE_TO_POINT(path, point) path.lineTo(point)
#define PATH_RLINE_TO(path, x, y) path.rLineTo(x, y)
#define PATH_CLOSE(path) path.close()
#define PATH_GET_BOUNDS(path) path.getBounds()
#define PATH_TRANSFORM(path, matrix) \
  path.transform(clay::ConvertSkityMatrixToSkMatrix(matrix))
#define PATH_CUBIC_TO_POINT(path, point1, point2, point3) \
  path.cubicTo(point1, point2, point3)
#define PATH_QUAD_TO_POINT(path, point1, point2) path.quadTo(point1, point2)
#define PATH_ARC_TO(path, point1, axis, arc, sweep, point2) \
  path.arcTo(point1, axis, arc, sweep, point2)
#define PATH_GET_LAST_POINT(path, point) path.getLastPt(point)
#define PATH_SWAP(path, other) path->swap(other)

// GrPathMeasure
#define PATH_MEASURE_GET_LENGTH(measure) measure.getLength()
#define PATH_MEASURE_NEXT_CONTOUR(measure) measure.nextContour()

// GrImage
#define IMAGE_WIDTH(image) image->width()
#define IMAGE_HEIGHT(image) image->height()
#define IMAGE_BYTE_SIZE(image) image->imageInfo().computeMinByteSize()

// GrSamplingOptions
#define SAMPLING_OPTIONS(filter_mode, mipmap_mode)          \
  SkSamplingOptions(static_cast<SkFilterMode>(filter_mode), \
                    static_cast<SkMipmapMode>(mipmap_mode))

// GrColorFilter
#define COLOR_FILTER_MATRIX(m) SkColorFilters::Matrix(m)
#define COLOR_FILTER_FILTER_COLOR(filter, color) \
  filter->filterColor(ToSk(color))

// GrImageFilter
#define IMAGE_FILTERS_BLUR(sigma_x, sigma_y, tile_mode, input) \
  SkImageFilters::Blur(sigma_x, sigma_y, tile_mode, input)
#define IMAGE_FILTERS_DILATE(radius_x, radius_y, input) \
  SkImageFilters::Dilate(radius_x, radius_y, input)
#define IMAGE_FILTERS_ERODE(radius_x, radius_y, input) \
  SkImageFilters::Erode(radius_x, radius_y, input)
#define IMAGE_FILTERS_MATRIX_TRANSFORM(matrix, sampling, input)         \
  SkImageFilters::MatrixTransform(ConvertSkityMatrixToSkMatrix(matrix), \
                                  sampling, input)
#define IMAGE_FILTERS_COLOR_FILTER(color_filter, input) \
  SkImageFilters::ColorFilter(color_filter, input)

// GrRect
#define RECT_ASSIGN(dst_rect, src_rect) \
  dst_rect = clay::ConvertSkRectToSkityRect(src_rect)
#define RECT_IS_SORTED(rect) rect.isSorted()

// GrImageInfo
#define IMAGE_INFO_MAKE_WH(width, height) \
  SkImageInfo::MakeN32Premul(width, height)
#define IMAGE_INFO_MAKE_DIMENSIONS(image_info, dimensions) \
  image_info.makeDimensions(SkISize::Make((int)dimensions.x, (int)dimensions.y))

// GrShader
#define SHADER_IS_OPAQUE(shader) shader->isOpaque()

// GrPathEffect
#define PATH_EFFECT_MAKE_DASH(intervals, count, phase) \
  SkDashPathEffect::Make(intervals, count, phase)

// GrFontMgr
#define FONT_MANAGER_MATCH_FAMILY(mgr_ptr, family_name) \
  mgr_ptr->matchFamily(family_name)

// GrFontStyleSet
#define FONT_STYLE_SET_COUNT(style_set) style_set->count()

// GrPoint
#define POINT_GET_X(point) point.fX
#define POINT_GET_Y(point) point.fY
#define POINT_SET_X(point, value) point.fX = value
#define POINT_SET_Y(point, value) point.fY = value

// GrBlurStyle
#define BLUR_STYLE_NORMAL SkBlurStyle::kNormal_SkBlurStyle
#define BLUR_STYLE_SOLID SkBlurStyle::kSolid_SkBlurStyle
#define BLUR_STYLE_OUTER SkBlurStyle::kOuter_SkBlurStyle
#define BLUR_STYLE_INNER SkBlurStyle::kInner_SkBlurStyle

// GrData
#define DATA_GET_DATA(pdata) pdata->data()
#define DATA_GET_SIZE(data) data->size()
#define DATA_GET_WRITABLE_DATA(data) data->writable_data()
#define DATA_GET_BYTES(data) data->bytes()
#define DATA_IS_EMPTY(data) data->isEmpty()

#else
// PROTOTYPES
using GrAutoCanvasRestore = SkityAutoCanvasRestore;
using GrBitmap = skity::Bitmap;
using GrContext = skity::GPUContext;
using GrContextPtr = std::shared_ptr<skity::GPUContext>;
using GrCanvas = skity::Canvas;
using GrPaint = skity::Paint;
using GrSurface = skity::GPUSurface;
using GrSurfacePtr = std::shared_ptr<skity::GPUSurface>;
using GrPath = skity::Path;
using GrImage = skity::Image;
using GrImagePtr = std::shared_ptr<skity::Image>;
using GrSamplingOptions = skity::SamplingOptions;
using GrShaderPtr = std::shared_ptr<skity::Shader>;
using GrColor = skity::Color;
using GrColorFilterPtr = std::shared_ptr<skity::ColorFilter>;
using GrPoint = skity::Point;
using GrTextBlobPtr = std::shared_ptr<skity::TextBlob>;
using GrGpuBackendType = skity::GPUBackendType;
using GrImageInfo = struct ImageInfo;
#ifdef ENABLE_SVG
using SVGDomPtr = std::shared_ptr<class SVGDom>;
#endif  // ENABLE_SVG
using GrColorFilter = skity::ColorFilter;
using GrColorFilterPtr = std::shared_ptr<skity::ColorFilter>;
using GrColorFilters = skity::ColorFilters;
using GrShader = skity::Shader;
using GrShaderPtr = std::shared_ptr<skity::Shader>;
using GrImageFilter = skity::ImageFilter;
using GrImageFilterPtr = std::shared_ptr<skity::ImageFilter>;
using GrImageFilters = skity::ImageFilters;
using GrMaskFilter = skity::MaskFilter;
using GrMaskFilterPtr = std::shared_ptr<skity::MaskFilter>;
using GrPathEffect = skity::PathEffect;
using GrPathEffectPtr = std::shared_ptr<skity::PathEffect>;
using GrData = skity::Data;
using GrDataPtr = std::shared_ptr<skity::Data>;
using GrClipOp = skity::Canvas::ClipOp;
using GrPathMeasure = skity::PathMeasure;
using GrPathDirection = skity::Path::Direction;
using GrFontMgr = skity::FontManager;
using GrRect = skity::Rect;
using GrBlurStyle = skity::BlurStyle;
using GrPicturePtr = std::shared_ptr<skity::DisplayList>;

// FUNCTIONS
// GrContext

// GrSurface
#define SURFACE_GET_CANVAS(surface, clear) surface->LockCanvas(clear)
#define SURFACE_GET_WIDTH(surface) surface->GetWidth()
#define SURFACE_GET_HEIGHT(surface) surface->GetHeight()

// GrCanvas
#define CANVAS_SAVE(canvas) canvas->Save()
#define CANVAS_FLUSH(canvas) canvas->Flush()
#define CANVAS_RESTORE(canvas) canvas->Restore()
#define CANVAS_AUTO_RESTORE(canvas, do_save) \
  clay::SkityAutoCanvasRestore auto_restore(canvas, do_save)
#define CANVAS_RESET_MATRIX(canvas) canvas->ResetMatrix()
#define CANVAS_DRAW_RECT(canvas, rect, paint) canvas->DrawRect(rect, paint)
#define CANVAS_DRAW_PATH(canvas, path, paint) canvas->DrawPath(path, paint)
#define CANVAS_CLIP_RECT(canvas, rect) canvas->ClipRect(rect)
#define CANVAS_CLIP_RECT_WITH_OP(canvas, rect, op) canvas->ClipRect(rect, op)
#define CANVAS_DRAW_PAINT(canvas, paint) canvas->DrawPaint(paint)
#define CANVAS_DRAW_IMAGE(canvas, image, bound, sampling_options, paint) \
  canvas->DrawImage(image, bound, sampling_options, paint)
#define CANVAS_DRAW_IMAGE_RECT(canvas, image, rect, sampling_options, paint) \
  canvas->DrawImage(image, rect, sampling_options, paint)
#define CANVAS_DRAW_IMAGE_SRC_RECT(canvas, image, src_rect, dst_rect, \
                                   sampling_options, paint)           \
  canvas->DrawImageRect(image, src_rect, dst_rect, sampling_options, &paint)
#define CANVAS_TRANSLATE(canvas, left, top) canvas->Translate(left, top)
#define CANVAS_GET_TOTAL_MATRIX(canvas) canvas->GetTotalMatrix()
#define CANVAS_CLEAR(canvas, color) canvas->Clear(clay::ToSk(color))
#define CANVAS_SCALE(canvas, x, y) canvas->Scale(x, y)
#define CANVAS_CONCAT(canvas, matrix) canvas->Concat(matrix)
#define CANVAS_SAVELAYER(canvas, bounds, paint) canvas->SaveLayer(bounds, paint)
#define CANVAS_DRAW_TEXTBLOB(blob, x, y, paint) \
  canvas->DrawTextBlob(blob, x, y, paint)
#define CANVAS_GET_SAVE_COUNT(canvas) canvas->GetSaveCount()
#define CANVAS_RESTORE_TO_COUNT(canvas, count) canvas->RestoreToCount(count)

// GrPaint
#define PAINT_SET_COLOR(paint, color) paint.SetColor(color)
#define PAINT_SET_STYLE(paint, style) \
  paint.SetStyle(static_cast<skity::Paint::Style>(style))
#define PAINT_SET_BLEND_MODE(paint, blend_mode) \
  paint.SetBlendMode(ToSk(blend_mode))
#define PAINT_SET_STROKE_WIDTH(paint, width) paint.SetStrokeWidth(width)
#define PAINT_SET_SHADER(paint, shader) paint.SetShader(shader)
#define PAINT_SET_IMAGE_FILTER(paint, image_filter) \
  paint.SetImageFilter(image_filter)
#define PAINT_SET_ALPHAF(paint, alphaf) paint.SetAlphaF(alphaf)
#define PAINT_SET_COLOR_FILTER(paint, color_filter) \
  paint.SetColorFilter(color_filter)
#define PAINT_SET_MASK_FILTER(paint, mask_filter) \
  paint.SetMaskFilter(mask_filter)
#define PAINT_SET_ANTI_ALIAS(paint, aa) paint.SetAntiAlias(aa)
#define PAINT_SET_STROKE_CAP(paint, cap) paint.SetStrokeCap(cap)
#define PAINT_SET_STROKE_JOIN(paint, join) paint.SetStrokeJoin(join)
#define PAINT_SET_STROKE_MITER(paint, miter) paint.SetStrokeMiter(miter)
#define PAINT_SET_STROKE_WIDTH(paint, width) paint.SetStrokeWidth(width)
#define PAINT_SET_PATH_EFFECT(paint, path_effect) \
  paint.SetPathEffect(path_effect)
#define PAINT_CAN_COMPUTE_FAST_BOUNDS(paint) paint.CanComputeFastBounds()
#define PAINT_COMPUTE_FAST_BOUNDS(paint, orig, storage) \
  paint.ComputeFastBounds(orig)

// GrPath
#define PATH_ADD_RECT(path, rect) path.AddRect(rect)
#define PATH_ADD_RRECT(path, round_rect) path.AddRRect(round_rect)
#define PATH_ADD_PATH(path, drawing_path) path.AddPath(drawing_path)
#define PATH_IS_EMPTY(path) path.IsEmpty()
#define PATH_IS_RECT(path, rect) path.IsRect(rect)
#define PATH_SET_FILL_TYPE(path, fill_type) \
  path.SetFillType(static_cast<skity::Path::PathFillType>(fill_type))
#define PATH_OP(hole_path, inner_path, path_op, intersect_path)   \
  skity::PathOp::Execute(hole_path, inner_path,                   \
                         static_cast<skity::PathOp::Op>(path_op), \
                         &intersect_path)
#define PATH_MOVE_TO(path, x, y) path.MoveTo(x, y)
#define PATH_MOVE_TO_POINT(path, point) path.MoveTo(point)
#define PATH_LINE_TO(path, x, y) path.LineTo(x, y)
#define PATH_LINE_TO_POINT(path, point) path.LineTo(point)
#define PATH_RLINE_TO(path, delta_x, delta_y)                    \
  {                                                              \
    skity::Point last_point;                                     \
    path.GetLastPt(&last_point);                                 \
    path.LineTo(last_point.x + delta_x, last_point.y + delta_y); \
  }
#define PATH_CLOSE(path) path.Close()
#define PATH_GET_BOUNDS(path) path.GetBounds()
#define PATH_TRANSFORM(path, matrix) path = path.CopyWithMatrix(matrix)
#define PATH_CUBIC_TO_POINT(path, point1, point2, point3) \
  path.CubicTo(point1, point2, point3)
#define PATH_QUAD_TO_POINT(path, point1, point2) path.QuadTo(point1, point2)
#define PATH_ARC_TO(path, point1, axis, arc, sweep, point2) \
  path.ArcTo(point1.x, point1.y, axis, arc, sweep, point2.x, point2.y)
#define PATH_GET_LAST_POINT(path, point) path.GetLastPt(point)
#define PATH_SWAP(path, other) path->Swap(other)

// GrPathMeasure
#define PATH_MEASURE_GET_LENGTH(measure) measure.GetLength()
#define PATH_MEASURE_NEXT_CONTOUR(measure) measure.NextContour()

// GrImage
#define IMAGE_WIDTH(image) image->Width()
#define IMAGE_HEIGHT(image) image->Height()
#define IMAGE_BYTE_SIZE(image) image->Width() * image->Height() * 4

// GrSamplingOptions
#define SAMPLING_OPTIONS(filter_mode, mipmap_mode)                    \
  skity::SamplingOptions(static_cast<skity::FilterMode>(filter_mode), \
                         static_cast<skity::MipmapMode>(mipmap_mode))

// GrColorFilter
#define COLOR_FILTER_MATRIX(m) skity::ColorFilters::Matrix(m)
#define COLOR_FILTER_FILTER_COLOR(filter, color) \
  filter->FilterColor(ToSk(color))

// GrImageFilter
#define IMAGE_FILTERS_BLUR(sigma_x, sigma_y, tile_mode, input) \
  skity::ImageFilters::Blur(sigma_x, sigma_y)
#define IMAGE_FILTERS_DILATE(radius_x, radius_y, input) \
  skity::ImageFilters::Dilate(radius_x, radius_y)
#define IMAGE_FILTERS_ERODE(radius_x, radius_y, input) \
  skity::ImageFilters::Erode(radius_x, radius_y)
#define IMAGE_FILTERS_MATRIX_TRANSFORM(matrix, sampling, input) \
  skity::ImageFilters::MatrixTransform(matrix)
#define IMAGE_FILTERS_COLOR_FILTER(color_filter, input) \
  skity::ImageFilters::ColorFilter(color_filter)

// GrRect
#define RECT_ASSIGN(dst_rect, src_rect) dst_rect = src_rect
#define RECT_IS_SORTED(rect) rect.IsSorted()

// GrImageInfo
#define IMAGE_INFO_MAKE_WH(width, height) ImageInfo::makeWH(width, height)
#define IMAGE_INFO_MAKE_DIMENSIONS(image_info, dimensions) \
  image_info.makeDimensions(dimensions)

// GrShader
#define SHADER_IS_OPAQUE(shader) shader->IsOpaque()

// GrPathEffect
#define PATH_EFFECT_MAKE_DASH(intervals, count, phase) \
  skity::PathEffect::MakeDashPathEffect(intervals, count, phase)

// GrFontMgr
#define FONT_MANAGER_MATCH_FAMILY(mgr_ptr, family_name) \
  mgr_ptr->MatchFamily(family_name)

// GrFontStyleSet
#define FONT_STYLE_SET_COUNT(style_set) style_set->Count()

// GrPoint
#define POINT_GET_X(point) point.x
#define POINT_GET_Y(point) point.y
#define POINT_SET_X(point, value) point.x = value
#define POINT_SET_Y(point, value) point.y = value

// GrBlurStyle
#define BLUR_STYLE_NORMAL skity::BlurStyle::kNormal
#define BLUR_STYLE_SOLID skity::BlurStyle::kSolid
#define BLUR_STYLE_OUTER skity::BlurStyle::kOuter
#define BLUR_STYLE_INNER skity::BlurStyle::kInner

// GrData
#define DATA_GET_DATA(pdata) pdata->RawData()
#define DATA_GET_SIZE(data) data->Size()
#define DATA_GET_WRITABLE_DATA(data) const_cast<void*>(data->RawData())
#define DATA_GET_BYTES(data) data->Bytes()
#define DATA_IS_EMPTY(data) data->IsEmpty()
#endif  // ENABLE_SKITY

}  // namespace clay

#endif  // CLAY_GFX_RENDERING_BACKEND_H_
