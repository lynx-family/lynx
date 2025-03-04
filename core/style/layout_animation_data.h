// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_STYLE_LAYOUT_ANIMATION_DATA_H_
#define CORE_STYLE_LAYOUT_ANIMATION_DATA_H_

#include <tuple>

#include "core/renderer/starlight/style/css_type.h"
#include "core/style/timing_function_data.h"

namespace lynx {
namespace starlight {

struct BaseLayoutAnimationData {
  long duration;
  long delay;
  starlight::AnimationPropertyType property;
  TimingFunctionData timing_function;
  BaseLayoutAnimationData();
  ~BaseLayoutAnimationData() = default;
  void Reset();
  bool operator==(const BaseLayoutAnimationData& rhs) const {
    return std::tie(duration, delay, property, timing_function) ==
           std::tie(rhs.duration, rhs.delay, rhs.property, rhs.timing_function);
  }
};

struct LayoutAnimationData {
  LayoutAnimationData() = default;
  ~LayoutAnimationData() = default;

  BaseLayoutAnimationData create_ani;
  BaseLayoutAnimationData update_ani;
  BaseLayoutAnimationData delete_ani;
};

}  // namespace starlight
}  // namespace lynx

#endif  // CORE_STYLE_LAYOUT_ANIMATION_DATA_H_
