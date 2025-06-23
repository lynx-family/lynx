// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "platform/harmony/lynx_harmony/src/main/cpp/shadow_node/shadow_node.h"

#include <limits>
#include <memory>
#include <utility>

#include "base/include/string/string_utils.h"
#include "core/base/harmony/props_constant.h"
#include "core/renderer/starlight/types/nlength.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_context.h"

namespace lynx {
namespace tasm {
namespace harmony {

MeasureFuncHarmony::MeasureFuncHarmony(CustomMeasureFunc* custom_measure_func)
    : custom_measure_func_(custom_measure_func) {}

LayoutResult MeasureFuncHarmony::Measure(float width, int32_t width_mode,
                                         float height, int32_t height_mode,
                                         bool final_measure) {
  return custom_measure_func_->Measure(
      width, static_cast<MeasureMode>(width_mode), height,
      static_cast<MeasureMode>(height_mode), final_measure);
}

void MeasureFuncHarmony::Alignment() { return custom_measure_func_->Align(); }

void ShadowNode::AdoptSlNode() {
  if (layout_node_manager_ != nullptr && custom_measure_func_ != nullptr) {
    layout_node_manager_->SetMeasureFunc(
        sign_, std::make_unique<MeasureFuncHarmony>(custom_measure_func_));
  }
}

void ShadowNode::OnLayoutBefore() {}

void ShadowNode::UpdateLayout(float left, float top, float width,
                              float height) {}

void ShadowNode::AddChild(ShadowNode* child, int index) {
  if (index == -1) {
    children_.emplace_back(child);
  } else {
    children_.insert(children_.begin() + index, child);
  }
  child->SetParent(this);
}

void ShadowNode::RemoveChild(ShadowNode* child) {
  child->parent_ = nullptr;
  children_.erase(std::remove(children_.begin(), children_.end(), child),
                  children_.end());
}

void ShadowNode::AlignLayoutNode(float left, float top) const {
  layout_node_manager_->AlignmentByPlatform(sign_, top, left);
}

void ShadowNode::AlignTo(float left, float top) const {
  // todo(renzhongyue): alignment of layout_node can be extract to a isolated
  // class.
  AlignLayoutNode(left / ScaleDensity(), top / ScaleDensity());
}

LayoutResult ShadowNode::MeasureLayoutNode(float width, MeasureMode width_mode,
                                           float height,
                                           MeasureMode height_mode,
                                           bool final_measure) const {
  LayoutResult size = layout_node_manager_->UpdateMeasureByPlatform(
      sign_, width, static_cast<int32_t>(width_mode), height,
      static_cast<int32_t>(height_mode), final_measure);
  size.width_ *= ScaleDensity();
  size.height_ *= ScaleDensity();
  size.baseline_ *= ScaleDensity();
  return size;
}

void ShadowNode::SetCustomMeasureFunc(CustomMeasureFunc* measure_func) {
  custom_measure_func_ = measure_func;
}

float ShadowNode::ComputedWidth() const {
  return layout_node_manager_->GetWidth(sign_);
}

float ShadowNode::ComputedHeight() const {
  return layout_node_manager_->GetHeight(sign_);
}

float ShadowNode::ComputedMinWidth() const {
  return layout_node_manager_->GetMinWidth(sign_);
}

float ShadowNode::ComputedMaxWidth() const {
  return layout_node_manager_->GetMaxWidth(sign_);
}

float ShadowNode::ComputedMinHeight() const {
  return layout_node_manager_->GetMinHeight(sign_);
}

float ShadowNode::ComputedMaxHeight() const {
  return layout_node_manager_->GetMaxHeight(sign_);
}

void ShadowNode::MarkDirty() const {
  if (is_destroyed_) {
    return;
  }
  if (!IsVirtual()) {
    layout_node_manager_->MarkDirtyAndRequestLayout(sign_);
  } else if (auto* node = FindNonVirtualNode(); node != nullptr) {
    node->MarkDirty();
  }
}

void ShadowNode::RequestLayout() const {
  if (is_destroyed_) {
    return;
  }
  if (!IsVirtual()) {
    layout_node_manager_->MarkDirtyAndRequestLayout(sign_);
  } else if (auto* node = FindNonVirtualNode(); node != nullptr) {
    node->RequestLayout();
  }
}

const ShadowNode* ShadowNode::FindNonVirtualNode() const {
  if (IsVirtual()) {
    return parent_ ? parent_->FindNonVirtualNode() : nullptr;
  }
  return this;
}

void ShadowNode::UpdateProps(PropBundleHarmony* props) {
  if (!props) {
    return;
  }
  PropBundleHarmony* pda = reinterpret_cast<PropBundleHarmony*>(props);
  auto& prop_map = pda->GetProps();
  std::for_each(prop_map.begin(), prop_map.end(),
                [this](std::pair<std::string, lepus::Value> const& entry) {
                  this->OnPropsUpdate(entry.first.c_str(), entry.second);
                });
  const auto& events = pda->GetEvents();
  if (events) {
    SetEvent(events.value());
  }
}

void ShadowNode::OnPropsUpdate(char const* attr, lepus::Value const& value) {
  if (base::StringEqual(attr, harmony::kEventThrough)) {
    if (value.IsBool()) {
      event_through_ = value.Bool() ? LynxEventPropStatus::kEnable
                                    : LynxEventPropStatus::kDisable;
    }
  } else if (base::StringEqual(attr, kIgnoreFocus)) {
    if (value.IsBool()) {
      ignore_focus_ = value.Bool() ? LynxEventPropStatus::kEnable
                                   : LynxEventPropStatus::kDisable;
    }
  }
}

void ShadowNode::MeasureChildrenNode(float width, MeasureMode width_mode,
                                     float height, MeasureMode height_mode,
                                     bool final_measure) {
  for (const auto& child : GetChildren()) {
    child->MeasureChildrenNode(width, width_mode, height, height_mode,
                               final_measure);
  }
}

void ShadowNode::SetLayoutNodeManager(LayoutNodeManager* layout_node_manager) {
  layout_node_manager_ = layout_node_manager;
}

void ShadowNode::SetEvent(const std::vector<lepus::Value>& events) {
  events_.clear();
  for (const auto& e : events) {
    if (!e.IsArray() || e.Array()->size() == 0) {
      continue;
    }
    const auto& name = e.Array()->get(0).StdString();
    events_.emplace_back(name);
  }
}

bool ShadowNode::IsBindEvent() const {
  return events_.size() > 0 ||
         ignore_focus_ != LynxEventPropStatus::kUndefined ||
         event_through_ != LynxEventPropStatus::kUndefined;
}

void ShadowNode::ReleaseSelf() const { delete this; }

float ShadowNode::ScaleDensity() const {
  if (!context_) {
    return 1.f;
  }
  return context_->ScaledDensity();
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
