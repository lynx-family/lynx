// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/third_party/txt/src/tttext/paragraph_tt_text.h"

#include <textra/layout_drawer.h>
#include <textra/layout_region.h>
#include <textra/text_layout.h>
#include <textra/text_line.h>
#include <algorithm>
#include <optional>
#include "base/include/string/string_utils.h"
#include "clay/third_party/txt/src/txt/placeholder_run.h"
#ifdef ENABLE_SKITY
#include "clay/third_party/txt/src/txt/font_collection_skity.h"
#else
#include "clay/third_party/txt/src/txt/font_collection_skia.h"
#include "third_party/textlayout/textra/public/textra/platform/skia/skia_canvas_helper.h"
#include "third_party/textlayout/textra/public/textra/run_delegate.h"
#endif

namespace txt {

#ifdef ENABLE_SKITY
namespace {

// TTText paints directly to skity::Canvas, so notify GraphicsCanvas after the
// main text blob is drawn. This keeps the text paint discoverable by Clay's
// raster color animation without changing TTText's rendering behavior.
class ClaySkityCanvasHelper final : public tttext::SkityCanvasHelper {
 public:
  explicit ClaySkityCanvasHelper(clay::GraphicsCanvas* canvas,
                                 bool paint_as_mask = false)
      : tttext::SkityCanvasHelper(canvas->GetGrCanvas()),
        canvas_(canvas),
        paint_as_mask_(paint_as_mask) {}

  void DrawGlyphs(const tttext::ITypefaceHelper* font,
                  uint32_t glyph_count,
                  const uint16_t* glyphs,
                  const char* text,
                  uint32_t text_bytes,
                  float ox,
                  float oy,
                  float* pos_x,
                  float* pos_y,
                  tttext::Painter* painter) override {
    if (glyph_count == 0) {
      return;
    }

    if (!paint_as_mask_) {
      tttext::SkityCanvasHelper::DrawGlyphs(font, glyph_count, glyphs, text,
                                            text_bytes, ox, oy, pos_x, pos_y,
                                            painter);
      canvas_->OnDrawDynamicTextBlob();
      return;
    }

    auto* skity_painter = static_cast<tttext::SkityPainter*>(painter);
    // A foreground painter bypasses Painter's fill color in SkityCanvasHelper.
    // Detach it while drawing the mask so every glyph is opaque white.
    auto platform_painter = std::move(skity_painter->platform_painter_);
    const auto fill_color = painter->GetFillColor();
    const auto stroke_color = painter->GetStrokeColor();
    const auto shadows = painter->GetShadowList();
    painter->SetFillColor(tttext::TTColor::WHITE);
    painter->SetStrokeColor(tttext::TTColor::UNDEFINED);
    painter->SetShadowList({});
    tttext::SkityCanvasHelper::DrawGlyphs(font, glyph_count, glyphs, text,
                                          text_bytes, ox, oy, pos_x, pos_y,
                                          painter);
    painter->SetFillColor(fill_color);
    painter->SetStrokeColor(stroke_color);
    painter->SetShadowList(shadows);
    skity_painter->platform_painter_ = std::move(platform_painter);
    canvas_->OnDrawDynamicTextBlob();
  }

 private:
  clay::GraphicsCanvas* canvas_;
  bool paint_as_mask_;
};

}  // namespace
#endif

class TTShapeRun : public tttext::RunDelegate {
 public:
  TTShapeRun(const PlaceholderRun& span,
             const tttext::Style& style,
             const std::optional<tttext::FontInfo>& font_info) {
    FML_DCHECK(span.baseline == TextBaseline::kAlphabetic);
    if (span.alignment == PlaceholderAlignment::kMiddle) {
      float text_size = style.GetTextSize();
      if (text_size <= 0) {
        text_size = static_cast<float>(span.height);
      }
      const float text_ascent =
          font_info.has_value() ? -font_info->GetAscent() : text_size * 0.75f;
      const float text_descent = font_info.has_value()
                                     ? font_info->GetDescent()
                                     : text_size - text_ascent;
      const float middle = (text_ascent - text_descent) / 2.f;
      ascent_ = -std::max(0.f, middle + static_cast<float>(span.height) / 2.f);
      descent_ = std::max(0.f, -middle + static_cast<float>(span.height) / 2.f);
    } else {
      const float baseline = static_cast<float>(span.baseline_offset);
      ascent_ = -baseline;
      descent_ = static_cast<float>(span.height) - baseline;
    }
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

namespace {

std::optional<tttext::FontInfo> ResolveFontInfo(
    const std::shared_ptr<FontCollection>& font_collection,
    const tttext::Style& style) {
  if (font_collection == nullptr || style.GetTextSize() <= 0) {
    return std::nullopt;
  }
#ifdef ENABLE_SKITY
  auto tt_font_collection = font_collection->GetIFontCollection();
  auto* tt_font_collection_ptr = &tt_font_collection;
#else
  auto tt_font_collection = font_collection->CreateTTFontCollection();
  auto* tt_font_collection_ptr = tt_font_collection.get();
#endif
  if (tt_font_collection_ptr == nullptr) {
    return std::nullopt;
  }
  auto typefaces =
      tt_font_collection_ptr->findTypefaces(style.GetFontDescriptor());
  if (typefaces.empty() || typefaces.front() == nullptr) {
    return std::nullopt;
  }
  return typefaces.front()->GetFontInfo(style.GetTextSize());
}

}  // namespace

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
  return region_->GetLine(0)->GetLineBaseLine();
}
double ParagraphTTText::GetIdeographicBaseline() {
  if (region_ == nullptr || region_->GetLineCount() == 0) {
    return 0;
  }
  return region_->GetLine(0)->GetLineBottom();
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
#if defined(ENABLE_SKITY)
  auto i_font_collection = font_collection_->GetIFontCollection();
  tttext::TextLayout layout(&i_font_collection, tttext::kSelfRendering);
#else
  auto i_font_collection = font_collection_->CreateTTFontCollection();
  tttext::TextLayout layout(i_font_collection.get(), tttext::kSelfRendering);
#endif
  auto& paragraph_style = paragraph_->GetParagraphStyle();
  auto halign = paragraph_style.GetHorizontalAlign();
  if (std::isnan(width)) {
    FML_DCHECK(false) << "The TTText layout width must not be NaN.";
    width = 0;
  }
  const bool is_unbounded =
      std::isinf(width) ||
      width >= static_cast<double>(std::numeric_limits<float>::max());
  if (is_unbounded) {
    paragraph_style.SetHorizontalAlign(
        tttext::ParagraphHorizontalAlignment::kLeft);
  }
  auto layout_halign = paragraph_style.GetHorizontalAlign();
  auto width_mode = layout_halign == tttext::ParagraphHorizontalAlignment::kLeft
                        ? tttext::LayoutMode::kAtMost
                        : tttext::LayoutMode::kDefinite;
  region_ = std::make_unique<tttext::LayoutRegion>(
      width, std::numeric_limits<float>::max(), width_mode,
      tttext::LayoutMode::kAtMost);
  tttext::TTTextContext context;
  context.SetEnableSystemFontAdjust(false);
  if (need_trim_space_) {
    context.EnableFeature(ttoffice::tttext::FeatureOption::kTrimLineTailSpace,
                          false);
  }
  tttext::LayoutResult result =
      layout.Layout(paragraph_.get(), region_.get(), context);
  if (is_unbounded) {
    paragraph_style.SetHorizontalAlign(halign);
  }
  if (result != tttext::LayoutResult::kNormal &&
      result != tttext::LayoutResult::kBreakPage) {
    FML_DCHECK(false) << "TTText layout result is not normal!";
  }
  line_metrics_.resize(region_->GetLineCount());

  for (uint32_t k = 0; k < region_->GetLineCount(); k++) {
    auto* text_line = region_->GetLine(k);
    auto& metrics = line_metrics_[k];
    metrics.baseline = text_line->GetLineBaseLine();
    metrics.ascent = metrics.baseline - text_line->GetLineTop();
    metrics.descent = text_line->GetLineBottom() - metrics.baseline;
    float rect_ltwh[4];
    text_line->GetBoundingRectForLine(rect_ltwh);
    metrics.width = rect_ltwh[2];
    metrics.height = text_line->GetLineBottom();
    metrics.line_number = k;
    metrics.start_index = text_line->GetStartCharPos();
    metrics.end_index = text_line->GetEndCharPos();
    metrics.start_index = index_mapper_.ToUTF16Position(metrics.start_index);
    metrics.end_index = index_mapper_.ToUTF16Position(metrics.end_index);
    metrics.hard_break =
        paragraph_->GetContentString(text_line->GetEndCharPos(), 1) == "\n";
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

void ParagraphTTText::PaintMask(SkCanvas* canvas, double x, double y) {
#ifdef ENABLE_SKITY
  FML_DCHECK(false);
#else
  paragraph_->SaveStyle();
  tttext::Style mask_style;
  mask_style.SetForegroundColor(tttext::TTColor(0xFFFFFFFF));
  mask_style.SetForegroundPainter(nullptr);
  paragraph_->ApplyStyleInRange(mask_style, 0, paragraph_->GetCharCount());
  Paint(canvas, x, y);
  paragraph_->RestoreStyle();
#endif
}

#ifdef ENABLE_SKITY
void ParagraphTTText::Paint(clay::GraphicsCanvas* canvas, double x, double y) {
  canvas->Save();
  canvas->Translate(x, y);
  ClaySkityCanvasHelper helper(canvas);
  tttext::LayoutDrawer drawer(&helper);
  drawer.DrawLayoutPage(region_.get());
  canvas->Restore();
}

void ParagraphTTText::PaintMask(clay::GraphicsCanvas* canvas,
                                double x,
                                double y) {
  canvas->Save();
  canvas->Translate(x, y);
  ClaySkityCanvasHelper helper(canvas, true);
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
  start = index_mapper_.ToTTTextPosition(start);
  end = index_mapper_.ToTTTextRangeEnd(end);
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
    result.push_back(
        TextBox(skity::Rect::MakeXYWH(rect[0], rect[1], rect[2], rect[3]),
                TextDirection::ltr));
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
                      TextDirection::ltr, i));
        }
      }
    }
  }
  return result;
}
Paragraph::PositionWithAffinity ParagraphTTText::GetGlyphPositionAtCoordinate(
    double dx,
    double dy) {
  ttoffice::tttext::CharPos result = 0;
  for (uint32_t k = 0; k < region_->GetLineCount(); k++) {
    auto* text_line = region_->GetLine(k);
    float rect[4] = {0};
    text_line->GetBoundingRectForLine(rect);
    if (k == region_->GetLineCount() - 1 && dy > rect[1] + rect[3]) {
      dy = rect[1] + rect[3];
    }
    if (rect[2] != 0 && rect[3] != 0 && dy <= rect[1] + rect[3]) {
      if ((k != 0 && dy < rect[1])) {
        break;
      }
      auto char_pos = text_line->GetCharPosByCoordinateX(dx);
      if (char_pos > 0) {
        auto end_char = paragraph_->GetContentString(char_pos - 1, 1);
        if (end_char == "\n") {
          char_pos -= 1;
        }
      }
      result += char_pos;
      break;
    } else {
      result += text_line->GetCharCount();
    }
  }
  result = index_mapper_.ToUTF16Position(result);
  return Paragraph::PositionWithAffinity(result, Paragraph::DOWNSTREAM);
}
Paragraph::Range<size_t> ParagraphTTText::GetWordBoundary(size_t offset) {
  if (region_ == nullptr)
    return Range<size_t>(0, 0);
  if (index_mapper_.GetUTF16Size() == 0)
    return Range<size_t>(0, 0);
  offset = index_mapper_.ToTTTextPosition(offset);
  auto word = paragraph_->GetWordBoundary(offset);
  word.first = index_mapper_.ToUTF16Position(word.first);
  word.second = index_mapper_.ToUTF16Position(word.second);
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
  tt_painter_.sk_paint_ = std::make_unique<SkPaint>(sk_paint_);
#endif
  tttext::Style style;
  style.SetForegroundPainter(&tt_painter_);
  text_size = index_mapper_.ToTTTextRangeEnd(text_size);
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
  start = index_mapper_.ToTTTextPosition(start);
  end = index_mapper_.ToTTTextRangeEnd(end);
  paragraph_->ApplyStyleInRange(style, start, end - start);
}
#endif

void ParagraphTTText::AddPlaceholder(tttext::Style& style,
                                     PlaceholderRun& span,
                                     bool is_float) {
  auto font_info = ResolveFontInfo(font_collection_, style);
  auto delegate = std::make_unique<TTShapeRun>(span, style, font_info);
  placeholder_pos_.push_back(paragraph_->GetCharCount());
  index_mapper_.AppendText(u"\uFFFC");
  paragraph_->AddShapeRun(&style, std::move(delegate), is_float);
}

void ParagraphTTText::AddTextRun(tttext::Style& style,
                                 const std::u16string& content) {
  const std::u16string supported_content =
      content.substr(0, content.find(u'\0'));
  index_mapper_.AppendText(supported_content);
  const std::string utf8_content = lynx::base::U16StringToU8(supported_content);
  paragraph_->AddTextRun(&style, utf8_content.data(),
                         static_cast<uint32_t>(utf8_content.size()));
}
void ParagraphTTText::AddTextRun(tttext::Style& style,
                                 const std::string& content) {
  const std::string supported_content = content.substr(0, content.find('\0'));
  index_mapper_.AppendText(lynx::base::U8StringToU16(supported_content));
  paragraph_->AddTextRun(&style, supported_content.data(),
                         static_cast<uint32_t>(supported_content.size()));
}
uint32_t ParagraphTTText::GetTextSize() const {
  return static_cast<uint32_t>(index_mapper_.GetUTF16Size());
}
tttext::WriteDirection ParagraphTTText::GetResolvedWriteDirection() const {
  return paragraph_->GetResolvedWriteDirection();
}

}  // namespace txt
