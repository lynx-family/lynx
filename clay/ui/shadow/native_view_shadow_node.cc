// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/shadow/native_view_shadow_node.h"

#include "clay/ui/component/native_view.h"
#include "clay/ui/component/view_context.h"

namespace clay {

MeasureResult NativeViewShadowNode::Measure(
    const MeasureConstraint& constraint) {
  // NOTE: For now we do not support layout thread for NativeViews.
  FML_DCHECK(owner_->GetUITaskRunner()->RunsTasksOnCurrentThread());

  auto view = owner_->GetViewContext()->GetViewById(id_);
  if (view && view->Is<NativeView>()) {
    return static_cast<NativeView*>(view)->Measure(constraint);
  }
  return {};
}

}  // namespace clay
