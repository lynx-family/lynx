// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_COMMON_SERVICES_INITIALIZE_SERVICE_H_
#define CLAY_SHELL_COMMON_SERVICES_INITIALIZE_SERVICE_H_

#include <memory>

#include "clay/common/service/service.h"
#include "clay/common/service/service_manager.h"
#include "clay/gfx/gpu_object.h"
#include "clay/ui/shadow/shadow_node_owner.h"

namespace clay {

class PageView;
class ViewContext;

class InitializeService : public Service<InitializeService, clay::Owner::kUI,
                                         clay::ServiceFlags::kMultiThread> {
 public:
  static std::shared_ptr<InitializeService> Create();

  std::unique_ptr<PageView> CreatePageView(
      uint32_t id, std::shared_ptr<ServiceManager> service_manager,
      fml::RefPtr<GPUUnrefQueue> unref_queue,
      clay::TaskRunners task_runners) const;

  std::shared_ptr<ViewContext> CreateViewContext(
      PageView* root, ShadowNodeOwner* shadow_node_owner);

 private:
  void OnInit(ServiceManager&, const UIServiceContext& ctx) override;
};

}  // namespace clay

#endif  // CLAY_SHELL_COMMON_SERVICES_INITIALIZE_SERVICE_H_
