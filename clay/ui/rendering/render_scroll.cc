// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/rendering/render_scroll.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "clay/flow/animation/scroll_offset_animation.h"
#include "clay/gfx/style/color_source.h"
#include "clay/ui/compositing/pending_offset_layer.h"
#include "clay/ui/compositing/pending_scroll_offset_layer.h"

namespace clay {
namespace {

std::shared_ptr<ColorSource> CreateFadingEdgeMask(const FloatRect& rect,
                                                  ScrollDirection direction,
                                                  float edge_length,
                                                  bool fade_start,
                                                  bool fade_end) {
  const float length = std::min(
      edge_length, direction == ScrollDirection::kVertical ? rect.height() / 2
                                                           : rect.width() / 2);
  if (length <= 0 || (!fade_start && !fade_end)) {
    return nullptr;
  }

  const float extent =
      direction == ScrollDirection::kVertical ? rect.height() : rect.width();
  const float start_stop = length / extent;
  const float end_stop = 1.f - start_stop;
  const Color transparent = Color::kTransparent();
  const Color opaque = Color::kWhite();
  const Color colors[] = {fade_start ? transparent : opaque, opaque, opaque,
                          fade_end ? transparent : opaque};
  const float stops[] = {0.f, start_stop, end_stop, 1.f};
  const skity::Vec2 start = {rect.x(), rect.y()};
  const skity::Vec2 end = direction == ScrollDirection::kVertical
                              ? skity::Vec2{rect.x(), rect.MaxY()}
                              : skity::Vec2{rect.MaxX(), rect.y()};
  return ColorSource::MakeLinear(start, end, std::size(colors), colors, stops,
                                 TileMode::kClamp);
}

}  // namespace

const char* RenderScroll::GetName() const { return "RenderScroll"; }

void RenderScroll::Paint(PaintingContext& context, const FloatPoint& offset) {
  RenderBox::Paint(context, offset);

  auto painter = [this](PaintingContext& ctx, const FloatPoint& offset) {
    // Static offset includes border, padding and margin.
    FloatPoint layer_offset = offset;
    FloatPoint scroll_offset = GetPaintOffsetForScroll();

    PendingOffsetLayer* offset_layer = nullptr;
    if (is_raster_fling_) {
      auto visible_rect = VisibleOverflowRect();
      FloatRect visible_offset_range(
          visible_rect.x(), visible_rect.y(),
          std::max(visible_rect.MaxX() - ContentWidth() - visible_rect.x(),
                   0.f),
          std::max(visible_rect.MaxY() - ContentHeight() - visible_rect.y(),
                   0.f));
      FloatRect max_offset_range(0, 0, MaxScrollWidth(), MaxScrollHeight());
      // Use ScrollOffsetLayer as sub layer of the RenderScroll to handle scroll
      // animtion on raster. It will build a TransformLayer and
      // ScrollOffsetMutator in flow.
      offset_layer = new PendingScrollOffsetLayer(
          layer_offset, FloatPoint() - scroll_offset, visible_offset_range,
          max_offset_range, raster_fling_animation_);
    } else {
      // Use OffsetLayer to handle static offset, and it will build a
      // TransformLayer in flow.
      offset_layer = new PendingOffsetLayer(layer_offset - scroll_offset);
    }
    auto painter = [this](PaintingContext& ctx,
                          const FloatPoint& layer_offset) {
      auto visitor = [&](RenderObject* child) {
        if (child->IsOverlay() && child->Visible()) {
          renderer_->AddOverlayChild(child);
        }
        if (clip_children_to_bounds_ && !child->IsOverlay()) {
          const FloatPoint child_offset = child->OffsetInLayer();
          const FloatRect child_clip_rect(child_offset.x(), child_offset.y(),
                                          child->Width(), child->Height());
          auto child_painter = [child](PaintingContext& child_ctx,
                                       const FloatPoint&) {
            child_ctx.PaintChild(child, child->OffsetInLayer());
          };
          ctx.PushClipRect(child_clip_rect, FloatPoint(), child_painter);
        } else {
          ctx.PaintChild(child, child->OffsetInLayer());
        }
      };
      VisitChildren(visitor);
    };
    ctx.PushLayer(offset_layer, painter, FloatPoint(0, 0));
  };

  PaintingContextCallback content_painter = painter;
  if (fading_edge_length_ > 0) {
    const FloatPoint clip_offset = offset + ClipOffset();
    const FloatRect mask_rect(clip_offset.x(), clip_offset.y(), ClientWidth(),
                              ClientHeight());
    const bool fade_start = scroll_direction_ == ScrollDirection::kVertical
                                ? ScrollTop() > 0
                                : ScrollLeft() > 0;
    const bool fade_end = scroll_direction_ == ScrollDirection::kVertical
                              ? ScrollTop() < MaxScrollHeight()
                              : ScrollLeft() < MaxScrollWidth();
    if (auto mask =
            CreateFadingEdgeMask(mask_rect, scroll_direction_,
                                 fading_edge_length_, fade_start, fade_end)) {
      content_painter = [mask = std::move(mask), mask_rect, painter](
                            PaintingContext& ctx,
                            const FloatPoint& content_offset) {
        ctx.PushShaderMask(mask, mask_rect, BlendMode::kDstIn, content_offset,
                           painter);
      };
    }
  }

  if (HasOverflowClip()) {
    // RenderScroll do not need clip because ScrollWrapper
    FloatPoint clip_offset = offset + ClipOffset();
    FloatRect clip_rect(clip_offset.x(), clip_offset.y(), ClientWidth(),
                        ClientHeight());
    if (Overflow() == CSSProperty::OVERFLOW_X) {
      clip_rect.SetWidth(renderer_->GetFrameSize().width() * 2);
      clip_rect.SetX(clip_rect.x() - renderer_->GetFrameSize().width());
    } else if (Overflow() == CSSProperty::OVERFLOW_Y) {
      clip_rect.SetHeight(renderer_->GetFrameSize().height() * 2);
      clip_rect.SetY(clip_rect.y() - renderer_->GetFrameSize().height());
    }
    context.PushClipRect(clip_rect, offset, content_painter);
  } else {
    content_painter(context, offset);
  }
}

void RenderScroll::SetFadingEdgeLength(float length) {
  length = std::max(length, 0.f);
  if (fading_edge_length_ == length) {
    return;
  }
  fading_edge_length_ = length;
  MarkNeedsPaint();
}

bool RenderScroll::ScrollTo(float scroll_x, float scroll_y) {
  if (!overscroll_enabled_) {
    scroll_x = std::clamp<float>(scroll_x, 0, MaxScrollWidth());
    scroll_y = std::clamp<float>(scroll_y, 0, MaxScrollHeight());
  }

  if (scroll_x != ScrollLeft() || scroll_y != ScrollTop()) {
    SetScrollLeft(scroll_x);
    SetScrollTop(scroll_y);
    return true;
  }
  return false;
}

FloatRect RenderScroll::VisibleOverflowRect() const {
  // For normal ScrollView, the overflow visible rect is the same as the
  // overflow rect.
  if (visible_overflow_rect_.IsEmpty()) {
    return OverflowRect();
  }
  return visible_overflow_rect_;
}

void RenderScroll::SetVisibleOverflowRect(const FloatRect& rect) {
  visible_overflow_rect_ = rect;
  MarkNeedsPaint();
}

void RenderScroll::StartRasterFling(
    std::shared_ptr<clay::ScrollOffsetAnimation> animation) {
  is_raster_fling_ = true;
  raster_fling_animation_ = std::move(animation);
  MarkNeedsPaint();
}

void RenderScroll::StopRasterFling() {
  is_raster_fling_ = false;
  raster_fling_animation_.reset();
  MarkNeedsPaint();
}

ScrollableDirection RenderScroll::GetScrollableDirection() const {
  ScrollableDirection result = ScrollableDirection::kNone;
  if (ScrollLeft() > 0) {
    result |= ScrollableDirection::kLeftwards;
  }
  if (ScrollLeft() < MaxScrollWidth()) {
    result |= ScrollableDirection::kRightwards;
  }
  if (ScrollTop() > 0) {
    result |= ScrollableDirection::kUpwards;
  }
  if (ScrollTop() < MaxScrollHeight()) {
    result |= ScrollableDirection::kDownwards;
  }
  return static_cast<ScrollableDirection>(result);
}

}  // namespace clay
