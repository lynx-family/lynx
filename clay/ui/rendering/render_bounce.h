// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_RENDERING_RENDER_BOUNCE_H_
#define CLAY_UI_RENDERING_RENDER_BOUNCE_H_

#include "clay/ui/rendering/render_container.h"

namespace clay {

class RenderBounce : public RenderContainer {
 public:
  RenderBounce() = default;
  ~RenderBounce() override = default;

  const char* GetName() const override { return "RenderBounce"; }

  bool IsRenderBounceView() const override { return true; }
};

}  // namespace clay
#endif  // CLAY_UI_RENDERING_RENDER_BOUNCE_H_
