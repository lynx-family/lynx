// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_STARLIGHT_LAYOUT_ELASTIC_LAYOUT_UTILS_H_
#define CORE_RENDERER_STARLIGHT_LAYOUT_ELASTIC_LAYOUT_UTILS_H_

#include <functional>

#include "core/renderer/starlight/layout/direction_selector.h"
#include "core/renderer/starlight/layout/layout_object.h"

namespace lynx {
namespace starlight {

// Elastic layout utils is a utils class for shared usages of linear and flex
// layout algorithm. The utils class can be used to resolve items size affected
// by grow/shrink factor and min/max css properties in given available spaces.
class ElasticLayoutUtils {
 private:
  ElasticLayoutUtils() {}

 public:
  using ElasticFactorGetter = std::function<float(const LayoutObject&)>;

  // ElasticInfos is a struct used as a closure of inputs for elastic sizing
  // algorithm, with the variables definition the same as the flex box layout
  // algorithm definition in W3C html css.
  struct ElasticInfos {
    ElasticInfos(const LayoutItems& targets,
                 const InlineFloatArray& elastic_bases,
                 const InlineFloatArray& hypothetical_sizes,
                 bool is_elastic_grow,
                 const DirectionSelector& direction_selector, size_t start,
                 size_t end, float main_axis_gap)
        : targets_(targets),
          elastic_bases_(elastic_bases),
          hypothetical_sizes_(hypothetical_sizes),
          is_elastic_grow_(is_elastic_grow),
          direction_selector_(direction_selector),
          start_idx_(start),
          end_idx_(end),
          main_axis_gap_(main_axis_gap) {}
    const LayoutItems& targets_;
    const InlineFloatArray& elastic_bases_;
    const InlineFloatArray& hypothetical_sizes_;
    bool is_elastic_grow_;
    const DirectionSelector& direction_selector_;
    size_t start_idx_, end_idx_;
    // Use -1.f to mark this value as unset here. Sadly optional is a c++17
    // feature.
    float total_elastic_factor_override_ = -1.f;
    float main_axis_gap_;
  };

  // To compute the item sizes with given infos.
  // compute_item_size will be used to return the computed size.
  // Return value is the remaining available space.
  static float ComputeElasticItemSizes(
      ElasticInfos& elastic_infos, float available_spaces,
      ElasticFactorGetter elastic_factor_getter,
      InlineFloatArray& computed_item_sizes);

  // To compute hypothetical size.
  // Computed hypothetical sizes will be stored in argument hypothetical_sizes.
  // Return value is the total hypothetical sizes.
  static float ComputeHypotheticalSizes(
      const LayoutItems& targets, const InlineFloatArray& elastic_bases,
      const DirectionSelector& direction_selector,
      InlineFloatArray& hypothetical_sizes);
};
}  // namespace starlight
}  // namespace lynx
#endif  // CORE_RENDERER_STARLIGHT_LAYOUT_ELASTIC_LAYOUT_UTILS_H_
