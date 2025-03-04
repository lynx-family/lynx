// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/starlight/layout/staggered_grid_layout_algorithm.h"

#include "core/renderer/starlight/layout/layout_object.h"
#include "core/renderer/starlight/layout/property_resolving_utils.h"

namespace lynx {
namespace starlight {

StaggeredGridLayoutAlgorithm::StaggeredGridLayoutAlgorithm(
    LayoutObject* container)
    : LinearLayoutAlgorithm(container) {
  auto& attr_map = container->attr_map();
  column_count_ = 1;
  if (attr_map.getColumnCount().has_value()) {
    column_count_ = *(attr_map.getColumnCount());
  }
  cross_axis_gap_ = container->GetCSSMutableStyle()->GetListCrossAxisGap();
}

void StaggeredGridLayoutAlgorithm::DetermineContainerSize() {
  bool flag_change = false;
  if (!IsSLDefiniteMode(container_constraints_[kMainAxis].Mode())) {
    container_constraints_[kMainAxis] =
        OneSideConstraint::Definite(total_main_size_);
    flag_change = true;
  }

  if (!IsSLDefiniteMode(container_constraints_[kCrossAxis].Mode())) {
    container_constraints_[kCrossAxis] =
        OneSideConstraint::Definite(total_cross_size_);
    flag_change = true;
  }

  if (flag_change) UpdateContainerSize();
}

void StaggeredGridLayoutAlgorithm::UpdateContainerSize() {
  for (LayoutObject* item : inflow_items_) {
    Constraints used_container_constraints = container_constraints_;
    if (!isHeaderFooter(item) &&
        used_container_constraints[CrossAxis()].Mode() !=
            SLMeasureModeIndefinite) {
      used_container_constraints[CrossAxis()] =
          OneSideConstraint((used_container_constraints[CrossAxis()].Size() -
                             (column_count_ - 1) * cross_axis_gap_) /
                                column_count_,
                            used_container_constraints[CrossAxis()].Mode());
    }

    item->GetBoxInfo()->UpdateBoxData(used_container_constraints, *item,
                                      item->GetLayoutConfigs());
  }
}

/* Algorithm-3
 * Update child size.
 */
void StaggeredGridLayoutAlgorithm::UpdateChildSize(const size_t idx) {
  LayoutObject* child = inflow_items_[idx];
  Constraints used_container_constraints = container_constraints_;
  if (!isHeaderFooter(child) &&
      used_container_constraints[CrossAxis()].Mode() !=
          SLMeasureModeIndefinite) {
    used_container_constraints[CrossAxis()] =
        OneSideConstraint((used_container_constraints[CrossAxis()].Size() -
                           (column_count_ - 1) * cross_axis_gap_) /
                              column_count_,
                          used_container_constraints[CrossAxis()].Mode());
  }
  UpdateChildSizeInternal(idx, used_container_constraints);
}

bool StaggeredGridLayoutAlgorithm::isHeaderFooter(LayoutObject* item) {
  if (!item->attr_map().getListCompType().has_value()) {
    return false;
  }
  ListComponentType type =
      (ListComponentType)(*(item->attr_map().getListCompType()));
  return LayoutStyleUtils::ListComponentTypeIsRow(type);
}

}  // namespace starlight
}  // namespace lynx
