// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/selection_handle_view.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "clay/gfx/geometry/float_rect.h"
#include "clay/gfx/geometry/float_size.h"
#include "clay/gfx/style/color.h"
#include "clay/ui/component/page_view.h"
#include "clay/ui/component/selection_handle_view.h"
#include "clay/ui/rendering/render_box.h"

namespace clay {

constexpr float kSelectionHandleRadius = 6;
constexpr float kSelectionHandleOverlap = 1.5;
constexpr float kHalfStrokeWidth = 1.0;

SelectionHandleView::SelectionHandleView(PageView* page_view,
                                         TextSelectionHandleType type)
    : WithTypeInfo(-1, "handle_bar_view", std::make_unique<RenderBox>(),
                   page_view) {
  type_ = type;
  selection_handle_radius_ = FromLogical(kSelectionHandleRadius);
  selection_handle_overlap_ = FromLogical(kSelectionHandleOverlap);
  half_stroke_width_ = FromLogical(kHalfStrokeWidth);
  auto drag_recognizer =
      std::make_unique<DragGestureRecognizer>(page_view_->gesture_manager());
  drag_recognizer_ = drag_recognizer.get();
  drag_recognizer_->SetDragUpdateCallback(
      [this](const FloatPoint& point, const FloatSize& delta) {
        handle_bar_function_(point, this);
      });
  AddGestureRecognizer(std::move(drag_recognizer));
}

SelectionHandleView::~SelectionHandleView() {
  RemoveGestureRecognizer(drag_recognizer_);
  drag_recognizer_ = nullptr;
}

FloatSize SelectionHandleView::GetHandleSize(double text_line_height) {
  return FloatSize(selection_handle_radius_ * 2,
                   text_line_height + selection_handle_radius_ * 2 -
                       selection_handle_overlap_);
}

// Helper method to create path with an oval and a rectangle
GrPath SelectionHandleView::CreateHandlePath(const FloatRect& oval_rect,
                                             const FloatRect& rect) {
  GrPath path;
#ifdef ENABLE_SKITY
  path.AddOval(oval_rect);
  path.AddRect(rect);
#else
  path.addOval(SkRect::MakeLTRB(oval_rect.left(), oval_rect.top(),
                                oval_rect.right(), oval_rect.bottom()));
  path.addRect(
      SkRect::MakeLTRB(rect.left(), rect.top(), rect.right(), rect.bottom()));
#endif
  return path;
}

void SelectionHandleView::BuildSelectionHandle(float line_height,
                                               FloatPoint offset) {
  switch (type_) {
    case TextSelectionHandleType::kLeft: {
      // Left handle: circle at top, line extends downward
      FloatRect circle_rect(0, 0, 2 * selection_handle_radius_,
                            2 * selection_handle_radius_);

      FloatRect line_rect(
          selection_handle_radius_ - half_stroke_width_,
          2 * selection_handle_radius_ - selection_handle_overlap_,
          2 * half_stroke_width_, line_height + selection_handle_overlap_);

      // Create path and position the handle
      GrPath handle_path = CreateHandlePath(circle_rect, line_rect);
      render_object()->SetClipPath(handle_path);

      // Position the left handle (circle at top aligns with text line top)
      SetY(offset.y() - 2 * selection_handle_radius_);
      SetX(offset.x() - half_stroke_width_ - selection_handle_radius_);
      break;
    }
    case TextSelectionHandleType::kRight: {
      // Right handle: circle at bottom, line extends upward
      FloatRect circle_rect(0, line_height - selection_handle_overlap_,
                            2 * selection_handle_radius_,
                            2 * selection_handle_radius_);

      FloatRect line_rect(selection_handle_radius_ - half_stroke_width_,
                          0,  // Start line from top of handle area
                          2 * half_stroke_width_,
                          line_height + selection_handle_overlap_);

      // Create path and position the handle
      GrPath handle_path = CreateHandlePath(circle_rect, line_rect);
      render_object()->SetClipPath(handle_path);

      // Position the right handle (circle at bottom aligns with text line
      // bottom)
      SetY(offset.y());
      SetX(offset.x() + half_stroke_width_ - selection_handle_radius_);
      break;
    }
  }

  // Common setup for both handle types
  origin_top_ = Top();
  origin_left_ = Left();
  render_object()->SetBackgroundColor(Color::kBlue());

  // Set fixed dimensions for the handle
  SetWidth(2 * selection_handle_radius_);
  SetHeight(2 * selection_handle_radius_ + line_height);
}

float SelectionHandleView::ProcessHandlePos(FloatPoint point, TextBox left_box,
                                            TextBox right_box) {
  if (type_ == TextSelectionHandleType::kLeft) {
    return std::min<float>(0, point.y() - left_box.GetTop());
  } else if (type_ == TextSelectionHandleType::kRight) {
    return std::max<float>(0, point.y() - right_box.GetBottom());
  }
  return 0;
}

void SelectionHandleView::UpdatePosWithScroll(FloatPoint delta) {
  origin_top_ += delta.y();
  origin_left_ += delta.x();
  SetY(origin_top_);
  SetX(origin_left_);
  render_object()->MarkNeedsPaint();
}

}  // namespace clay
