// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_TEXT_LAYOUT_CLIENT_H_
#define CLAY_UI_COMPONENT_TEXT_LAYOUT_CLIENT_H_

#include "clay/gfx/geometry/float_point.h"

namespace clay {

class BaseView;

// As text layout attributes and results are separated from text view,
// so delegate it to shadow node.
class LayoutClient {
 public:
  // Used for get the internal view like inline text/image hit by give point.
  virtual BaseView* GetViewAtPosition(const FloatPoint& point_by_paragraph,
                                      const FloatPoint& point_by_page) = 0;
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_TEXT_LAYOUT_CLIENT_H_
