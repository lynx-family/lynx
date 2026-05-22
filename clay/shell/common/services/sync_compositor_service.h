// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_COMMON_SERVICES_SYNC_COMPOSITOR_SERVICE_H_
#define CLAY_SHELL_COMMON_SERVICES_SYNC_COMPOSITOR_SERVICE_H_
#include <memory>

#include "clay/common/service/service.h"

namespace clay {

class SyncCompositorService
    : public clay::Service<SyncCompositorService, clay::Owner::kPlatform> {
 public:
  static std::shared_ptr<SyncCompositorService> Create();
  void Invalidate(bool is_raster_frame);

  void OnFirstMeaningfulLayout();
  void DemandDrawHw(bool force_draw);

 private:
  void OnInit(clay::ServiceManager& service_manager,
              const clay::PlatformServiceContext& ctx) override;

  Shell* shell_ = nullptr;
  PlatformView* platform_view_ = nullptr;
  bool first_meaningful_layout_ = false;
  bool has_invalidated_ = false;
};

}  // namespace clay

#endif  // CLAY_SHELL_COMMON_SERVICES_SYNC_COMPOSITOR_SERVICE_H_
