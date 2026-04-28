// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_STYLE_LAYOUT_ANIMATION_DATA_H_
#define CORE_STYLE_LAYOUT_ANIMATION_DATA_H_

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
    return duration == rhs.duration && delay == rhs.delay &&
           property == rhs.property && timing_function == rhs.timing_function;
  }
};

struct LayoutAnimationData {
  LayoutAnimationData() = default;
  ~LayoutAnimationData() = default;

  BaseLayoutAnimationData create_ani;
  BaseLayoutAnimationData update_ani;
  BaseLayoutAnimationData delete_ani;

  bool operator==(const LayoutAnimationData& rhs) const {
    return create_ani == rhs.create_ani && update_ani == rhs.update_ani &&
           delete_ani == rhs.delete_ani;
  }
};

}  // namespace starlight
}  // namespace lynx

#endif  // CORE_STYLE_LAYOUT_ANIMATION_DATA_H_
