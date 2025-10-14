// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/rendering/render_box.h"

#include <algorithm>

#include "clay/gfx/geometry/float_point.h"
#include "clay/gfx/geometry/float_rect.h"
#include "clay/gfx/image/image.h"
#include "clay/ui/common/background_data.h"
#include "clay/ui/component/css_property.h"
#include "clay/ui/painter/box_painter.h"
#include "clay/ui/painter/image_painter.h"
#include "clay/ui/rendering/renderer.h"

namespace clay {

const char* RenderBox::GetName() const { return "RenderBox"; }

float RenderBox::ClientWidth() const {
  return Width() - BorderLeft() - BorderRight();
}

float RenderBox::ClientHeight() const {
  return Height() - BorderTop() - BorderBottom();
}

float RenderBox::ScrollWidth() const {
  return std::max(ClientWidth(), OverflowRect().MaxX() - BorderLeft());
}

float RenderBox::ScrollHeight() const {
  return std::max(ClientHeight(), OverflowRect().MaxY() - BorderTop());
}

void RenderBox::SetScrollLeft(float scroll_left,
                              bool ignore_repaint /* = false */) {
  scroll_left_ = scroll_left;
  if (!ignore_repaint) {
    MarkNeedsPaint();
  }
}

void RenderBox::SetScrollTop(float scroll_top,
                             bool ignore_repaint /* = false */) {
  scroll_top_ = scroll_top;
  if (!ignore_repaint) {
    MarkNeedsPaint();
  }
}

bool RenderBox::ScrollsOverflowX() const {
  // TODO(jinsong): Support scrolls overflow.
  return false;
}

bool RenderBox::ScrollsOverflowY() const {
  // TODO(jinsong): Support scrolls overflow.
  return false;
}

float RenderBox::MaxScrollHeight() const {
  float max_height = overflow_rect_.height() - PaddingBoxRect().height();
  return std::max(max_height, 0.f);
}

float RenderBox::MaxScrollWidth() const {
  float max_width = overflow_rect_.width() - PaddingBoxRect().width();
  return std::max(max_width, 0.f);
}

void RenderBox::Paint(PaintingContext& context, const FloatPoint& offset) {
  if (!CanDisplay()) {
    return;
  }

  BoxPainter(this).Paint(context.GetGraphicsContext(), offset);
}

void RenderBox::PaintChildren(PaintingContext& context,
                              const FloatPoint& offset) {
  if (!CanDisplay()) {
    return;
  }

  DoPaintChildren(context, offset);
}

FloatPoint RenderBox::GetPaintOffsetForScroll() const {
  FloatPoint offset;
  if (layout_direction_ == DirectionType::kLtr ||
      layout_direction_ == DirectionType::kNormal) {
    offset = FloatPoint(ScrollLeft(), ScrollTop());
  } else {
    offset = FloatPoint(MaxScrollWidth() - ScrollLeft(), ScrollTop());
  }
  // To improve the TextBlob cache hit rate, round the scroll offset here.
  // Because the TextBlob cache only works when the translation between the
  // positionMatrix is integer.
  // See: third_party/skia/src/text/gpu/TextBlob.cpp#can_use_direct
  offset = FloatPoint(renderer_->RoundPixels(offset.x()),
                      renderer_->RoundPixels(offset.y()));
  return offset;
}

void RenderBox::DoPaintChildren(PaintingContext& context,
                                const FloatPoint& offset) {
  auto visitor = [&](RenderObject* child) {
    if (child->IsOverlay() && child->Visible()) {
      renderer_->AddOverlayChild(child);
      // Overlay won't apply offset from parent.
      context.PaintChild(child, child->OffsetInLayer());
    } else {
      FloatPoint layer_offset;
      layer_offset = offset - GetPaintOffsetForScroll();
      context.PaintChild(child, layer_offset + child->OffsetInLayer());
    }
  };
  VisitChildren(visitor);
}

void RenderBox::AddOverflowFromChildren() {
  overflow_rect_ = {};
  for (RenderObject* child = SlowFirstChild(); child != nullptr;
       child = child->NextSibling()) {
    if (!child->IsRenderBounceView()) {
      auto child_rect = child->GetFrameRect();
      child_rect.Expand(child->MarginTop(), child->MarginRight(),
                        child->MarginBottom(), child->MarginLeft());
      overflow_rect_.ExpandToInclude(child_rect, true);
    }
  }

  overflow_rect_.ShiftMaxXEdgeTo(overflow_rect_.MaxX() + PaddingRight());
  overflow_rect_.ShiftMaxYEdgeTo(overflow_rect_.MaxY() + PaddingBottom());

  overflow_rect_.ExpandToInclude(PaddingBoxRect(), true);
}

void RenderBox::SetOverflowRect(const FloatRect& rect) {
  overflow_rect_ = rect;
}

#ifndef NDEBUG
std::string RenderBox::ToString() const {
  std::stringstream ss;
  ss << RenderObject::ToString();
  ss << " scroll_size=(" << ScrollWidth() << "," << ScrollHeight() << ")";
  return ss.str();
}
#endif

}  // namespace clay
