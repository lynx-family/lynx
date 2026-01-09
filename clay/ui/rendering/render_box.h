// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_RENDERING_RENDER_BOX_H_
#define CLAY_UI_RENDERING_RENDER_BOX_H_

#include <optional>
#include <string>

#include "clay/gfx/geometry/float_rect.h"
#include "clay/ui/component/css_property.h"
#include "clay/ui/painter/painting_context.h"
#include "clay/ui/rendering/render_object.h"
#include "third_party/googletest/googletest/include/gtest/gtest_prod.h"  // nogncheck

namespace clay {

// A render object in a 2D Cartesian coordinate system.
class RenderBox : public RenderObject {
 public:
  RenderBox() = default;
  virtual ~RenderBox() = default;

  const char* GetName() const override;

  FloatSize size() const { return FloatSize(Width(), Height()); }

  FloatRect BorderBoxRect() const { return FloatRect({}, size()); }
  FloatRect PaddingBoxRect() const {
    return FloatRect(BorderLeft(), BorderTop(), ClientWidth(), ClientHeight());
  }

  // The content area of the box (excludes padding - and intrinsic padding for
  // table cells, etc... - and border).
  FloatRect ContentBoxRect() const {
    return FloatRect(BorderLeft() + PaddingLeft(), BorderTop() + PaddingTop(),
                     ContentWidth(), ContentHeight());
  }

  // The padding area.
  FloatRect PaddingRect() const {
    FloatRect frame_rect = GetFrameRect();
    return FloatRect(frame_rect.x() + BorderLeft(),
                     frame_rect.y() + BorderTop(), ClientWidth(),
                     ClientHeight());
  }

  // The content area.
  FloatRect ContentRect() const {
    FloatRect frame_rect = GetFrameRect();
    return FloatRect(frame_rect.x() + BorderLeft() + PaddingLeft(),
                     frame_rect.y() + BorderTop() + PaddingTop(),
                     ContentWidth(), ContentHeight());
  }

  FloatRect ClientRect() const {
    FloatRect frame_rect = GetFrameRect();
    return FloatRect(frame_rect.x() + BorderLeft(),
                     frame_rect.y() + BorderTop(), ClientWidth(),
                     ClientHeight());
  }

  float ContentWidth() const {
    return ClientWidth() - PaddingLeft() - PaddingRight();
  }
  float ContentHeight() const {
    return ClientHeight() - PaddingTop() - PaddingBottom();
  }

  // ClientWidth and ClientHeight represent the interior of an object excluding
  // border and scrollbar.  ClientLeft/Top are just the BorderLeftWidth and
  // BorderTopWidth.
  float ClientLeft() const { return BorderLeft(); }
  float ClientTop() const { return BorderTop(); }
  float ClientWidth() const;
  float ClientHeight() const;
  FloatRect ClientBoxRect() const {
    return FloatRect(ClientLeft(), ClientTop(), ClientWidth(), ClientHeight());
  }

  FloatPoint ScrollOffset() const {
    return FloatPoint(scroll_left_, scroll_top_);
  }
  float ScrollLeft() const override { return scroll_left_; }
  float ScrollTop() const override { return scroll_top_; }
  float ScrollWidth() const;
  float ScrollHeight() const;
  void SetScrollLeft(float scroll_left, bool ignore_repaint = false);
  void SetScrollTop(float scroll_top, bool ignore_repaint = false);

  const FloatRect& OverflowRect() const { return overflow_rect_; }

  void ResetOverflowRect() {
    overflow_rect_.SetX(0);
    overflow_rect_.SetY(0);
    overflow_rect_.SetWidth(0);
    overflow_rect_.SetHeight(0);
  }

  bool ScrollsOverflow() const {
    return ScrollsOverflowX() || ScrollsOverflowY();
  }

  bool ScrollsOverflowX() const;
  bool ScrollsOverflowY() const;

  float MaxScrollHeight() const;
  float MaxScrollWidth() const;

  void Paint(PaintingContext& context, const FloatPoint& offset) override;
  void PaintChildren(PaintingContext& context,
                     const FloatPoint& offset) override;

  FloatPoint GetPaintOffsetForScroll() const;

  void AddOverflowFromChildren();
  void SetOverflowRect(const FloatRect& rect);

  void SetLayoutDirection(DirectionType layout_direction) {
    layout_direction_ = layout_direction;
    MarkNeedsPaint();
  }
  DirectionType GetLayoutDirection() { return layout_direction_; }

#ifndef NDEBUG
  std::string ToString() const override;
#endif

 private:
  FRIEND_TEST(RenderBoxTest, Overflow);

  void DoPaintChildren(PaintingContext& context, const FloatPoint& offset);

  float scroll_left_ = 0.f;
  float scroll_top_ = 0.f;

  FloatRect overflow_rect_;
  std::optional<FloatRect> clip_rect_;
  DirectionType layout_direction_ = DirectionType::kLtr;
};

}  // namespace clay

#endif  // CLAY_UI_RENDERING_RENDER_BOX_H_
