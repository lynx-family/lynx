// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/rendering/editable/render_editable.h"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

#include "clay/fml/logging.h"
#include "clay/gfx/geometry/float_rect.h"
#include "clay/gfx/graphics_context.h"
#include "clay/gfx/style/color.h"
#include "clay/ui/common/utils/floating_comparison.h"
#include "clay/ui/component/editable/text_utils.h"
#include "clay/ui/component/text/text_style.h"
#include "clay/ui/painter/painting_context.h"
#include "clay/ui/painter/text_painter.h"

namespace clay {

namespace {

#if defined(OS_MAC) || defined(OS_WIN)
constexpr float kCaretWidth = 1.f;
#else
constexpr float kCaretWidth = 2.f;
#endif
constexpr float kCaretVerticalPreserveSpace = 2.f;
constexpr uint32_t kCaretColor = 0xff000000;
constexpr uint32_t kSelectionColor = 0xff90caf9;  // material blue[200]

}  // namespace

RenderEditable::RenderEditable()
    : painter_(std::make_unique<TextPainter>()), weak_factory_(this) {
  SetPaddingLeft(2);
  SetPaddingRight(2);
}

void RenderEditable::SetSelection(const TextRange& selection,
                                  Affinity selection_affinity) {
  auto text_editing_value = GetTextEditingValue();
  if (text_editing_value.selection() == selection &&
      text_editing_value.SelectionAffinity() == selection_affinity) {
    return;
  }
  text_editing_value.SetSelection(selection);
  text_editing_value.SetSelectionAffinity(selection_affinity);
  SetTextEditingValue(text_editing_value);
  MarkNeedsPaint();
}

void RenderEditable::SetSelection(int base, int extent,
                                  Affinity selection_affinity) {
  SetSelection(TextRange(base, extent), selection_affinity);
}

void RenderEditable::Paint(PaintingContext& context, const FloatPoint& offset) {
  RenderBox::Paint(context, offset);

  auto content_rect = GetFrameRect();
  content_rect.SetLocation({0, 0});
  content_rect.SetWidth(content_rect.width() - HorizontalThickness());
  content_rect.SetHeight(content_rect.height() - VerticalThickness());

  // caret_rect's offset is relative to text.
  FloatRect caret_rect = ComputeCaretRect();
  const auto& text_editing_value = GetTextEditingValue();
  if (need_paint_placeholder_) {
    EnsurePlaceholderCenterInVertical(content_rect, caret_rect);
  }
  EnsureCaretCenterInVertical(content_rect, caret_rect);
  EnsureCaretInVisibleArea(content_rect, &caret_rect);
  ClampContent(content_rect, caret_rect);
  bool show_selection = !text_editing_value.selection().collapsed();

  if (painter_->CanPaint() || display_caret_ || show_selection) {
    GraphicsContext* graphics_context = context.GetGraphicsContext();
    GraphicsContext::AutoRestore auto_canvas(graphics_context, true);
    paint_offset_ = offset + PaintOffset();
    // Translate to the start of content region.
    graphics_context->Translate(paint_offset_.x(), paint_offset_.y());
    graphics_context->ClipRect(
        skity::Rect::MakeWH(static_cast<int>(content_rect.width()),
                            static_cast<int>(content_rect.height())),
        GrClipOp::kIntersect, false);

    // We implement textalign by manipulating the drawing position
    float x_offset = 0;
    if (!is_multiline_) {
      if (text_align_ == TextAlignment::kCenter) {
        x_offset = std::max(
            0.0,
            (ContentWidth() - painter_->GetParagraph()->GetLongestLine()) / 2);
      } else if (text_align_ == TextAlignment::kRight) {
        x_offset = std::max(
            0.0, ContentWidth() - painter_->GetParagraph()->GetLongestLine());
      }
    }

    if (text_editing_value.empty() && need_paint_placeholder_) {
      {
        GraphicsContext::AutoRestore saver(graphics_context, true);
        graphics_context->Translate(
            offset_for_placeholder_visible_.x() + x_offset,
            offset_for_placeholder_visible_.y());
        if (painter_->CanPaint()) {
          painter_->SetWidth(content_rect.width());
          painter_->SetHeight(content_rect.height());
          painter_->Paint(graphics_context);
        }
      }
      graphics_context->Translate(offset_for_caret_visible_.x(),
                                  offset_for_caret_visible_.y());
      if (display_caret_) {
        PaintCaret(graphics_context, caret_rect);
        context.SetWillChangeHint();
      }
    } else {
      graphics_context->Translate(offset_for_caret_visible_.x() + x_offset,
                                  offset_for_caret_visible_.y());
      if (show_selection) {
        auto boxes =
            painter_->GetRectsForRange(text_editing_value.selection().start(),
                                       text_editing_value.selection().end(),
                                       txt::Paragraph::RectHeightStyle::kMax,
                                       txt::Paragraph::RectWidthStyle::kMax);
        PaintSelection(graphics_context, boxes);
      } else if (display_caret_) {
        PaintCaret(graphics_context, caret_rect);
        context.SetWillChangeHint();
      }
      if (painter_->CanPaint()) {
        painter_->SetWidth(content_rect.width());
        painter_->SetHeight(content_rect.height());
        painter_->Paint(graphics_context);
      }
    }
  }

  RenderBox::PaintChildren(context, offset);

  if (controller_) {
    controller_->PostPaint();
  }
}

void RenderEditable::PaintCaret(GraphicsContext* context,
                                const FloatRect& caret_rect) {
  class Paint paint;
  paint.setColor(caret_color_ ? caret_color_->Value() : kCaretColor);
  context->DrawRect(caret_rect, paint);
}

// Input: boxes - list of bounding boxes for each glyph
// Output: res - list of updated bounding boxes for each line
void RenderEditable::UpdateSelectionForMultiLine(
    const std::vector<TextBox>& boxes, std::vector<skity::Rect>& res) {
  // TODO(haochen): RTL support
  if (text_direction_ == TextDirection::kRtl) {
    std::transform(boxes.begin(), boxes.end(), res.begin(),
                   [](TextBox box) -> skity::Rect { return box.rect; });
    return;
  }
  for (auto& box : boxes) {
    // first line
    if (res.empty()) {
      res.emplace_back(box.rect);
      continue;
    }
    auto& last_box = res.back();
    // same line
    if (skity::Rect rect = box.rect; RoughlyEqual(last_box.Top(), rect.Top())) {
      last_box.Join(rect);
      continue;
    }
    // new line
    if (float t = GetFrameRect().MaxX(); last_box.Right() < t) {
      last_box.SetRight(t);
    }
    res.emplace_back(box.rect);
  }
}

void RenderEditable::PaintSelection(GraphicsContext* context,
                                    const std::vector<TextBox>& boxes) {
  class Paint paint;
  paint.setColor(kSelectionColor);
  std::vector<skity::Rect> paint_boxes;
  UpdateSelectionForMultiLine(boxes, paint_boxes);
  for (const auto& box : paint_boxes) {
    context->DrawRect(box, paint);
  }
}

FloatRect RenderEditable::ComputeCaretRect() {
  // Default draw a vertical line.
  float caret_height =
      std::min(GetRoughTextLineHeight(), static_cast<float>(ContentHeight())) -
      2 * CaretVerticalPreserveSpace();
  const FloatRect caret_proto =
      FloatRect(EmptyCaretOffset(), CaretVerticalPreserveSpace(), CaretWidth(),
                caret_height);
  const auto& text_editing_value = GetTextEditingValue();
  int caret_offset = text_editing_value.selection().extent();
  if (text_direction_ == TextDirection::kRtl &&
      text_align_ != TextAlignment::kLeft) {
    caret_offset = caret_offset - 1;
  }
  if (!painter_ || text_editing_value.GetU16Length() == 0) {
    return caret_proto;
  }

  FloatRect caret_rect = caret_proto;
  const auto& text = text_editing_value.GetU16Text();
  // TODO(yulitao): support affinity.
  bool upstream =
      text_editing_value.SelectionAffinity() == Affinity::kUpstream &&
      !TextUtils::IsNewLine(text.at(std::max(0, caret_offset - 1)));
  if (upstream) {
    // If caret is at position 0, there's no upstream glyphs.
    // So need fallback to downstream.
    if (!UpdateCaretRectUpstream(caret_offset, caret_rect)) {
      UpdateCaretRectDownstream(caret_offset, caret_rect);
    }
  } else if (!UpdateCaretRectDownstream(caret_offset, caret_rect)) {
    // If caret is at the end, there's no downstream glyphs. Fallback.
    UpdateCaretRectUpstream(caret_offset, caret_rect);
  }
  return caret_rect;
}

FloatRect RenderEditable::ComputeCaretRectRelativeToCanvas() {
  FloatRect rect = ComputeCaretRect();
  // Clip invisible region to fix coordinates of caret.
  auto correct_rect_x = rect.x() + offset_for_caret_visible_.x();
  auto correct_rect_y = rect.y() + offset_for_caret_visible_.y();
  return FloatRect(correct_rect_x + paint_offset_.x(),
                   correct_rect_y + paint_offset_.y(), rect.width(),
                   rect.height());
}

FloatRect RenderEditable::ComputeComposingRect(TextRange composing_range) {
  auto caret_rect = ComputeCaretRectRelativeToCanvas();
  if (composing_range.collapsed()) {
    return caret_rect;
  }
  auto boxes = painter_->GetRectsForRange(composing_range.start(),
                                          composing_range.end());
  if (boxes.size() == 0) {
    return caret_rect;
  }
  FloatRect rect = boxes[0].rect;
  for (auto& box : boxes) {
    rect.ExpandToInclude(box.rect);
  }
  float x_offset = 0;
  if (!is_multiline_) {
    if (text_align_ == TextAlignment::kCenter) {
      x_offset = std::max(
          0.0,
          (ContentWidth() - painter_->GetParagraph()->GetLongestLine()) / 2);
    } else if (text_align_ == TextAlignment::kRight) {
      x_offset = std::max(
          0.0, ContentWidth() - painter_->GetParagraph()->GetLongestLine());
    }
  }
  return FloatRect(
      rect.x() + x_offset + paint_offset_.x() + offset_for_caret_visible_.x(),
      rect.y() + paint_offset_.y() + offset_for_caret_visible_.y(),
      rect.width(), rect.height());
}

void RenderEditable::EnsurePlaceholderCenterInVertical(const FloatRect& content,
                                                       const FloatRect& caret) {
  // centering the text and  cursor in the input type
  if (!is_multiline_) {
    FloatPoint offset_for_placeholder_visible(
        0, (content.height() - GetPlaceholderLineHeight()) / 2.0f);
    offset_for_placeholder_visible_.MoveBy(
        last_offset_for_placeholder_visible_);
    offset_for_placeholder_visible_.MoveBy(offset_for_placeholder_visible);
    last_offset_for_placeholder_visible_ =
        FloatPoint(-offset_for_placeholder_visible.x(),
                   -offset_for_placeholder_visible.y());
  }
}

void RenderEditable::EnsureCaretCenterInVertical(const FloatRect& content,
                                                 const FloatRect& caret) {
  // centering the text and  cursor in the input type
  if (!is_multiline_) {
    FloatPoint offset_for_caret_visible(
        0, (content.height() - GetRoughTextLineHeight()) / 2.0f);
    offset_for_caret_visible_.MoveBy(last_offset_for_caret_visible_);
    offset_for_caret_visible_.MoveBy(offset_for_caret_visible);
    last_offset_for_caret_visible_ = FloatPoint(-offset_for_caret_visible.x(),
                                                -offset_for_caret_visible.y());
  }
}

void RenderEditable::EnsureCaretInVisibleArea(const FloatRect& content,
                                              FloatRect* caret) {
  FloatRect current = FloatRect(content.x() - offset_for_caret_visible_.x(),
                                content.y() - offset_for_caret_visible_.y(),
                                content.width(), content.height());
  if (!is_multiline_) {
    // move caret to visible
    if (caret->x() < current.x()) {
      offset_for_caret_visible_.Move(
          std::max(0.f, std::min(current.x() - caret->x(),
                                 current.MaxX() - caret->MaxX())),
          0.f);
    } else if (caret->MaxX() > current.MaxX()) {
      offset_for_caret_visible_.Move(
          std::min(0.f, std::max(current.x() - caret->x(),
                                 current.MaxX() - caret->MaxX())),
          0.f);
    }
    if (caret->y() < current.y()) {
      offset_for_caret_visible_.Move(
          0.f, std::max(0.f, std::min(current.y() - caret->y(),
                                      current.MaxY() - caret->MaxY())));
    } else if (caret->MaxY() > current.MaxY()) {
      offset_for_caret_visible_.Move(
          0.f, std::min(0.f, std::max(current.y() - caret->y(),
                                      current.MaxY() - caret->MaxY())));
    }
  } else {
    if (caret->x() > current.MaxX() - caret->width()) {
      caret->SetX(current.MaxX() - caret->width());
    }
  }
}

void RenderEditable::ClampContent(const FloatRect& content,
                                  const FloatRect& caret) {
  if (is_multiline_) {
    return;
  }
  // [offset.x(), offset.x() + content.width()] in [-caret.width(),
  // paragraph.width() + caret.width()]
  float offset_x = std::max(
      std::clamp(
          -offset_for_caret_visible_.x() + content.width(), -caret.width(),
          static_cast<float>(painter_->GetParagraph()->GetMaxIntrinsicWidth() +
                             caret.width())) -
          content.width(),
      -caret.width());
  offset_for_caret_visible_.SetX(-offset_x);
}

// Update offset onto |caret_rect|. If update failed, nothing changes.
bool RenderEditable::UpdateCaretRectUpstream(int caret_offset,
                                             FloatRect& caret_rect) {
  // TODO(yulitao): quick reject
  if (caret_offset == 0) {
    return false;
  }
  FML_CHECK(painter_);

  auto text = GetTextEditingValue().GetU16Text();
  int length = text.length();
  size_t prev_code_unit = text.at(std::clamp(caret_offset - 1, 0, length - 1));
  // If the upstream character is a newline, cursor is at start of next line
  const int NEWLINE_CODE_UNIT = 10;
  bool needs_search = TextUtils::IsHighSurrogate(prev_code_unit) ||
                      TextUtils::IsLowSurrogate(prev_code_unit) ||
                      text.at(std::clamp(caret_offset, 0, length - 1)) ==
                          TextUtils::kZWJUtf16 ||
                      TextUtils::IsUnicodeDirectionality(prev_code_unit);
  int graphemeClusterLength = needs_search ? 2 : 1;
  std::vector<TextBox> boxes;
  while (boxes.empty()) {
    int prev_offset = std::max(caret_offset - graphemeClusterLength, 0);
    // As code point is not the minimum draw unit, so given 1 or 2 code point
    // there may not be a complete box.
    boxes = painter_->GetRectsForRange(prev_offset, caret_offset,
                                       RectHeightStyle::kMax);
    if (boxes.empty()) {
      if (!needs_search && prev_code_unit == NEWLINE_CODE_UNIT) {
        // Only perform one iteration if no search is required.
        break;
      }
      if (prev_offset <= 0) {
        // Stop iterating when beyond the max length of the text.
        break;
      }
      graphemeClusterLength *= 2;
      continue;
    }
  }

  if (boxes.empty()) {
    return false;
  }

  // Get the closest box to caret.
  const auto& box = boxes[boxes.size() - 1];
  // TODO(yulitao): assume LTR. fix it after text directional is done.
  UpdateCaretRectBesideBox(caret_rect, box.rect, true);
  return true;
}

bool RenderEditable::UpdateCaretRectDownstream(int caret_offset,
                                               FloatRect& caret_rect) {
  FML_CHECK(painter_);

  auto text = GetTextEditingValue().GetU16Text();
  size_t next_code_unit =
      text.at(std::clamp(caret_offset, 0, static_cast<int>(text.length() - 1)));
  bool needs_search = TextUtils::IsHighSurrogate(next_code_unit) ||
                      TextUtils::IsLowSurrogate(next_code_unit) ||
                      next_code_unit == TextUtils::kZWJUtf16 ||
                      TextUtils::IsUnicodeDirectionality(next_code_unit);
  int graphemeClusterLength = needs_search ? 2 : 1;
  std::vector<TextBox> boxes;
  while (boxes.empty()) {
    // TODO(yulitao): First check whether next_offset exceeds length.
    size_t next_offset =
        std::min(static_cast<size_t>(caret_offset + graphemeClusterLength),
                 text.length());
    boxes = painter_->GetRectsForRange(caret_offset, next_offset,
                                       RectHeightStyle::kStrut);
    if (boxes.empty()) {
      if (!needs_search) {
        // Only perform one iteration if no search is required.
        break;
      }
      if (next_offset >= text.length()) {
        // Stop iterating when beyond the max length of the text.
        break;
      }
      // Multiply by two to log(n) time cover the entire text span. This allows
      // faster discovery of very long clusters and reduces the possibility
      // of certain large clusters taking much longer than others, which can
      // cause jank.
      graphemeClusterLength *= 2;
      continue;
    }
  }

  if (boxes.empty()) {
    return false;
  }

  const auto& box = boxes[0];
  UpdateCaretRectBesideBox(caret_rect, box.rect, false);
  return true;
}

// Get line height for current caret offset.
float RenderEditable::GetRoughTextLineHeight() {
  auto line_height = rough_text_height_;
  if (!line_height) {
    return default_line_height_;
  }
  return line_height;
}

float RenderEditable::GetPlaceholderLineHeight() {
  auto placeholder_line_height = placeholder_line_height_;
  if (!placeholder_line_height) {
    return default_line_height_;
  }
  return placeholder_line_height;
}

void RenderEditable::SetMaxLines(uint32_t max_lines) { max_lines_ = max_lines; }

float RenderEditable::CaretWidth() const {
  return renderer_->ConvertFrom<kPixelTypeLogical>(kCaretWidth);
}

float RenderEditable::CaretVerticalPreserveSpace() const {
  return renderer_->ConvertFrom<kPixelTypeLogical>(kCaretVerticalPreserveSpace);
}

void RenderEditable::SelectWord(FloatPoint point) {
  point.Move(-PaddingLeft() - BorderLeft(), -PaddingTop() - BorderTop());
  point.Move(-offset_for_caret_visible_.x(), -offset_for_caret_visible_.y());
  TextPosWithAffinity pair =
      painter_->GetGlyphPositionAtCoordinate(point.x(), point.y());
  const auto& word_range = painter_->GetWordBoundary(pair.first);
  if (!word_range.collapsed()) {
    SetSelection(word_range.start(), word_range.end(), pair.second);
  }
}

void RenderEditable::SelectLine(FloatPoint point) {
  point.Move(-PaddingLeft() - BorderLeft(), -PaddingTop() - BorderTop());
  point.Move(-offset_for_caret_visible_.x(), -offset_for_caret_visible_.y());
  TextPosWithAffinity pair =
      painter_->GetGlyphPositionAtCoordinate(point.x(), point.y());
  const auto& line_range = painter_->GetLineRangeForPosition(pair.first);
  if (!line_range.collapsed()) {
    SetSelection(line_range.start(), line_range.end());
  }
}

void RenderEditable::UpdateCaretByCoordinate(const FloatPoint& point,
                                             bool is_select) {
  FloatPoint pointer_in_paragraph = point;
  pointer_in_paragraph.Move(-PaddingLeft() - BorderLeft(),
                            -PaddingTop() - BorderTop());
  pointer_in_paragraph.Move(-offset_for_caret_visible_.x(),
                            -offset_for_caret_visible_.y());
  UpdateCaretByParagraph(pointer_in_paragraph, is_select);
}

void RenderEditable::UpdateCaretByParagraph(const FloatPoint& point,
                                            bool is_select) {
  for (const auto& box : painter_->GetRectsForPlaceholders()) {
    if (box.rect.Contains(point)) {
      // TODO(yulitao): Currently editable doesn't support placeholders.
      return;
    }
  }
  // Because we offset the drawing, clicks also need to be processed.
  float x_offset = 0;
  if (!GetTextEditingValue().empty() && !is_multiline_) {
    if (text_align_ == TextAlignment::kCenter) {
      x_offset = std::max(
          0.0,
          (ContentWidth() - painter_->GetParagraph()->GetLongestLine()) / 2);
    } else if (text_align_ == TextAlignment::kRight) {
      x_offset = std::max(
          0.0, ContentWidth() - painter_->GetParagraph()->GetLongestLine());
    }
  }
  TextPosWithAffinity pair =
      painter_->GetGlyphPositionAtCoordinate(point.x() - x_offset, point.y());
  // TODO(yulitao): Deal with affinity when support RTL.
  if (is_select) {
    SetSelection(
        TextRange(GetTextEditingValue().selection().base(), pair.first),
        pair.second);
  } else {
    SetSelection(TextRange(pair.first), pair.second);
  }
}

void RenderEditable::MoveCaretUpDown(VerticalDirection direction,
                                     bool key_combination) {
  auto height = painter_->GetUpDownLineHeightForPosition(
      std::make_pair(
          static_cast<size_t>(GetTextEditingValue().selection().extent()),
          text_editing_controller_->GetValue().SelectionAffinity()),
      direction);
  bool is_select = !GetTextEditingValue().selection().collapsed();
  if (!height && !is_select) {
    // No up or down line.
    return;
  }
  FloatRect caret_rect;
  if (is_select && !key_combination) {
    auto up_position = std::min(GetTextEditingValue().selection().base(),
                                GetTextEditingValue().selection().extent());
    float caret_height = std::min(GetRoughTextLineHeight(),
                                  static_cast<float>(ContentHeight())) -
                         2 * CaretVerticalPreserveSpace();
    caret_rect = FloatRect(EmptyCaretOffset(), CaretVerticalPreserveSpace(),
                           CaretWidth(), caret_height);
    FloatRect down_rect = caret_rect;
    UpdateCaretRectDownstream(up_position, caret_rect);
    if (direction == VerticalDirection::kDown) {
      auto down_position = std::max(GetTextEditingValue().selection().base(),
                                    GetTextEditingValue().selection().extent());
      UpdateCaretRectUpstream(down_position, down_rect);
      caret_rect.SetY(down_rect.y());
    }
  } else {
    caret_rect = ComputeCaretRect();
  }
  FloatPoint new_position(caret_rect.x(), 0.f);
  if (direction == VerticalDirection::kUp) {
    new_position.SetY(caret_rect.y() - height / 2);
  } else {
    new_position.SetY(caret_rect.MaxY() + height / 2);
  }
  UpdateCaretByParagraph(new_position, key_combination);
}

void RenderEditable::SetCaretDisplay(bool display) {
  if (display_caret_ != display) {
    display_caret_ = display;
    MarkNeedsPaint();
  }
}

void RenderEditable::SetDisabled(bool disabled) {
  if (disabled_ != disabled) {
    disabled_ = disabled;
    MarkNeedsPaint();
  }
}

void RenderEditable::SetCaretColor(const Color& color) {
  caret_color_ = color;
  MarkNeedsPaint();
}

float RenderEditable::EmptyCaretOffset() const {
  float empty_caret_offset = 0.0f;
  switch (text_align_) {
    case TextAlignment::kLeft:
      empty_caret_offset = 0.0f;
      break;
    case TextAlignment::kCenter:
      empty_caret_offset = ContentWidth() / 2.0;
      break;
    case TextAlignment::kRight:
      empty_caret_offset = ContentWidth() - CaretWidth();
      break;
    // TODO(wangyanyi): Support textalign's start, end, justify attributes
    case TextAlignment::kStart:
      break;
    case TextAlignment::kEnd:
      break;
    case TextAlignment::kJustify:
      break;
  }
  return empty_caret_offset;
}

void RenderEditable::UpdateCaretRectBesideBox(FloatRect& caret_rect,
                                              const FloatRect& box_nearby,
                                              bool caret_after_box) {
  float box_nearby_height =
      std::min(box_nearby.height(), static_cast<float>(ContentHeight()));
  caret_rect.SetHeight(box_nearby_height - CaretVerticalPreserveSpace() * 2);
  caret_rect.SetY(box_nearby.y() + CaretVerticalPreserveSpace());
  if (caret_after_box) {
    caret_rect.SetX(box_nearby.MaxX());
  } else {
    caret_rect.SetX(box_nearby.x());
  }
}

}  // namespace clay
