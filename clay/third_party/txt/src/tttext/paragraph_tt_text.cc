// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/third_party/txt/src/tttext/paragraph_tt_text.h"

#include <textra/layout_drawer.h>
#include <textra/layout_region.h>
#include <textra/text_layout.h>
#include <textra/text_line.h>
#include <algorithm>
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

namespace {

constexpr float kSubScriptShiftRatio = 0.2f;
constexpr float kSuperScriptShiftRatio = -0.33f;

#ifdef ENABLE_SKITY
// TTText paints directly to skity::Canvas, so notify GraphicsCanvas after the
// main text blob is drawn. This keeps the text paint discoverable by Clay's
// raster color animation without changing TTText's rendering behavior.
class ClaySkityCanvasHelper final : public tttext::SkityCanvasHelper {
 public:
  explicit ClaySkityCanvasHelper(clay::GraphicsCanvas* canvas)
      : tttext::SkityCanvasHelper(canvas->GetGrCanvas()), canvas_(canvas) {}

  void DrawGlyphs(const tttext::ITypefaceHelper* font, uint32_t glyph_count,
                  const uint16_t* glyphs, const char* text,
                  uint32_t text_bytes, float ox, float oy, float* pos_x,
                  float* pos_y, tttext::Painter* painter) override {
    if (glyph_count == 0) {
      return;
    }
    tttext::SkityCanvasHelper::DrawGlyphs(
        font, glyph_count, glyphs, text, text_bytes, ox, oy, pos_x, pos_y,
        painter);
    canvas_->OnDrawDynamicTextBlob();
  }

 private:
  clay::GraphicsCanvas* canvas_;
};
#endif

}  // namespace

class TTShapeRun : public tttext::RunDelegate {
 public:
  TTShapeRun(const PlaceholderRun& span, const tttext::Style& style) {
    FML_DCHECK(span.baseline == TextBaseline::kAlphabetic);
    if (span.alignment == PlaceholderAlignment::kMiddle) {
      float text_size = style.GetTextSize();
      if (text_size <= 0) {
        text_size = static_cast<float>(span.height);
      }
      const float text_ascent = text_size * 0.75f;
      const float text_descent = text_size - text_ascent;
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
  auto width_mode =
      layout_halign == tttext::ParagraphHorizontalAlignment::kLeft
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
    metrics.left = rect_ltwh[0];
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

#ifdef ENABLE_SKITY
void ParagraphTTText::Paint(clay::GraphicsCanvas* canvas, double x, double y) {
  canvas->Save();
  canvas->Translate(x, y);
  ClaySkityCanvasHelper helper(canvas);
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

    const auto range_start = std::max(start, start_index);
    const auto range_end = std::min(end, end_index);
    if (rect_height_style == Paragraph::RectHeightStyle::kLineBox) {
      float range_rect[4] = {0};
      text_line->GetBoundingRectByCharRange(range_rect, range_start, range_end);
      const auto line_box = line_metrics_[k].GetLineBox();
      range_rect[1] = line_box.Top();
      range_rect[3] = line_box.Height();
      result.emplace_back(skity::Rect::MakeXYWH(range_rect[0], range_rect[1],
                                                range_rect[2], range_rect[3]),
                          TextDirection::ltr);
      continue;
    }

    std::vector<size_t> range_boundaries = {range_start, range_end};
    for (const auto& baseline_offset : text_baseline_offsets_) {
      if (baseline_offset.end > range_start &&
          baseline_offset.start < range_end) {
        range_boundaries.push_back(
            std::max(range_start, baseline_offset.start));
        range_boundaries.push_back(std::min(range_end, baseline_offset.end));
      }
    }
    std::sort(range_boundaries.begin(), range_boundaries.end());
    range_boundaries.erase(
        std::unique(range_boundaries.begin(), range_boundaries.end()),
        range_boundaries.end());

    skity::Rect line_rect;
    bool has_line_rect = false;
    for (size_t i = 1; i < range_boundaries.size(); ++i) {
      const size_t segment_start = range_boundaries[i - 1];
      const size_t segment_end = range_boundaries[i];
      float rect[4] = {0};
      text_line->GetBoundingRectByCharRange(rect, segment_start, segment_end);
      for (const auto& baseline_offset : text_baseline_offsets_) {
        if (baseline_offset.start <= segment_start &&
            segment_start < baseline_offset.end) {
          rect[1] += baseline_offset.offset;
          break;
        }
      }
      auto segment_rect =
          skity::Rect::MakeXYWH(rect[0], rect[1], rect[2], rect[3]);
      if (has_line_rect) {
        line_rect.Join(segment_rect);
      } else {
        line_rect = segment_rect;
        has_line_rect = true;
      }
    }
    if (has_line_rect) {
      result.emplace_back(line_rect, TextDirection::ltr);
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
    const auto line_box = line_metrics_[k].GetLineBox();
    const float line_top = line_box.Top();
    const float line_bottom = line_box.Bottom();
    if (k == region_->GetLineCount() - 1 && dy > line_bottom) {
      dy = line_bottom;
    }
    // Choose the line from its full line box rather than the tight glyph
    // bounds. With an exact line height, the leading around the glyphs still
    // belongs to that line and must remain horizontally selectable.
    if (dy <= line_bottom) {
      if (k != 0 && dy < line_top) {
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
  // An inline placeholder is an atomic inline-level box and must be allowed to
  // expand its line box beyond the specified line height. TTText's exact rule
  // clamps object runs as well as glyph runs, which can move a tall placeholder
  // above the preceding line and leave following lines overlapping it.
  auto& paragraph_style = paragraph_->GetParagraphStyle();
  if (paragraph_style.GetLineHeightRule() == tttext::RulerType::kExact) {
    paragraph_style.SetLineHeightInPxAtLeast(
        paragraph_style.GetLineHeightInPx());
  }
  // TTText applies subscript and superscript after it has finalized the line
  // metrics. Represent the same object-run shift as a baseline offset so the
  // containing line expands around the placeholder's final bounds.
  const auto vertical_alignment = style.GetVerticalAlignment();
  if (vertical_alignment == tttext::CharacterVerticalAlignment::kSubScript ||
      vertical_alignment == tttext::CharacterVerticalAlignment::kSuperScript) {
    const float shift =
        static_cast<float>(span.height) *
        (vertical_alignment == tttext::CharacterVerticalAlignment::kSubScript
             ? kSubScriptShiftRatio
             : kSuperScriptShiftRatio);
    style.SetVerticalAlignment(tttext::CharacterVerticalAlignment::kBaseLine);
    style.SetBaselineOffset(style.GetBaselineOffset() + shift);
  }
  auto delegate = std::make_unique<TTShapeRun>(span, style);
  placeholder_pos_.push_back(paragraph_->GetCharCount());
  index_mapper_.AppendText(u"\uFFFC");
  paragraph_->AddShapeRun(&style, std::move(delegate), is_float);
}

void ParagraphTTText::AddTextRun(tttext::Style& style,
                                 const std::u16string& content) {
  const std::u16string supported_content =
      content.substr(0, content.find(u'\0'));
  index_mapper_.AppendText(supported_content);
  AddTextRunWithBaselineOffset(
      style, lynx::base::U16StringToU8(supported_content));
}
void ParagraphTTText::AddTextRun(tttext::Style& style,
                                 const std::string& content) {
  const std::string supported_content = content.substr(0, content.find('\0'));
  index_mapper_.AppendText(lynx::base::U8StringToU16(supported_content));
  AddTextRunWithBaselineOffset(style, supported_content);
}
void ParagraphTTText::AddTextRunWithBaselineOffset(tttext::Style& style,
                                                   const std::string& content) {
  if (style.GetBaselineOffset() != 0.f) {
    // A length or percentage vertical-align shifts the inline box relative to
    // the parent baseline. The shifted box still participates in line-box
    // sizing, so a specified line height is a minimum rather than a clamp.
    // Otherwise TTText centers over-tall metrics into the exact height and can
    // move the whole line into its preceding sibling.
    auto& paragraph_style = paragraph_->GetParagraphStyle();
    if (paragraph_style.GetLineHeightRule() == tttext::RulerType::kExact) {
      paragraph_style.SetLineHeightInPxAtLeast(
          paragraph_style.GetLineHeightInPx());
    }
  }
  const size_t start = paragraph_->GetCharCount();
  paragraph_->AddTextRun(&style, content.data(),
                         static_cast<uint32_t>(content.size()));
  if (style.GetBaselineOffset() != 0.f) {
    text_baseline_offsets_.push_back(
        {start, paragraph_->GetCharCount(), style.GetBaselineOffset()});
  }
}
uint32_t ParagraphTTText::GetTextSize() const {
  return static_cast<uint32_t>(index_mapper_.GetUTF16Size());
}
tttext::WriteDirection ParagraphTTText::GetResolvedWriteDirection() const {
  return paragraph_->GetResolvedWriteDirection();
}

}  // namespace txt
