// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_BOUNCE_VIEW_BOUNCE_VIEW_H_
#define CLAY_UI_COMPONENT_BOUNCE_VIEW_BOUNCE_VIEW_H_

#include "clay/gfx/geometry/direction.h"
#include "clay/ui/component/base_view.h"

namespace clay {

class BounceView : public WithTypeInfo<BounceView, BaseView> {
 public:
  BounceView(int id, PageView* page_view);
  ~BounceView() override = default;

  Direction GetDirection() const { return direction_; }

  void SetBounceDirection(Direction direction) { direction_ = direction; }

  void SetAttribute(const char* attr, const clay::Value& value) override;

 private:
  Direction direction_ = Direction::kRight;
};

}  // namespace clay
#endif  // CLAY_UI_COMPONENT_BOUNCE_VIEW_BOUNCE_VIEW_H_
