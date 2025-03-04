// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_AGENT_HIERARCHY_OBSERVER_IMPL_H_
#define DEVTOOL_LYNX_DEVTOOL_AGENT_HIERARCHY_OBSERVER_IMPL_H_

#include <memory>
#include <string>

#include "core/renderer/dom/element_manager.h"

namespace lynx {

namespace tasm {
class Element;
class LayoutNode;
}  // namespace tasm

namespace devtool {

class InspectorUIExecutor;

class HierarchyObserverImpl : public tasm::HierarchyObserver {
 public:
  HierarchyObserverImpl() = default;
  explicit HierarchyObserverImpl(
      const std::shared_ptr<InspectorUIExecutor>& ui_executor);
  ~HierarchyObserverImpl() override = default;

  void OnLayoutNodeCreated(int32_t id, tasm::LayoutNode* ptr) override;
  void OnLayoutNodeDestroy(int32_t id) override;

  void OnComponentUselessUpdate(const std::string& component_name,
                                const lepus::Value& properties) override;

 private:
  std::weak_ptr<InspectorUIExecutor> ui_executor_wp_;
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_AGENT_HIERARCHY_OBSERVER_IMPL_H_
