// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/common/services/initialize_service.h"

#include "clay/ui/component/page_view.h"
#include "clay/ui/component/view_context.h"

namespace clay {
// static
std::shared_ptr<InitializeService> InitializeService::Create() {
  return std::make_shared<InitializeService>();
}

std::unique_ptr<PageView> InitializeService::CreatePageView(
    uint32_t id, std::shared_ptr<ServiceManager> service_manager,
    fml::RefPtr<GPUUnrefQueue> unref_queue,
    clay::TaskRunners task_runners) const {
  return std::make_unique<PageView>(id, service_manager, unref_queue,
                                    task_runners);
}

std::shared_ptr<ViewContext> InitializeService::CreateViewContext(
    PageView* root, ShadowNodeOwner* shadow_node_owner) {
  return std::make_shared<ViewContext>(root, shadow_node_owner);
}

void InitializeService::OnInit(ServiceManager&, const UIServiceContext& ctx) {}

}  // namespace clay
