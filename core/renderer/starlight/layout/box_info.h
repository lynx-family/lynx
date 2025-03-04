// Copyright 2017 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_STARLIGHT_LAYOUT_BOX_INFO_H_
#define CORE_RENDERER_STARLIGHT_LAYOUT_BOX_INFO_H_

#include <array>

#include "core/renderer/starlight/layout/layout_global.h"
#include "core/renderer/starlight/types/layout_types.h"
#include "core/renderer/starlight/types/nlength.h"

namespace lynx {
namespace starlight {

class LayoutObject;

using FourValue = DirectionValue<float>;

class BoxInfo {
 public:
  bool IsDependentOnPercentBase(bool is_horizontal) const {
    return is_horizontal ? values_of_width_modify_ : values_of_height_modify_;
  }

  DimensionValue<float> min_size_;
  DimensionValue<float> max_size_;
  FourValue padding_;
  FourValue margin_;

  unsigned box_info_props_modified : 1;

  BoxInfo();
  void SetBoxInfoPropsModified();
  void ResetBoxInfo();

  void InitializeBoxInfo(const Constraints& constraints, LayoutObject& obj,
                         const LayoutConfigs& layout_config);

  void ResolveBoxInfoForAbsoluteAndFixed(const Constraints& constraints,
                                         LayoutObject& obj,
                                         const LayoutConfigs& layout_config);

  void UpdateBoxData(const Constraints& constraint, LayoutObject& obj,
                     const LayoutConfigs& layout_config);

 private:
  void InitializeMarginPadding(const NLength& length,
                               const LayoutUnit& available_size, float& value);
  void ResolveMinMax(const NLength& width, const NLength& height,
                     const LayoutUnit& available_width,
                     const LayoutUnit& available_height,
                     const LayoutConfigs& layout_config,
                     const LayoutComputedStyle& style, float default_value,
                     DimensionValue<float>& value);
  void UpdateHorizontalBoxData(const LayoutUnit& available_width,
                               const LayoutComputedStyle& style, bool& dirty);
  float CalculateLengthValue(const NLength& length,
                             const LayoutUnit& available_width);

  bool values_of_width_modify_;
  bool values_of_height_modify_;

  DimensionValue<bool> max_should_modify_;
  DimensionValue<bool> min_should_modify_;
  DirectionValue<bool> padding_should_modify_;
  DirectionValue<bool> margin_should_modify_;
};
}  // namespace starlight
}  // namespace lynx
#endif  // CORE_RENDERER_STARLIGHT_LAYOUT_BOX_INFO_H_
