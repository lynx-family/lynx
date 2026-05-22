// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_THIRD_PARTY_TXT_SRC_TTTEXT_PARAGRAPH_TT_TEXT_H_
#define CLAY_THIRD_PARTY_TXT_SRC_TTTEXT_PARAGRAPH_TT_TEXT_H_

#include <memory>
#include <string>
#include <vector>

#include <textra/paragraph_style.h>
#include "clay/gfx/graphics_canvas.h"
#ifdef ENABLE_SKITY
#include "tttext/tttext_headers.h"
#else
#include "third_party/textlayout/src/ports/platform/skia/skia_painter.h"
#endif
#include "txt/paragraph.h"

namespace ttoffice {
namespace tttext {
class Paragraph;
class Style;
class LayoutRegion;
}  // namespace tttext
}  // namespace ttoffice
namespace txt {
class FontCollection;
class PlaceholderRun;

// Implementation of Paragraph based on Skia's text layout module.
class ParagraphTTText : public Paragraph {
 public:
  ParagraphTTText(std::shared_ptr<FontCollection> font_collection,
                  const tttext::ParagraphStyle& paragraph_style);

  ~ParagraphTTText() override = default;

  double GetMaxWidth() override;

  double GetHeight() override;

  double GetLongestLine() override;

  double GetMinIntrinsicWidth() override;

  double GetMaxIntrinsicWidth() override;

  double GetAlphabeticBaseline() override;

  double GetIdeographicBaseline() override;

  std::vector<LineMetrics>& GetLineMetrics() override;

  bool DidExceedMaxLines() override;

  void Layout(double width) override;

  void Paint(SkCanvas* canvas, double x, double y) override;

#ifdef ENABLE_SKITY
  void Paint(clay::GraphicsCanvas* canvas, double x, double y);
#endif

  std::vector<TextBox> GetRectsForRange(
      size_t start,
      size_t end,
      RectHeightStyle rect_height_style,
      RectWidthStyle rect_width_style) override;

  std::vector<TextBox> GetRectsForPlaceholders() override;

  PositionWithAffinity GetGlyphPositionAtCoordinate(double dx,
                                                    double dy) override;

  Range<size_t> GetWordBoundary(size_t offset) override;

#ifdef ENABLE_SKITY
  void UpdateForegroundPaint(size_t text_size, skity::Paint paint);
  void UpdateForegroundPaint(size_t start, size_t end, skity::Paint paint);
#else
  void UpdateForegroundPaint(size_t text_size, SkPaint paint);
#endif

  void AddPlaceholder(tttext::Style& style,
                      PlaceholderRun& span,
                      bool is_float);
  void AddTextRun(tttext::Style& style, const std::u16string& content);
  void AddTextRun(tttext::Style& style, const std::string& content);
  uint32_t GetTextSize() const;

 private:
  std::shared_ptr<FontCollection> font_collection_;
  std::unique_ptr<tttext::Paragraph> paragraph_;
  std::unique_ptr<tttext::LayoutRegion> region_;
  std::vector<LineMetrics> line_metrics_;
  std::vector<size_t> placeholder_pos_;
#ifdef ENABLE_SKITY
  skity::Paint sk_paint_;
  tttext::SkityPainter tt_painter_;
  std::vector<std::unique_ptr<tttext::SkityPainter>> tt_painters_;
#else
  SkPaint sk_paint_;
  tttext::SkiaPainter tt_painter_;
#endif
};
}  // namespace txt
#endif  // CLAY_THIRD_PARTY_TXT_SRC_TTTEXT_PARAGRAPH_TT_TEXT_H_
