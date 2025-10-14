// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/shadow/editable_ng_shadow_node.h"

#include <algorithm>
#include <limits>
#include <string>

#include "clay/fml/logging.h"
#include "clay/ui/common/measure_constraint.h"
#include "clay/ui/component/editable/textarea_ng_view.h"

namespace clay {

namespace {

constexpr float kDefaultLineHeightFactor = 1.2f;

}  // namespace

EditableNGShadowNode::EditableNGShadowNode(ShadowNodeOwner* owner,
                                           std::string tag, int id)
    : EditableShadowNode(owner, tag, id) {}

void EditableNGShadowNode::Measure(const MeasureConstraint& constraint,
                                   MeasureResult& result) {
  if (constraint.height_mode == MeasureMode::kDefinite) {
    result.height = *constraint.height;
  } else {
    FML_DCHECK(owner_->GetUITaskRunner()->RunsTasksOnCurrentThread());
    auto editable_view =
        static_cast<TextAreaNGView*>(owner_->FindViewByViewId(id()));
    editable_view->Measure(constraint, result);
    return;
  }
  if (constraint.width_mode == MeasureMode::kIndefinite ||
      !constraint.width.has_value()) {
    result.width = std::numeric_limits<float>::infinity();
  } else {
    result.width = *constraint.width;
  }

  if (constraint.width_mode == MeasureMode::kAtMost) {
    result.width = std::min(result.width, constraint.width.value_or(0));
  }
  if (constraint.height_mode == MeasureMode::kAtMost) {
    result.height = std::min(result.height, constraint.height.value_or(0));
  }
}

}  // namespace clay
