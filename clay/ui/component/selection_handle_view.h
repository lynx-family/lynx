// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_SELECTION_HANDLE_VIEW_H_
#define CLAY_UI_COMPONENT_SELECTION_HANDLE_VIEW_H_

#include <utility>

#include "clay/gfx/geometry/float_point.h"
#include "clay/gfx/geometry/float_rect.h"
#include "clay/gfx/geometry/float_size.h"
#include "clay/ui/component/base_view.h"
#include "clay/ui/gesture/drag_gesture_recognizer.h"
#include "clay/ui/painter/text_painter.h"

namespace clay {

enum TextSelectionHandleType { kLeft, kRight };

class SelectionHandleView : public WithTypeInfo<SelectionHandleView, BaseView> {
 public:
  SelectionHandleView(PageView* page_view, TextSelectionHandleType type);

  ~SelectionHandleView();

  void SetHandleType(TextSelectionHandleType type) { type_ = type; }
  TextSelectionHandleType GetHandleType() { return type_; }

  FloatSize GetHandleSize(double text_line_height);

  using HandleBarFunctionListener = std::function<void(
      const FloatPoint& position, SelectionHandleView* view)>;
  void SetHandleMove(HandleBarFunctionListener&& handle_bar_function) {
    handle_bar_function_ = std::move(handle_bar_function);
  }

  void BuildSelectionHandle(float line_height, FloatPoint offset);

  float ProcessHandlePos(FloatPoint point, TextBox left_box, TextBox right_box);

  void UpdatePosWithScroll(FloatPoint delta);

  void SetScrollOffset(FloatPoint offset) { scroll_offset_ = offset; }

 private:
  // Helper function to create the handle bar path with a circle and a line
  GrPath CreateHandlePath(const FloatRect& circle_rect,
                          const FloatRect& line_rect);

  float selection_handle_radius_ = 0;
  float selection_handle_overlap_ = 0;
  double half_stroke_width_ = 0;
  float origin_top_ = 0;
  float origin_left_ = 0;
  FloatPoint scroll_offset_;
  HandleBarFunctionListener handle_bar_function_;
  DragGestureRecognizer* drag_recognizer_;
  TextSelectionHandleType type_;
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_SELECTION_HANDLE_VIEW_H_
