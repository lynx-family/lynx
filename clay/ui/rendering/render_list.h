// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_RENDERING_RENDER_LIST_H_
#define CLAY_UI_RENDERING_RENDER_LIST_H_

#include "clay/ui/rendering/render_scroll.h"

namespace clay {

class RenderList : public RenderScroll {
 public:
  RenderList() = default;
  ~RenderList() override = default;

  const char* GetName() const override;
};

}  // namespace clay

#endif  // CLAY_UI_RENDERING_RENDER_LIST_H_
