// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/rendering/text/render_text.h"

#include <algorithm>
#include <map>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

#include "clay/gfx/geometry/float_point.h"
#include "clay/gfx/geometry/float_rect.h"
#include "clay/gfx/rendering_backend.h"
#include "clay/gfx/style/color_source.h"
#include "clay/gfx/style/tile_mode.h"
#include "clay/ui/common/text_input_type_traits.h"
#include "clay/ui/component/text/inline_emoji_bitmap.h"
#include "clay/ui/component/text/text_style.h"
#include "clay/ui/painter/text_painter.h"
#include "clay/ui/rendering/renderer.h"

namespace clay {

namespace {

constexpr uint32_t kSelectionColor = 0x402196F3;  // material blue[200]

}  // namespace

RenderText::RenderText() : painter_(std::make_unique<TextPainter>()) {}

RenderText::~RenderText() { paragraph_ = nullptr; }

const char* RenderText::GetName() const { return "RenderText"; }

void RenderText::SetParagraph(txt::Paragraph* paragraph,
                              const std::u16string& text) {
  paragraph_ = paragraph;
  text_ = text;
  painter_->SetParagraph(paragraph_);
  MarkNeedsPaint();
}

void RenderText::SetGradient(const std::optional<Gradient>& gradient) {
  painter_->SetGradient(gradient);
  MarkNeedsPaint();
}

void RenderText::SetGradientShaderMap(
    std::map<int, std::shared_ptr<ColorSource>>&& gradient_shader_map,
    std::map<int, std::pair<size_t, size_t>>&& range_map) {
  painter_->SetGradientShaderMap(std::move(gradient_shader_map),
                                 std::move(range_map));
  MarkNeedsPaint();
}

void RenderText::SetTextStrokeMap(
    std::unordered_map<int, TextStroke>&& text_stroke_map) {
  painter_->SetTextStrokeMap(std::move(text_stroke_map));
  MarkNeedsPaint();
}

void RenderText::SetInlineEmojiInfo(
    std::vector<InlineEmojiInfo> inline_emoji_info) {
  inline_emojis_.clear();
  for (auto& info : inline_emoji_info) {
    auto image = CreateInlineEmojiGraphicsImage(info.bitmap);
    if (info.placeholder_id >= 0 && image) {
      inline_emojis_.emplace(info.placeholder_id,
                             InlineEmojiRenderInfo{std::move(image)});
    }
  }
  MarkNeedsPaint();
}

bool RenderText::IsInlineEmojiPlaceholder(int placeholder_id) const {
  return inline_emojis_.find(placeholder_id) != inline_emojis_.end();
}

void RenderText::Paint(PaintingContext& context, const FloatPoint& offset) {
  if (HasBackgroundClipText() && painter_->CanPaint()) {
    auto painter = [this](PaintingContext& ctx,
                          const FloatPoint& layer_offset) {
      RenderBox::Paint(ctx, layer_offset);
      // Paint inline children background first.
      RenderBox::PaintChildren(ctx, layer_offset);
    };

    GraphicsContext child_context(
        context.GetGraphicsContext()->GetUnrefQueue());
    skity::Rect rect = skity::Rect::MakeXYWH(0, 0, Width(), Height());
    child_context.BeginRecording(rect);
    FloatPoint paint_offset = offset + PaintOffset();
    // Draw text into a mask that can be used to clip background.
    PaintText(&child_context, paint_offset);
    auto picture = child_context.FinishRecording();
    // TODO(Jinsong): Generate a Lazy PaintImage to avoid blocking.
    auto image = renderer_->renderer_client()->MakeRasterSnapshot(
        picture->picture()->raw(),
        skity::Vec2(ContentWidth(), ContentHeight()));

    if (image) {
      auto gradient_shader = std::make_shared<ImageColorSource>(
          image, TileMode::kDecal, TileMode::kDecal);
      context.PushShaderMask(gradient_shader,
                             FloatRect(paint_offset.x(), paint_offset.y(),
                                       ClientWidth(), ClientHeight()),
                             BlendMode::kDstIn, offset, painter);
      return;
    }
  }
  // Draw text as normal.
  RenderBox::Paint(context, offset);
  // Paint inline children background first.
  RenderBox::PaintChildren(context, offset);
  if (painter_->CanPaint()) {
    FloatPoint paint_offset = offset + PaintOffset();
    GraphicsContext* graphics_context = context.GetGraphicsContext();
    PaintText(graphics_context, paint_offset);
  }
}

void RenderText::PaintText(GraphicsContext* graphics_context,
                           const FloatPoint& offset) {
  GraphicsContext::AutoRestore saver(graphics_context, true);
  graphics_context->Translate(offset.x(), offset.y());
  bool needs_clip_x = Overflow() == CSSProperty::OVERFLOW_Y ||
                      Overflow() == CSSProperty::OVERFLOW_HIDDEN;
  bool needs_clip_y = Overflow() == CSSProperty::OVERFLOW_X ||
                      Overflow() == CSSProperty::OVERFLOW_HIDDEN;
  if (!needs_clip_x && needs_clip_y) {
    skity::Rect rect = skity::Rect::MakeXYWH(
        -renderer_->GetFrameSize().width(), 0,
        2 * renderer_->GetFrameSize().width(), ContentHeight());
    graphics_context->ClipRect(rect, GrClipOp::kIntersect, false);
  } else if (needs_clip_x && !needs_clip_y) {
    skity::Rect rect = skity::Rect::MakeXYWH(
        0, -renderer_->GetFrameSize().height(), ContentWidth(),
        2 * renderer_->GetFrameSize().height());
    graphics_context->ClipRect(rect, GrClipOp::kIntersect, false);
  } else if (needs_clip_x && needs_clip_y) {
    skity::Rect rect =
        skity::Rect::MakeXYWH(0, 0, ContentWidth(), ContentHeight());
    graphics_context->ClipRect(rect, GrClipOp::kIntersect, false);
  }
  painter_->SetWidth(ContentWidth());
  painter_->SetHeight(ContentHeight());
  double x_offset = 0;
  // Not aligned with web behavior in bidirectional text but consistent
  // with lynx behavior
  if (text_paint_align_ == TextAlignment::kCenter) {
    x_offset =
        std::max(0.0, (ContentWidth() - paragraph_->GetLongestLine()) / 2);
  } else if (text_paint_align_ == TextAlignment::kRight) {
    x_offset = std::max(0.0, ContentWidth() - paragraph_->GetLongestLine());
  } else {
    FML_DCHECK(text_paint_align_ == TextAlignment::kLeft);
  }
  if (HasColorRasterAnimation()) {
    graphics_context->Canvas()->OnDrawDynamicTextBlobsStart();
    painter_->Paint(graphics_context, x_offset, line_spacing_offset_);
    PaintInlineEmojis(graphics_context, x_offset, line_spacing_offset_);
    graphics_context->Canvas()->OnDrawDynamicTextBlobsEnd();
  } else {
    painter_->Paint(graphics_context, x_offset, line_spacing_offset_);
    PaintInlineEmojis(graphics_context, x_offset, line_spacing_offset_);
  }
  if (select_end_ != select_start_) {
    PaintSelection(graphics_context);
  }
}

void RenderText::PaintInlineEmojis(GraphicsContext* graphics_context,
                                   double x_offset, double y_offset) {
  if (!paragraph_ || inline_emojis_.empty()) {
    return;
  }
  class Paint paint;
  paint.setAntiAlias(false);
  for (const auto& box : paragraph_->GetRectsForPlaceholders()) {
    auto it = inline_emojis_.find(static_cast<int>(box.placeholder_id));
    if (it == inline_emojis_.end() || !it->second.image) {
      continue;
    }
    auto dst = box.rect;
    dst.Offset(x_offset, y_offset);
    graphics_context->DrawImageRect(it->second.image, dst,
                                    SAMPLING_OPTIONS(FilterMode::kNearest, 0),
                                    &paint);
  }
}

void RenderText::SetSelection(const TextRange& range) {
  select_start_ = range.start();
  select_end_ = range.end();
  if (selection_changed_callback_) {
    selection_changed_callback_(select_start_, select_end_);
  }
  pre_select_end_ = select_end_;
  MarkNeedsPaint();
}

void RenderText::SetAllSelection() {
  SetSelection(TextRange(0, text_.length()));
}

void RenderText::PaintSelection(GraphicsContext* context) {
  class Paint paint;
  paint.setColor(kSelectionColor);
  auto text_boxes =
      painter_->GetRectsForRange(std::min(select_start_, select_end_),
                                 std::max(select_start_, select_end_),
                                 RectHeightStyle::kMax, RectWidthStyle::kMax);
  clay::GrPath path;
  for (auto box : text_boxes) {
    PATH_ADD_RECT(path, box.rect);
  }
  context->DrawPath(path, paint);
}

std::u16string RenderText::GetSelectionString() const {
  return text_.substr(std::min(select_start_, select_end_),
                      std::abs(select_end_ - select_start_));
}

std::vector<Point> RenderText::GetPointsFromRangeSelection(
    int select_start, int select_end) const {
  if (select_start == select_end) {
    return std::vector<Point>{};
  } else {
    std::vector<TextBox> boxes = painter_->GetRectsForRange(
        std::min(select_start, select_end), std::max(select_start, select_end));
    if (boxes.empty()) {
      return std::vector<Point>();
    }
    // TODO(wangyanyi) now it is assumed textdirection is 'left'
    return std::vector<Point>{
        Point(boxes.front().GetLeft(), boxes.front().GetBottom()),
        Point(boxes.back().GetRight(), boxes.back().GetBottom())};
  }
}

TextBox RenderText::GetEndTextPositionTopAndBottom() const {
  std::vector<TextBox> boxes;
  if (select_start_ <= select_end_) {
    boxes = painter_->GetRectsForRange(select_end_ - 1, select_end_);
  } else {
    boxes = painter_->GetRectsForRange(select_end_, select_end_ + 1);
  }

  if (boxes.empty()) {
    return TextBox(FloatRect());
  }
  return boxes.front();
}

TextBox RenderText::GetStartTextPositionTopAndBottom() const {
  std::vector<TextBox> boxes;
  if (select_start_ <= select_end_) {
    boxes = painter_->GetRectsForRange(select_start_, select_start_ + 1);
  } else {
    boxes = painter_->GetRectsForRange(select_start_ - 1, select_start_);
  }
  if (boxes.empty()) {
    TextBox(FloatRect());
  }
  return boxes.front();
}

TextBox RenderText::GetLeftTextBox() {
  std::vector<TextBox> boxes;
  boxes = painter_->GetRectsForRange(std::min(select_start_, select_end_),
                                     std::min(select_start_, select_end_) + 1);
  if (boxes.empty()) {
    TextBox(FloatRect());
  }
  return boxes.front();
}

TextBox RenderText::GetRightTextBox() {
  std::vector<TextBox> boxes;
  boxes = painter_->GetRectsForRange(std::max(select_start_, select_end_) - 1,
                                     std::max(select_start_, select_end_));
  if (boxes.empty()) {
    TextBox(FloatRect());
  }
  return boxes.back();
}

std::vector<FloatRect> RenderText::GetTextLineRects(int start, int end) {
  return painter_->GetTextLineRects(start, end);
}

FloatRect RenderText::GetTextBoundingRect(
    int start, int end, const std::vector<FloatRect>& line_rect) {
  if (line_rect.empty()) {
    return FloatRect();
  }

  FloatRect result = line_rect.front();

  for (auto rect : line_rect) {
    result.ExpandToInclude(rect);
  }
  return result;
}

}  // namespace clay
