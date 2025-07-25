// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RENDERER_DOM_ELEMENT_LAYOUT_NODE_MANAGER_H_
#define CORE_RENDERER_DOM_ELEMENT_LAYOUT_NODE_MANAGER_H_

#include <memory>
#include <unordered_set>

#include "core/public/layout_node_manager.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/fiber_element.h"

namespace lynx {
namespace tasm {

class ElementLayoutNodeManager : public LayoutNodeManager {
 public:
  explicit ElementLayoutNodeManager(ElementManager& element_manager);
  ~ElementLayoutNodeManager() override = default;

  void SetMeasureFunc(int32_t id,
                      std::unique_ptr<MeasureFunc> measure_func) override;
  void MarkDirtyAndRequestLayout(int32_t id) override;
  void MarkDirtyAndForceLayout(int32_t id) override;
  bool IsDirty(int32_t id) override;
  FlexDirection GetFlexDirection(int32_t id) override;
  float GetWidth(int32_t id) override;
  float GetHeight(int32_t id) override;
  float GetMinWidth(int32_t id) override;
  float GetMaxWidth(int32_t id) override;
  float GetMinHeight(int32_t id) override;
  float GetMaxHeight(int32_t id) override;
  float GetPaddingLeft(int32_t id) override;
  float GetPaddingTop(int32_t id) override;
  float GetPaddingRight(int32_t id) override;
  float GetPaddingBottom(int32_t id) override;
  float GetMarginLeft(int32_t id) override;
  float GetMarginTop(int32_t id) override;
  float GetMarginRight(int32_t id) override;
  float GetMarginBottom(int32_t id) override;
  LayoutResult UpdateMeasureByPlatform(int32_t id, float width,
                                       int32_t width_mode, float height,
                                       int32_t height_mode,
                                       bool final_measure) override;
  void AlignmentByPlatform(int32_t id, float offset_top,
                           float offset_left) override;
  void DestroyLayoutNode(int32_t id);
  void DestroyPlatformLayoutNodes();

 private:
  FiberElement* GetFiberElement(int32_t id) const;
  std::unordered_set<int32_t> destroyed_layout_node_ids_;
  ElementManager& element_manager_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_ELEMENT_LAYOUT_NODE_MANAGER_H_
