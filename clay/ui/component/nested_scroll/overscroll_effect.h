// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_NESTED_SCROLL_OVERSCROLL_EFFECT_H_
#define CLAY_UI_COMPONENT_NESTED_SCROLL_OVERSCROLL_EFFECT_H_

#include "clay/ui/rendering/render_scroll.h"

namespace clay {

class OverscrollEffect {
 public:
  explicit OverscrollEffect(RenderScroll* render_scroll)
      : render_scroll_(render_scroll) {}
  virtual ~OverscrollEffect() = default;

  virtual void OnOverscroll(FloatPoint offset, FloatPoint prev_offset) = 0;

 protected:
  RenderScroll* render_scroll_;
};

class OffsetOverscrollEffect : public OverscrollEffect {
 public:
  using OverscrollEffect::OverscrollEffect;

  void OnOverscroll(FloatPoint offset, FloatPoint prev_offset) override;
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_NESTED_SCROLL_OVERSCROLL_EFFECT_H_
