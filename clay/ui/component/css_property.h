// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_CSS_PROPERTY_H_
#define CLAY_UI_COMPONENT_CSS_PROPERTY_H_

#include <string>

#include "clay/public/clay.h"
#include "clay/public/value.h"
#include "clay/ui/component/keywords.h"

namespace clay {

enum class Side { kTop, kRight, kBottom, kLeft, kAll };

// Adapting lynx side direction layout properties
enum class DirectionType : unsigned {
  kNormal = 0,   // version:1.0
  kLynxRtl = 1,  // version:1.0
  kRtl = 2,      // version:1.0
  kLtr = 3,      // version:2.0
};

class BaseView;

class CSSProperty {
 public:
  static constexpr uint8_t OVERFLOW_HIDDEN = 0x00;
  static constexpr uint8_t OVERFLOW_X = 0x01;
  static constexpr uint8_t OVERFLOW_Y = 0x02;
  static constexpr uint8_t OVERFLOW_XY = OVERFLOW_X | OVERFLOW_Y;

  static bool SetAttribute(BaseView* view, KeywordID id,
                           const clay::Value& value);
};

}  // namespace clay
#endif  // CLAY_UI_COMPONENT_CSS_PROPERTY_H_
