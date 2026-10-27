// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/common/services/sync_compositor_service.h"

#include <utility>

#include "clay/shell/common/platform_view.h"
#include "clay/shell/common/services/ui_frame_service.h"
#include "clay/shell/common/shell.h"

namespace clay {

std::shared_ptr<SyncCompositorService> SyncCompositorService::Create() {
  return std::make_shared<SyncCompositorService>();
}

void SyncCompositorService::Invalidate(bool is_raster_frame) {
  if (shell_ && !has_invalidated_) {
    shell_->OnPostInvalidate(is_raster_frame);
    has_invalidated_ = true;
  }
}

void SyncCompositorService::OnFirstMeaningfulLayout() {
  first_meaningful_layout_ = true;
  shell_->OnPostInvalidate(false);
}

void SyncCompositorService::DemandDrawHw(bool force_draw) {
  has_invalidated_ = false;
  if (!first_meaningful_layout_) {
    // First meaningful layout is not ready, ignore this frame.
    return;
  }

  if (force_draw) {
    clay::Puppet<clay::Owner::kPlatform, RasterFrameService>
        raster_frame_service =
            shell_->GetServiceManager()->GetService<RasterFrameService>();
    raster_frame_service.Act(
        [](auto& impl) mutable { impl.RequestRasterFrame(); });
  }

  clay::Puppet<clay::Owner::kPlatform, UIFrameService> ui_frame_service =
      shell_->GetServiceManager()->GetService<UIFrameService>();
  // Submit a frame future, and it will be consumed in Android RenderThread.
  std::promise<bool> promise;
  platform_view_->SubmitFrameFuture(promise.get_future());
  // Now it's time to do BeginFrame and push LayerTree, which will be scheduled
  // by StateMachine.
  ui_frame_service.Act([promise = std::move(promise)](auto& impl) mutable {
    impl.DemandDrawHw(std::move(promise));
  });
}

void SyncCompositorService::OnInit(clay::ServiceManager& service_manager,
                                   const clay::PlatformServiceContext& ctx) {
  shell_ = ctx.shell;
  platform_view_ = ctx.platform_view;
}

}  // namespace clay
