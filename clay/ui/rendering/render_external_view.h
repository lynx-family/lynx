// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_RENDERING_RENDER_EXTERNAL_VIEW_H_
#define CLAY_UI_RENDERING_RENDER_EXTERNAL_VIEW_H_

#include "clay/ui/rendering/render_container.h"

namespace clay {

class RenderExternalView : public RenderContainer {
 public:
  RenderExternalView() = default;
  ~RenderExternalView() override = default;

  const char* GetName() const override { return "render_external_view"; }
  bool IsRepaintBoundary() const override { return true; }

  void SetBackingSize(skity::Vec2 size);

  skity::Vec2 GetSize() const { return size_; }

  bool IsExternalView() const override { return true; }

  void Paint(PaintingContext& context, const FloatPoint& offset) override;

 private:
  skity::Vec2 size_ = {0, 0};
};

}  // namespace clay

#endif  // CLAY_UI_RENDERING_RENDER_EXTERNAL_VIEW_H_
