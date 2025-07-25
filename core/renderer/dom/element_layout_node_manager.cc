// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/renderer/dom/element_layout_node_manager.h"

#include <memory>
#include <utility>

#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/fiber_element.h"

namespace lynx {
namespace tasm {

ElementLayoutNodeManager::ElementLayoutNodeManager(
    ElementManager& element_manager)
    : element_manager_(element_manager) {}

void ElementLayoutNodeManager::SetMeasureFunc(
    int32_t id, std::unique_ptr<MeasureFunc> measure_func) {
  auto* element = GetFiberElement(id);
  if (element && element->IsShadowNodeCustom()) {
    element->SetMeasureFunc(std::move(measure_func));
  }
}

void ElementLayoutNodeManager::MarkDirtyAndRequestLayout(int32_t id) {
  auto* element = GetFiberElement(id);
  if (element) {
    element->MarkLayoutDirty();
  }
}

void ElementLayoutNodeManager::MarkDirtyAndForceLayout(int32_t id) {
  auto* element = GetFiberElement(id);
  if (element) {
    element->MarkLayoutDirty();
  }
}

bool ElementLayoutNodeManager::IsDirty(int32_t id) { return false; }

FlexDirection ElementLayoutNodeManager::GetFlexDirection(int32_t id) {
  return FlexDirection::kRow;
}

float ElementLayoutNodeManager::GetWidth(int32_t id) { return 0; }

float ElementLayoutNodeManager::GetHeight(int32_t id) { return 0; }

float ElementLayoutNodeManager::GetMinWidth(int32_t id) { return 0; }

float ElementLayoutNodeManager::GetMaxWidth(int32_t id) { return 0; }

float ElementLayoutNodeManager::GetMinHeight(int32_t id) { return 0; }

float ElementLayoutNodeManager::GetMaxHeight(int32_t id) { return 0; }

float ElementLayoutNodeManager::GetPaddingLeft(int32_t id) { return 0; }

float ElementLayoutNodeManager::GetPaddingTop(int32_t id) { return 0; }

float ElementLayoutNodeManager::GetPaddingRight(int32_t id) { return 0; }

float ElementLayoutNodeManager::GetPaddingBottom(int32_t id) { return 0; }

float ElementLayoutNodeManager::GetMarginLeft(int32_t id) { return 0; }

float ElementLayoutNodeManager::GetMarginTop(int32_t id) { return 0; }

float ElementLayoutNodeManager::GetMarginRight(int32_t id) { return 0; }

float ElementLayoutNodeManager::GetMarginBottom(int32_t id) { return 0; }

LayoutResult ElementLayoutNodeManager::UpdateMeasureByPlatform(
    int32_t id, float width, int32_t width_mode, float height,
    int32_t height_mode, bool final_measure) {
  return LayoutResult();
}

void ElementLayoutNodeManager::AlignmentByPlatform(int32_t id, float offset_top,
                                                   float offset_left) {}

void ElementLayoutNodeManager::DestroyLayoutNode(int32_t id) {
  destroyed_layout_node_ids_.insert(id);
}

void ElementLayoutNodeManager::DestroyPlatformLayoutNodes() {
  if (!destroyed_layout_node_ids_.empty()) {
    element_manager_.layout_context()->DestroyLayoutNodes(
        destroyed_layout_node_ids_);
    destroyed_layout_node_ids_.clear();
  }
}

FiberElement* ElementLayoutNodeManager::GetFiberElement(int32_t id) const {
  return reinterpret_cast<FiberElement*>(
      element_manager_.node_manager()->Get(id));
}

}  // namespace tasm
}  // namespace lynx
