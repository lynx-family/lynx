// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/hierarchy_observer_impl.h"

#include "devtool/lynx_devtool/agent/inspector_ui_executor.h"
#include "devtool/lynx_devtool/agent/inspector_util.h"

namespace lynx {
namespace devtool {

HierarchyObserverImpl::HierarchyObserverImpl(
    const std::shared_ptr<InspectorUIExecutor>& ui_executor)
    : ui_executor_wp_(ui_executor) {}

void HierarchyObserverImpl::OnLayoutNodeCreated(int32_t id,
                                                tasm::LayoutNode* ptr) {
  auto ui_executor = ui_executor_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN(ui_executor, "ui_executor is null");
  ui_executor->OnLayoutNodeCreated(id, ptr);
}

void HierarchyObserverImpl::OnLayoutNodeDestroy(int32_t id) {
  auto ui_executor = ui_executor_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN(ui_executor, "ui_executor is null");
  ui_executor->OnLayoutNodeDestroy(id);
}

void HierarchyObserverImpl::OnComponentUselessUpdate(
    const std::string& component_name, const lepus::Value& properties) {
  auto ui_executor = ui_executor_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN(ui_executor, "ui_executor is null");
  ui_executor->OnComponentUselessUpdate(component_name, properties);
}

}  // namespace devtool
}  // namespace lynx
