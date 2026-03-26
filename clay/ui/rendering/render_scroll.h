// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_RENDERING_RENDER_SCROLL_H_
#define CLAY_UI_RENDERING_RENDER_SCROLL_H_

#include <memory>

#include "clay/flow/animation/scroll_offset_animation.h"
#include "clay/gfx/geometry/float_point.h"
#include "clay/ui/gesture/scrollable_direction.h"
#include "clay/ui/painter/painting_context.h"
#include "clay/ui/rendering/render_box.h"

namespace clay {

class RenderScroll : public RenderBox {
 public:
  RenderScroll() = default;
  ~RenderScroll() override = default;

  const char* GetName() const override;
  void Paint(PaintingContext& context, const FloatPoint& offset) override;
  bool IsRepaintBoundary() const override { return true; }
  bool HasClip() const override { return true; }

  virtual bool ScrollTo(float scroll_x, float scroll_y);

  void SetOverscrollEnabled(bool value) { overscroll_enabled_ = value; }
  bool OverscrollEnabled() const { return overscroll_enabled_; }

  FloatRect VisibleOverflowRect() const;
  void SetVisibleOverflowRect(const FloatRect& rect);

  // Try to start fling animation on raster thread with
  // `clay::ScrollOffsetAnimation`.
  void StartRasterFling(std::shared_ptr<clay::ScrollOffsetAnimation>);
  void StopRasterFling();

  ScrollableDirection GetScrollableDirection() const;

  bool IsScrollable() const override { return true; }

  void SetScrollDirection(ScrollDirection direction) {
    scroll_direction_ = direction;
  }
  ScrollDirection GetScrollDirection() const { return scroll_direction_; }

  void SetInScrollAnimation(bool value) { in_scroll_animation_ = value; }
  bool InScrollAnimation() const { return in_scroll_animation_; }

  // Clip each direct child to its own bounds before painting.
  void SetClipChildrenToBounds(bool value) { clip_children_to_bounds_ = value; }

 private:
  // If overscroll is enabled, we can keep scrolling regardless of whether
  // exceeding the overflow rect.
  bool overscroll_enabled_ = false;

  // Indicates whether the scroll is currently being animated by the raster.
  bool is_raster_fling_ = false;
  std::shared_ptr<clay::ScrollOffsetAnimation> raster_fling_animation_;
  // The visible overflow rect, distinguish from the overflow rect, components
  // like ListContainerView may has more invisible items out of it's visible
  // range. The animation should not finish, just blocked at the edge unless
  // reach the real overflow rect. The raster scroll animation will use it to
  // calculate the offset range.
  FloatRect visible_overflow_rect_;
  ScrollDirection scroll_direction_;
  bool in_scroll_animation_ = false;
  bool clip_children_to_bounds_ = false;
};

}  // namespace clay
#endif  // CLAY_UI_RENDERING_RENDER_SCROLL_H_
