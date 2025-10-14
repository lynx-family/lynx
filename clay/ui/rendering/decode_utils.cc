// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/rendering/decode_utils.h"

#include <algorithm>

#include "clay/ui/rendering/render_object.h"
#include "clay/ui/rendering/render_scroll.h"

namespace clay {

namespace {

float LeftWithScroll(RenderObject* render_object) {
  if (render_object) {
    return render_object->Left() - render_object->ScrollLeft();
  }
  return 0.f;
}

float TopWithScroll(RenderObject* render_object) {
  if (render_object) {
    return render_object->Top() - render_object->ScrollTop();
  }
  return 0.f;
}

// @Param render_object: child render object of a scrollable parent.
// @Param parent_scroll: will be set to the first scrollable parent of render
// object.
// @Return: the visible area of render_object in its parent scroll area.
std::optional<FloatRect> VisibleAreaInParentScroll(
    RenderObject* render_object, RenderScroll** parent_scroll) {
  auto self = render_object;
  RenderObject* parent = static_cast<RenderObject*>(render_object->Parent());
  FloatRect visible_rect(render_object->Left(), render_object->Top(),
                         render_object->Width(), render_object->Height());
  while (parent) {
    if (self->IsOverlay()) {
      break;
    } else {
      self = parent;
      float left = LeftWithScroll(parent);
      float top = TopWithScroll(parent);
      visible_rect.Move(left, top);
      if (parent->IsScrollable()) {
        // parent_scroll will be set to the first render scroll parent.
        if (*parent_scroll == nullptr) {
          *parent_scroll = static_cast<RenderScroll*>(parent);
        }
        FloatRect parent_rect(0, 0, parent->Width(), parent->Height());
        // child is not visible in its parent scroll area.
        if (!visible_rect.Intersects(parent_rect)) {
          return std::nullopt;
        }
        visible_rect.Intersect(parent_rect);
      }
      parent = static_cast<RenderObject*>(parent->Parent());
    }
  }
  return visible_rect;
}

// @Param render_object: child render object of a scrollable parent.
// @Param parent_scroll: will be set to the first scrollable parent of
// render_object.
// @Return: true if render_object is in the visible area of its parent scroll
// area.
bool IsInVisibleArea(RenderObject* render_object,
                     RenderScroll** parent_scroll) {
  Renderer* renderer = render_object->GetRenderer();
  Size frame_size = renderer->GetFrameSize();
  FloatRect visible_area =
      FloatRect(0, 0, frame_size.width(), frame_size.height());

  auto bounds = VisibleAreaInParentScroll(render_object, parent_scroll);
  return bounds.has_value() ? visible_area.Intersects(*bounds) : false;
}

bool InScrollAnimation(RenderScroll* render_scroll) {
  return render_scroll->InScrollAnimation();
}

// @Param render_object: child render object of a scrollable parent.
// @Param parent_scroll: the scrollable parent of render_object.
// @Return: true if render_object is in the preload range of parent_scroll.
bool IsInPreloadRange(RenderObject* render_object,
                      RenderScroll* parent_scroll) {
  // We would like to preload at the top and bottom of the scrolling container,
  // with some distances if scroll direction is vertical. Otherwise, preload at
  // the left and right.
  skity::Rect preload_bounds;
  float distance = 0.f;
  Renderer* renderer = render_object->GetRenderer();
  Size frame_size = renderer->GetFrameSize();
  if (parent_scroll->GetScrollDirection() == ScrollDirection::kVertical) {
    distance = std::min(parent_scroll->Height(),
                        static_cast<float>(frame_size.height())) /
               4.f;
    preload_bounds =
        skity::Rect::MakeXYWH(0, -distance, parent_scroll->Width(),
                              parent_scroll->Height() + 2 * distance);
  } else {
    distance = std::min(parent_scroll->Width(),
                        static_cast<float>(frame_size.width())) /
               4.f;
    preload_bounds = skity::Rect::MakeXYWH(
        -distance, 0, parent_scroll->Width() + 2 * distance,
        parent_scroll->Height());
  }

  float left = render_object->Left();
  float top = render_object->Top();
  RenderObject* parent = static_cast<RenderObject*>(render_object->Parent());
  while (parent != parent_scroll) {
    left += LeftWithScroll(parent);
    top += TopWithScroll(parent);
    parent = static_cast<RenderObject*>(parent->Parent());
  }
  left += LeftWithScroll(parent_scroll);
  top += TopWithScroll(parent_scroll);
  skity::Rect child_rect = skity::Rect::MakeXYWH(
      left, top, render_object->Width(), render_object->Height());

  return preload_bounds.Intersect(child_rect);
}

}  // namespace

DecodePriority DecodeUtils::GetDecodePriority(RenderObject* render_object) {
  if (!render_object) {
    return DecodePriority::kPending;
  }
  if (!render_object->ImageDecodeWithPriority()) {
    return DecodePriority::kImmediate;
  }
  TRACE_EVENT("clay", __PRETTY_FUNCTION__);

  RenderScroll* parent_scroll = nullptr;
  bool visible = IsInVisibleArea(render_object, &parent_scroll);
  if (visible) {
    if (parent_scroll) {
      // If the parent scroll is in scroll animation, the parent scroll
      // container will be responsible for decoding its children nodes due to
      // visibility after animation. So here just return kPending to avoid
      // unnecessary decode.
      if (InScrollAnimation(parent_scroll)) {
        return DecodePriority::kPending;
      }
      return DecodePriority::kImmediate;
    } else {
      return DecodePriority::kImmediate;
    }
  } else {
    if (parent_scroll) {
      if (InScrollAnimation(parent_scroll)) {
        return DecodePriority::kPending;
      }
      // If parent scrollable is visible and render object is in preload range,
      // return kDeferred to decode later.
      if (IsInPreloadRange(render_object, parent_scroll) &&
          IsInVisibleArea(parent_scroll, nullptr)) {
        return DecodePriority::kDeferred;
      }
      return DecodePriority::kPending;
    } else {
      return DecodePriority::kPending;
    }
  }
}

}  // namespace clay
