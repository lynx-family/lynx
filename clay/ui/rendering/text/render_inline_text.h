// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_RENDERING_TEXT_RENDER_INLINE_TEXT_H_
#define CLAY_UI_RENDERING_TEXT_RENDER_INLINE_TEXT_H_

#include <list>

#include "clay/gfx/geometry/float_rect.h"
#include "clay/ui/painter/painting_context.h"
#include "clay/ui/rendering/render_box.h"

namespace clay {

class RenderInlineText : public RenderBox {
 public:
  RenderInlineText() = default;
  virtual ~RenderInlineText() = default;

  const char* GetName() const override { return "RenderInlineText"; }

  // Use same layer with parent 'text' view.
  bool IsRepaintBoundary() const override { return false; }

  void Paint(PaintingContext& context, const FloatPoint& offset) override;

  void AddTextBox(const skity::Rect& box) { text_boxes_.emplace_back(box); }

  void ClearTextBox() { text_boxes_.clear(); }

  // Inline text can always be displayed when `Paint()` called because once
  // the parent text view is set to display:none, inline text won't be
  // requested to paint.
  bool CanDisplay() override { return true; }

 private:
  std::list<FloatRect> text_boxes_;
};

}  // namespace clay

#endif  // CLAY_UI_RENDERING_TEXT_RENDER_INLINE_TEXT_H_
