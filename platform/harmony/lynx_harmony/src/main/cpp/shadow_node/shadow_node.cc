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

std::unordered_map<std::string, ShadowNode::PropSetter>
    ShadowNode::prop_setters_ = {
        {"ignore-focus", &ShadowNode::SetIgnoreFocus},
        {"idSelector", &ShadowNode::SetIdSelector},
        {"event-through", &ShadowNode::SetEventThrough},
        {"pointer-events", &ShadowNode::SetPointerEvents}};

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
  child->SetParent(nullptr);
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
  const ShadowNode* node = this;
  int hop = 0;
  while (node && node->IsVirtual()) {
    if (!node->context_ || node->parent_sign_ < 0) {
      return nullptr;
    }
    node = node->context_->FindShadowNodeBySign(node->parent_sign_);
    if (++hop > 128) {
      return nullptr;
    }
  }
  return node;
}

void ShadowNode::UpdateProps(PropBundleHarmony* props) {
  if (!props) {
    return;
  }
  PropBundleHarmony* pda = reinterpret_cast<PropBundleHarmony*>(props);
  auto& prop_map = pda->GetProps();
  std::for_each(prop_map.begin(), prop_map.end(),
                [this](std::pair<std::string, lepus::Value> const& entry) {
                  this->OnPropsUpdate(entry.first, entry.second);
                });
  const auto& events = pda->GetEvents();
  if (events) {
    SetEvent(events.value());
  }
}

void ShadowNode::OnPropsUpdate(const std::string& name,
                               const lepus::Value& value) {
  if (auto it = prop_setters_.find(name); it != prop_setters_.end()) {
    PropSetter setter = it->second;
    (this->*setter)(value);
  }
}

void ShadowNode::SetPointerEvents(const lepus::Value& value) {
  if (value.IsNumber()) {
    int int_value = value.Number();
    if (int_value >= static_cast<int>(LynxPointerEventsValue::kAuto) &&
        int_value < static_cast<int>(LynxPointerEventsValue::kUnset)) {
      pointer_events_ = static_cast<LynxPointerEventsValue>(int_value);
    }
  }
}

void ShadowNode::SetIgnoreFocus(const lepus::Value& value) {
  if (value.IsBool()) {
    ignore_focus_ = value.Bool() ? LynxEventPropStatus::kEnable
                                 : LynxEventPropStatus::kDisable;
  }
}

void ShadowNode::SetEventThrough(const lepus::Value& value) {
  if (value.IsBool()) {
    event_through_ = value.Bool() ? LynxEventPropStatus::kEnable
                                  : LynxEventPropStatus::kDisable;
  } else if (value.IsString()) {
    auto bool_str = value.StdString();
    if (bool_str == "true") {
      event_through_ = LynxEventPropStatus::kEnable;
    } else if (bool_str == "false") {
      event_through_ = LynxEventPropStatus::kDisable;
    }
  }
}

void ShadowNode::SetIdSelector(const lepus::Value& value) {
  if (value.IsNil() || value.IsUndefined()) {
    id_selector_.clear();
    return;
  }
  if (value.IsString()) {
    id_selector_ = value.StdString();
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
