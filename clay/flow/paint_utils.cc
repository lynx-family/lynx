// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/flow/paint_utils.h"

#include <stdlib.h>

#include <string>

#include "clay/fml/logging.h"
#include "clay/gfx/style/color.h"

namespace clay {

namespace {

[[maybe_unused]] clay::GrShaderPtr CreateCheckerboardShader(Color c1, Color c2,
                                                            int size) {
#ifndef ENABLE_SKITY
  SkBitmap bm;
  bm.allocN32Pixels(2 * size, 2 * size);
  bm.eraseColor(ToSk(c1));
  bm.eraseArea(SkIRect::MakeLTRB(0, 0, size, size), c2);
  bm.eraseArea(SkIRect::MakeLTRB(size, size, 2 * size, 2 * size), c2);
  return bm.makeShader(SkTileMode::kRepeat, SkTileMode::kRepeat,
                       SkSamplingOptions());
#else
#if !OS_IOS
  auto bitmap =
      skity::Bitmap(size * 2, size * 2, skity::AlphaType::kPremul_AlphaType);
  auto canvas = skity::Canvas::MakeSoftwareCanvas(&bitmap);
  canvas->Clear(ToSk(c1));
  skity::Paint paint;
  paint.SetColor(ToSk(c2));
  paint.SetBlendMode(skity::BlendMode::kSrc);
  canvas->DrawRect(skity::Rect::MakeLTRB(0, 0, size, size), paint);
  canvas->DrawRect(skity::Rect::MakeLTRB(size, size, size * 2, size * 2),
                   paint);
  auto shader = skity::Shader::MakeShader(
      skity::Image::MakeImage(bitmap.GetPixmap()), skity::SamplingOptions(),
      skity::TileMode::kRepeat, skity::TileMode::kRepeat);
  return shader;
#else
  return nullptr;
#endif  // OS_IOS
#endif  // ENABLE_SKITY
}

}  // anonymous namespace

void DrawCheckerboard(clay::GrCanvas* canvas, const skity::Rect& rect) {
#if !ENABLE_SKITY
  if (!canvas) {
    return;
  }
  // Draw a checkerboard
  CANVAS_SAVE(canvas);
  CANVAS_CLIP_RECT(canvas, rect);

  // Secure random number generation isn't needed here.
  // NOLINTBEGIN(clang-analyzer-security.insecureAPI.rand)
  auto checkerboard_color =
      Color::ARGBColor(64, rand() % 256, rand() % 256, rand() % 256);
  // NOLINTEND(clang-analyzer-security.insecureAPI.rand)

  clay::GrPaint paint;
  PAINT_SET_SHADER(
      paint, CreateCheckerboardShader(checkerboard_color, 0x00000000, 12));
  CANVAS_DRAW_PAINT(canvas, paint);
  CANVAS_RESTORE(canvas);

  // Stroke the drawn area
  clay::GrPaint debug_paint;
  PAINT_SET_STROKE_WIDTH(debug_paint, 8);
  PAINT_SET_COLOR(debug_paint, Color::ColorSetA(checkerboard_color, 255));
  PAINT_SET_STYLE(debug_paint, 1);
  CANVAS_DRAW_RECT(canvas, rect, debug_paint);
#endif
}

#if !defined(NDEBUG)
void DrawRasterCacheTag(GrCanvas* canvas, float x, float y, int use_count) {
  FML_DCHECK(use_count >= 0);
  std::string tag = std::to_string(use_count);
  GrPaint paint;
  if (use_count == 0) {
    PAINT_SET_COLOR(paint, Color::kRed());
  } else {
    PAINT_SET_COLOR(paint, Color::kGreen());
  }
#ifndef ENABLE_SKITY
  SkFont font;
  font.setSize(20.f);
  canvas->drawString(tag.c_str(), x, y, font, paint);
#else
  paint.SetTextSize(20.f);
  canvas->DrawSimpleText2(tag.c_str(), x, y, paint);
#endif  // ENABLE_SKITY
}

void DrawDebugBorders(GrCanvas* canvas, const skity::Rect& bounds) {
  GrPaint paint;
  PAINT_SET_COLOR(paint, Color::ARGBColor(0xFF, 210, 105, 30));  // brown
  PAINT_SET_STYLE(paint, GrPaint::kStroke_Style);
  PAINT_SET_STROKE_WIDTH(paint, 3.f);
  auto rect = bounds;
  rect.Inset(0.5f, 0.5f);
  CANVAS_DRAW_RECT(canvas, rect, paint);
}
#endif

}  // namespace clay
