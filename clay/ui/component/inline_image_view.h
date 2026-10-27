// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/geometry/float_point.h"
#include "clay/ui/component/base_image_view.h"

#ifndef CLAY_UI_COMPONENT_INLINE_IMAGE_VIEW_H_
#define CLAY_UI_COMPONENT_INLINE_IMAGE_VIEW_H_

namespace clay {

class InlineImageView : public WithTypeInfo<InlineImageView, BaseImageView> {
 public:
  InlineImageView(int id, PageView* page_view);
  ~InlineImageView() override = default;
  void SetLocation(FloatPoint location) { location_ = location; }
  const FloatPoint& GetLocation() const { return location_; }

 private:
  FloatPoint location_;
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_INLINE_IMAGE_VIEW_H_
