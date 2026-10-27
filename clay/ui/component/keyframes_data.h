// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_KEYFRAMES_DATA_H_
#define CLAY_UI_COMPONENT_KEYFRAMES_DATA_H_

#include <memory>
#include <vector>

#include "clay/gfx/geometry/box_shadow_value.h"
#include "clay/gfx/geometry/filter_value.h"
#include "clay/public/clay.h"
#include "clay/public/style_types.h"
#include "clay/public/value.h"

namespace clay {

struct ClayAnimationPropertyValue {
  ClayAnimationPropertyType type;
  clay::Value value;
};

struct ClayKeyframe {
  float percentage;
  int size;
  ClayAnimationPropertyValue* properties;
};

struct ClayKeyframesRule {
  const char* name;
  int size;
  ClayKeyframe* keyframes;
};

struct KeyframesData {
  int size;
  ClayKeyframesRule* keyframe_rules;

  KeyframesData(int size = 0, ClayKeyframesRule* rules = nullptr) {
    this->size = size;
    this->keyframe_rules = rules;
  }
  KeyframesData(const clay::Value& prop_keyframes_value);
  ~KeyframesData();

 private:
  std::unique_ptr<ClayKeyframesRule[]> rules_;
  std::vector<std::unique_ptr<ClayKeyframe[]>> keyframes_;
  std::vector<std::unique_ptr<ClayAnimationPropertyValue[]>> properties_;
  std::vector<std::unique_ptr<ClayTransform>> transforms_;
  std::vector<std::unique_ptr<std::vector<FilterValue>>> filters_;
  std::vector<std::unique_ptr<std::vector<BoxShadowValue>>> shadows_;
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_KEYFRAMES_DATA_H_
