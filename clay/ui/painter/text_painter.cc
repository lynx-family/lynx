// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/painter/text_painter.h"

#include <algorithm>
#include <map>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

#include "clay/fml/logging.h"
#include "clay/gfx/geometry/float_rect.h"
#include "clay/gfx/geometry/float_size.h"
#include "clay/gfx/paint.h"
#include "clay/gfx/rendering_backend.h"
#include "clay/gfx/style/color.h"
#include "clay/ui/common/editing_misc.h"
#include "clay/ui/common/isolate.h"
#include "clay/ui/painter/gradient_factory.h"
#include "skity/geometry/rect.hpp"
#if defined(CLAY_ENABLE_SKSHAPER)
#include "clay/third_party/txt/src/skia/paragraph_builder_skia.h"
#include "clay/third_party/txt/src/skia/paragraph_skia.h"
#include "third_party/skia/modules/skparagraph/src/ParagraphImpl.h"  // nogncheck
#elif defined(CLAY_ENABLE_TTTEXT)
#include "clay/third_party/txt/src/tttext/paragraph_tt_text.h"
#endif
#include "clay/third_party/txt/src/txt/paragraph.h"

namespace clay {

TextPainter::TextPainter() = default;

TextPainter::~TextPainter() = default;

void TextPainter::SetWidth(float width) {
  if (width != width_) {
    width_ = width;
    is_gradient_dirty_ = true;
  }
}

void TextPainter::SetHeight(float height) {
  if (height != height_) {
    height_ = height;
    is_gradient_dirty_ = true;
  }
}

void TextPainter::SetParagraph(txt::Paragraph* paragraph) {
  paragraph_ = paragraph;
  is_gradient_dirty_ = true;
}

void TextPainter::SetGradient(const std::optional<Gradient>& gradient) {
  is_gradient_dirty_ = false;
  if (!paragraph_) {
    FML_DCHECK(false) << "paragraph is null";
    return;
  }
  if (gradient) {
    auto gradient_shader = GradientFactory::CreateShader(
        gradient.value(),
        FloatRect(0, 0, GetLongestLine(), paragraph_->GetHeight()));
    class Paint paint;
    if (gradient_shader) {
      paint.setColorSource(gradient_shader);
    }
#if CLAY_ENABLE_SKSHAPER
    static_cast<txt::ParagraphSkia*>(paragraph_)
        ->UpdateForegroundPaint(GetTextSize(), paint.gr_object());
#elif CLAY_ENABLE_TTTEXT
    static_cast<txt::ParagraphTTText*>(paragraph_)
        ->UpdateForegroundPaint(GetTextSize(), paint.gr_object());
#endif
  }
}

void TextPainter::SetGradientShaderMap(
    std::map<int, std::shared_ptr<ColorSource>>&& gradient_shader_map,
    std::map<int, std::pair<size_t, size_t>>&& range_map) {
  gradient_shader_map_ = std::move(gradient_shader_map);
  gradient_shader_range_map_ = std::move(range_map);
  is_gradient_dirty_ = true;
}

void TextPainter::SetTextStrokeMap(
    std::unordered_map<int, TextStroke>&& text_stroke_map) {
  text_stroke_map_ = std::move(text_stroke_map);
}

std::vector<TextBox> TextPainter::GetRectsForPlaceholders() {
  if (!paragraph_) {
    return {};
  }
  std::vector<txt::Paragraph::TextBox> boxes =
      paragraph_->GetRectsForPlaceholders();
  std::vector<TextBox> res;
  res.reserve(boxes.size());
  for (const txt::Paragraph::TextBox& box : boxes) {
    res.emplace_back(FloatRect(box.rect));
  }
  return res;
}

std::vector<TextBox> TextPainter::GetRectsForRange(int start, int end,
                                                   RectHeightStyle height_style,
                                                   RectWidthStyle width_style) {
  if (!paragraph_) {
    return {};
  }
  std::vector<txt::Paragraph::TextBox> boxes =
      paragraph_->GetRectsForRange(start, end, height_style, width_style);
  std::vector<TextBox> res;
  res.reserve(boxes.size());
  for (const txt::Paragraph::TextBox& box : boxes) {
    res.emplace_back(FloatRect(box.rect));
  }
  return res;
}

TextPosWithAffinity TextPainter::GetGlyphPositionAtCoordinate(float x,
                                                              float y) {
  if (!paragraph_) {
    return {0, Affinity::kDownstream};
  }
  auto result = paragraph_->GetGlyphPositionAtCoordinate(x, y);
  return {result.position, result.affinity == txt::Paragraph::UPSTREAM
                               ? Affinity::kUpstream
                               : Affinity::kDownstream};
}

double TextPainter::GetLineHeightForPosition(size_t position, Affinity affinity,
                                             size_t* line_number) {
  if (!paragraph_) {
    return 0.0;
  }
  for (const auto& metrics : paragraph_->GetLineMetrics()) {
    if (position > metrics.end_index) {
      continue;
    }
    // Assume metrics are incremental.
    if (position >= metrics.start_index) {
      if (line_number) {
        // Taking into account the position of the cursor when wrapping
        if (position == metrics.end_index &&
            metrics.end_including_newline == metrics.end_index &&
            affinity == Affinity::kDownstream) {
          *line_number = metrics.line_number + 1;
        } else {
          *line_number = metrics.line_number;
        }
      }
      return metrics.height;
    }
    break;
  }
  return 0.0;
}

double TextPainter::GetUpDownLineHeightForPosition(
    TextPosWithAffinity pos_with_affinity, VerticalDirection direction) {
  size_t current_line = 0;
  if (!GetLineHeightForPosition(pos_with_affinity.first,
                                pos_with_affinity.second, &current_line)) {
    // Not valid position.
    return 0.0;
  }

  const auto& metrics = paragraph_->GetLineMetrics();
  auto target_line = direction == VerticalDirection::kUp
                         ? std::max(static_cast<size_t>(0), current_line - 1)
                         : std::min(current_line + 1, metrics.size() - 1);
  if (target_line == current_line) {
    // current_line is the first or last line.
    return 0.0;
  }

  auto find_iter = std::find_if(
      metrics.begin(), metrics.end(), [target_line](const auto& line_metrics) {
        // |line_number| starts from 0.
        return target_line == line_metrics.line_number;
      });
  return find_iter != metrics.end() ? find_iter->height : 0.0;
}

TextRange TextPainter::GetLineRangeForPosition(size_t position) {
  if (!paragraph_) {
    return {};
  }
  for (const auto& metrics : paragraph_->GetLineMetrics()) {
    if (position > metrics.end_index) {
      continue;
    }
    // Assume metrics are incremental.
    if (position >= metrics.start_index) {
      return TextRange{metrics.start_index, metrics.end_index};
    }
    break;
  }
  return {};
}

void TextPainter::Paint(GraphicsContext* context, double x_offset,
                        double y_offset) {
  UpdateGradientIfNeeded(context);
  if (paragraph_) {
#ifndef ENABLE_SKITY
    paragraph_->Paint(context->GetGrCanvas(), x_offset, y_offset);

    if (text_stroke_map_.size() > 0) {
      UpdateTextStrokePaint();
      paragraph_->Paint(context->GetGrCanvas(), x_offset, y_offset);
      UpdateTextFillPaint();
      is_gradient_dirty_ = true;
    }
#else
    txt::ParagraphTTText* paragraph_tt_text =
        static_cast<txt::ParagraphTTText*>(paragraph_);
    paragraph_tt_text->Paint(context->Canvas(), x_offset, y_offset);
#endif  // ENABLE_SKITY
  }
}

size_t TextPainter::GetTextSize() {
  if (paragraph_ == nullptr) {
    return 0;
  }

#if defined(CLAY_ENABLE_SKSHAPER)
  auto* impl = static_cast<skia::textlayout::ParagraphImpl*>(
      (static_cast<txt::ParagraphSkia*>(paragraph_)->paragraph_).get());
  return impl->text().size();
#elif defined(CLAY_ENABLE_TTTEXT)
  auto* impl = static_cast<txt::ParagraphTTText*>(paragraph_);
  return impl->GetTextSize();
#endif
  return 0;
}

void TextPainter::UpdateGradientIfNeeded(GraphicsContext* context) {
  if (!is_gradient_dirty_ || gradient_shader_map_.size() == 0) {
    return;
  }
  is_gradient_dirty_ = false;

  if (paragraph_ == nullptr) {
    return;
  }

  // enable_skity implies enable_tttext
#ifndef ENABLE_SKITY
  std::unordered_map<int, SkPaint> paint_map;
  class Paint paint;
  for (auto gradient_shader : gradient_shader_map_) {
    if (gradient_shader.second) {
      paint.setColorSource(gradient_shader.second);
    }
    paint_map[gradient_shader.first] = paint.gr_object();
  }

#if CLAY_ENABLE_SKSHAPER
  static_cast<txt::ParagraphSkia*>(paragraph_)
      ->UpdateForegroundPaint(paint_map);
#elif CLAY_ENABLE_TTTEXT
  // Currently tttext dont support inline-text set gradient attribute
  static_cast<txt::ParagraphTTText*>(paragraph_)
      ->UpdateForegroundPaint(GetTextSize(), paint.gr_object());
#endif
#else
  std::unordered_map<int, skity::Paint> paint_map;
  class Paint paint;
  for (auto gradient_shader : gradient_shader_map_) {
    if (gradient_shader.second) {
      paint.setColorSource(gradient_shader.second);
    }
    paint_map[gradient_shader.first] = paint.gr_object();

    if (gradient_shader_range_map_.find(gradient_shader.first) !=
        gradient_shader_range_map_.end()) {
      auto& [start, end] = gradient_shader_range_map_.at(gradient_shader.first);
      static_cast<txt::ParagraphTTText*>(paragraph_)
          ->UpdateForegroundPaint(start, end, paint.gr_object());
    }
  }
#endif  // ENABLE_SKITY
}

void TextPainter::UpdateTextFillPaint() {
  std::unordered_map<int, GrPaint> paint_map;
  class Paint paint;
  for (auto text_stroke : text_stroke_map_) {
    paint.setDrawStyle(DrawStyle::kFill);
    paint.setColor(Color(text_stroke.second.fill_color));
    paint_map[text_stroke.first] = paint.gr_object();
  }

#ifndef ENABLE_SKITY
#if CLAY_ENABLE_SKSHAPER
  static_cast<txt::ParagraphSkia*>(paragraph_)
      ->UpdateForegroundPaint(paint_map);
#elif CLAY_ENABLE_TTTEXT
  // Currently tttext dont support inline-text set text stroke attribute
  static_cast<txt::ParagraphTTText*>(paragraph_)
      ->UpdateForegroundPaint(GetTextSize(), paint.gr_object());
#endif
#else
#if CLAY_ENABLE_TTTEXT
  // Currently tttext dont support inline-text set text stroke attribute
  static_cast<txt::ParagraphTTText*>(paragraph_)
      ->UpdateForegroundPaint(GetTextSize(), paint.gr_object());
#endif
#endif  // ENABLE_SKITY
}

void TextPainter::UpdateTextStrokePaint() {
  std::unordered_map<int, GrPaint> paint_map;
  class Paint paint;
  for (auto text_stroke : text_stroke_map_) {
    paint.setDrawStyle(DrawStyle::kStroke);
    paint.setStrokeWidth(text_stroke.second.width);
    paint.setColor(Color(text_stroke.second.stroke_color));
    paint_map[text_stroke.first] = paint.gr_object();
  }

#ifndef ENABLE_SKITY
#if CLAY_ENABLE_SKSHAPER
  static_cast<txt::ParagraphSkia*>(paragraph_)
      ->UpdateForegroundPaint(paint_map);
#elif CLAY_ENABLE_TTTEXT
  // Currently tttext dont support inline-text set text stroke attribute
  static_cast<txt::ParagraphTTText*>(paragraph_)
      ->UpdateForegroundPaint(GetTextSize(), paint.gr_object());
#endif
#else
#if CLAY_ENABLE_TTTEXT
  // Currently tttext dont support inline-text set text stroke attribute
  static_cast<txt::ParagraphTTText*>(paragraph_)
      ->UpdateForegroundPaint(GetTextSize(), paint.gr_object());
#endif
#endif  // ENABLE_SKITY
}

TextRange TextPainter::GetWordBoundary(size_t offset) {
  if (!paragraph_) {
    return TextRange();
  }
  auto range = paragraph_->GetWordBoundary(offset);
  return TextRange(range.start, range.end);
}

std::vector<FloatRect> TextPainter::GetTextLineRects(int start, int end) {
  FML_DCHECK(start < end);
  std::vector<FloatRect> line_rects;
  if (!paragraph_) {
    return line_rects;
  }
  auto line_metrics = paragraph_->GetLineMetrics();
  for (auto metric : line_metrics) {
    if (std::max(start, static_cast<int>(metric.start_index)) <
        std::min(end, static_cast<int>(metric.end_index))) {
      auto text_boxes = paragraph_->GetRectsForRange(
          std::max(start, static_cast<int>(metric.start_index)),
          std::min(end, static_cast<int>(metric.end_index)),
          RectHeightStyle::kMax, RectWidthStyle::kMax);
      if (text_boxes.empty()) {
        return line_rects;
      }
      skity::Rect rect = text_boxes.front().rect;
      for (auto& box : text_boxes) {
        rect.Join(box.rect);
      }
      line_rects.emplace_back(rect);
    }
  }
  return line_rects;
}

}  // namespace clay
