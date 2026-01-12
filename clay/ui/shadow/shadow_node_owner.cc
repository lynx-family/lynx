// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/shadow/shadow_node_owner.h"

#include <utility>

#include "clay/ui/component/page_view.h"
#include "clay/ui/component/view_context.h"
#include "clay/ui/shadow/shadow_node.h"

namespace clay {

ShadowNodeOwner::ShadowNodeOwner(fml::RefPtr<fml::TaskRunner> ui_task_runner)
    : ui_task_runner_(ui_task_runner) {}

float ShadowNodeOwner::Logical2ClayPixelRatio() {
  if (context_ && context_->GetPageView()) {
    return context_->GetPageView()
        ->GetPixelRatio<kPixelTypeLogical, kPixelTypeClay>();
  } else {
    return 1.0;
  }
}

BaseView* ShadowNodeOwner::FindViewByViewId(int id) {
  FML_DCHECK(ui_task_runner_->RunsTasksOnCurrentThread());
  if (context_) {
    return context_->FindViewByViewId(id);
  }
  return nullptr;
}

void ShadowNodeOwner::ScheduleLayout() { delegate_->ScheduleLayout(); }

void ShadowNodeOwner::TriggerLayout() {
  if (layout_delegate_) {
    layout_delegate_->OnTriggerLayout();
  }
}
uint32_t ShadowNodeOwner::GetContextId() const {
  if (context_) {
    return context_->unique_id();
  }
  return -1;
}

Size ShadowNodeOwner::ViewportSize() const {
  if (context_) {
    return context_->GetPageView()->physical_size();
  } else {
    return Size();
  }
}

void ShadowNodeOwner::MarkDirty(ShadowNode* node) const {
  if (layout_delegate_) {
    layout_delegate_->OnMarkDirty(node->id());
  }
}

ClayLayoutStyles ShadowNodeOwner::GetLayoutStyles(ShadowNode* node) const {
  if (layout_delegate_) {
    return layout_delegate_->OnGetLayoutStyles(node->id());
  }
  return ClayLayoutStyles();
}

MeasureResult ShadowNodeOwner::MeasureNativeNode(
    ShadowNode* node, const MeasureConstraint& constraint) const {
  auto measure_output = layout_delegate_->OnMeasureNativeNode(
      node->id(), constraint.width.value_or(0),
      static_cast<int>(constraint.width_mode), constraint.height.value_or(0),
      static_cast<int>(constraint.height_mode));
  MeasureResult result;
  result.width = measure_output.width;
  result.height = measure_output.height;
  result.baseline = measure_output.baseline;
  return result;
}

void ShadowNodeOwner::AlignNativeNode(ShadowNode* node, float top,
                                      float left) const {
  layout_delegate_->OnAlignNativeNode(node->id(), top, left);
}

void ShadowNodeOwner::AddNode(int id, ShadowNode* node) {
  shadow_node_map_[id] = node;
}

void ShadowNodeOwner::RemoveNode(int id) {
  auto it = shadow_node_map_.find(id);
  if (it != shadow_node_map_.end()) {
    delete it->second;
    shadow_node_map_.erase(it);
  }
}

ShadowNode* ShadowNodeOwner::GetNode(int id) {
  auto it = shadow_node_map_.find(id);
  if (it != shadow_node_map_.end()) {
    return it->second;
  }
  FML_DLOG(ERROR) << "target shadow node: " << id << " not found";
  return nullptr;
}

void ShadowNodeOwner::ClearNodes() {
  for (auto& it : shadow_node_map_) {
    delete it.second;
  }
  shadow_node_map_.clear();
}

}  // namespace clay
