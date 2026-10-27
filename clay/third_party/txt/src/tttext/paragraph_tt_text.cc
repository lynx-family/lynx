// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "paragraph_tt_text.h"

#include <textra/layout_drawer.h>
#include <textra/layout_region.h>
#include <textra/text_layout.h>
#include <textra/text_line.h>
#include "base/include/string/string_utils.h"
#include "txt/placeholder_run.h"
#ifdef ENABLE_SKITY
#include "txt/font_collection_skity.h"
#else
#include "src/ports/platform/skia/skia_canvas_helper.h"
#endif

namespace txt {

class TTShapeRun : public tttext::RunDelegate {
 public:
  TTShapeRun(const PlaceholderRun& span) {
    FML_DCHECK(span.baseline == TextBaseline::kAlphabetic);
    FML_DCHECK(span.alignment == PlaceholderAlignment::kBaseline);
    ascent_ = -span.height;
    descent_ = 0;
    advance_ = span.width;
  }
  float GetAscent() const override { return ascent_; }
  float GetDescent() const override { return descent_; }
  float GetAdvance() const override { return advance_; }

 private:
  float ascent_;
  float descent_;
  float advance_;
};

ParagraphTTText::ParagraphTTText(
    std::shared_ptr<FontCollection> font_collection,
    const tttext::ParagraphStyle& paragraph_style)
    : font_collection_(font_collection) {
  paragraph_ = tttext::Paragraph::Create();
  paragraph_->SetParagraphStyle(&paragraph_style);
}
double ParagraphTTText::GetMaxWidth() {
  if (region_ == nullptr)
    return 0;
  return region_->GetLayoutedWidth();
}
double ParagraphTTText::GetHeight() {
  if (region_ == nullptr)
    return 0;
  return region_->GetLayoutedHeight();
}
double ParagraphTTText::GetLongestLine() {
  return GetMaxWidth();
}
double ParagraphTTText::GetMinIntrinsicWidth() {
  return GetMaxWidth();
}
double ParagraphTTText::GetMaxIntrinsicWidth() {
  return GetMaxWidth();
}
double ParagraphTTText::GetAlphabeticBaseline() {
  if (region_ == nullptr || region_->GetLineCount() == 0) {
    return 0;
  }
  return region_->GetLine(0)->GetMaxAscent();
}
double ParagraphTTText::GetIdeographicBaseline() {
  if (region_ == nullptr || region_->GetLineCount() == 0) {
    return 0;
  }
  return region_->GetLine(0)->GetMaxAscent() +
         region_->GetLine(0)->GetMaxDescent();
}
std::vector<LineMetrics>& ParagraphTTText::GetLineMetrics() {
  return line_metrics_;
}

bool ParagraphTTText::DidExceedMaxLines() {
  if (region_ == nullptr) {
    return false;
  }
  return region_->DidExceedMaxLines();
}

void ParagraphTTText::Layout(double width) {
  auto i_font_collection = font_collection_->GetIFontCollection();
  tttext::TextLayout layout(&i_font_collection, tttext::kSelfRendering);
  auto halign = paragraph_->GetParagraphStyle().GetHorizontalAlign();
  auto width_mode =
      std::isinf(width) || halign == tttext::ParagraphHorizontalAlignment::kLeft
          ? tttext::LayoutMode::kAtMost
          : tttext::LayoutMode::kDefinite;
  region_ = std::make_unique<tttext::LayoutRegion>(
      width, std::numeric_limits<float>::max(), width_mode,
      tttext::LayoutMode::kAtMost);
  tttext::TTTextContext context;
  tttext::LayoutResult result =
      layout.Layout(paragraph_.get(), region_.get(), context);
  if (result != tttext::LayoutResult::kNormal &&
      result != tttext::LayoutResult::kBreakPage) {
    FML_DCHECK(false) << "TTText layout result is not normal!";
  }
  line_metrics_.resize(region_->GetLineCount());

  for (uint32_t k = 0; k < region_->GetLineCount(); k++) {
    auto* text_line = region_->GetLine(k);
    auto& metrics = line_metrics_[k];
    metrics.ascent = text_line->GetMaxAscent();
    metrics.descent = text_line->GetMaxDescent();
    float rect_ltwh[4];
    text_line->GetBoundingRectForLine(rect_ltwh);
    metrics.width = rect_ltwh[2];
    metrics.height = rect_ltwh[3];
    metrics.baseline = text_line->GetLineTop() + text_line->GetMaxAscent();
    metrics.line_number = k;
    metrics.start_index = text_line->GetStartCharPos();
    metrics.end_index = text_line->GetEndCharPos();
  }
}

void ParagraphTTText::Paint(SkCanvas* canvas, double x, double y) {
#ifdef ENABLE_SKITY
  FML_DCHECK(false);
#else
  canvas->save();
  canvas->translate(x, y);
  SkiaCanvasHelper helper(canvas);
  tttext::LayoutDrawer drawer(&helper);
  drawer.DrawLayoutPage(region_.get());
  canvas->restore();
#endif
}

#ifdef ENABLE_SKITY
void ParagraphTTText::Paint(clay::GraphicsCanvas* canvas, double x, double y) {
  canvas->Save();
  canvas->Translate(x, y);
  tttext::SkityCanvasHelper helper(canvas->GetGrCanvas());
  tttext::LayoutDrawer drawer(&helper);
  drawer.DrawLayoutPage(region_.get());
  canvas->Restore();
}
#endif

std::vector<Paragraph::TextBox> ParagraphTTText::GetRectsForRange(
    size_t start,
    size_t end,
    Paragraph::RectHeightStyle rect_height_style,
    Paragraph::RectWidthStyle rect_width_style) {
  std::vector<TextBox> result;
  for (uint32_t k = 0; k < region_->GetLineCount(); k++) {
    auto* text_line = region_->GetLine(k);
    size_t start_index = text_line->GetStartCharPos();
    size_t end_index = text_line->GetEndCharPos();

    // Check to see if we are finished.
    if (start_index >= end)
      break;

    if (end_index <= start)
      continue;

    float rect[4] = {0};
    text_line->GetBoundingRectByCharRange(rect, std::max(start, start_index),
                                          std::min(end, end_index));
    if (rect[2] != 0 && rect[3] != 0) {
      result.push_back(
          TextBox(skity::Rect::MakeXYWH(rect[0], rect[1], rect[2], rect[3]),
                  TextDirection::ltr));
    }
  }
  return result;
}
std::vector<Paragraph::TextBox> ParagraphTTText::GetRectsForPlaceholders() {
  std::vector<TextBox> result;
  for (uint32_t k = 0; k < region_->GetLineCount(); k++) {
    auto* text_line = region_->GetLine(k);
    size_t start_index = text_line->GetStartCharPos();
    size_t end_index = text_line->GetEndCharPos();
    for (size_t i = 0; i < placeholder_pos_.size(); i++) {
      if (placeholder_pos_[i] >= start_index &&
          placeholder_pos_[i] < end_index) {
        float rect[4] = {0};
        text_line->GetCharBoundingRect(rect, placeholder_pos_[i]);
        if (rect[2] != 0 && rect[3] != 0) {
          result.push_back(
              TextBox(skity::Rect::MakeXYWH(rect[0], rect[1], rect[2], rect[3]),
                      TextDirection::ltr));
        }
      }
    }
  }
  return result;
}
Paragraph::PositionWithAffinity ParagraphTTText::GetGlyphPositionAtCoordinate(
    double dx,
    double dy) {
  ttoffice::tttext::CharPos char_pos = 0;
  for (uint32_t k = 0; k < region_->GetLineCount(); k++) {
    auto* text_line = region_->GetLine(k);
    float rect[4] = {0};
    text_line->GetBoundingRectForLine(rect);
    if (rect[2] != 0 && rect[3] != 0 && dy < rect[1] + rect[3]) {
      if ((k != 0 && dy < rect[1])) {
        break;
      }
      char_pos += text_line->GetCharPosByCoordinateX(dx);
      break;
    } else {
      char_pos += text_line->GetCharCount();
    }
  }
  return Paragraph::PositionWithAffinity(char_pos, Paragraph::DOWNSTREAM);
}
Paragraph::Range<size_t> ParagraphTTText::GetWordBoundary(size_t offset) {
  if (region_ == nullptr)
    return Range<size_t>(0, 0);
  auto word = paragraph_->GetWordBoundary(offset);
  return Paragraph::Range<size_t>(word.first, word.second);
}

void ParagraphTTText::UpdateForegroundPaint(size_t text_size,
#ifdef ENABLE_SKITY
                                            skity::Paint paint) {
#else
                                            SkPaint paint) {
#endif
  sk_paint_ = paint;

#ifdef ENABLE_SKITY
  tt_painter_.platform_painter_ = std::make_unique<skity::Paint>(sk_paint_);
#else
  tt_painter_.sk_paint_ = &sk_paint_;
#endif
  tttext::Style style;
  style.SetForegroundPainter(&tt_painter_);
  paragraph_->ApplyStyleInRange(style, 0, text_size);
}

#ifdef ENABLE_SKITY
void ParagraphTTText::UpdateForegroundPaint(size_t start,
                                            size_t end,
                                            skity::Paint paint) {
  // Make sure start and end are valid positions.
  tttext::Style style;
  auto skity_painter = std::make_unique<tttext::SkityPainter>();
  tttext::SkityPainter* tt_painter = skity_painter.get();
  tt_painters_.push_back(std::move(skity_painter));
  tt_painter->platform_painter_ = std::make_unique<skity::Paint>(paint);
  style.SetForegroundPainter(tt_painter);
  paragraph_->ApplyStyleInRange(style, start, end - start);
}
#endif

void ParagraphTTText::AddPlaceholder(tttext::Style& style,
                                     PlaceholderRun& span,
                                     bool is_float) {
  auto delegate = std::make_unique<TTShapeRun>(span);
  placeholder_pos_.push_back(paragraph_->GetCharCount());
  paragraph_->AddShapeRun(&style, std::move(delegate), is_float);
}

void ParagraphTTText::AddTextRun(tttext::Style& style,
                                 const std::u16string& content) {
  paragraph_->AddTextRun(&style, lynx::base::U16StringToU8(content).c_str());
}
void ParagraphTTText::AddTextRun(tttext::Style& style,
                                 const std::string& content) {
  paragraph_->AddTextRun(&style, content.c_str());
}
uint32_t ParagraphTTText::GetTextSize() const {
  return paragraph_->GetCharCount();
}
}  // namespace txt
