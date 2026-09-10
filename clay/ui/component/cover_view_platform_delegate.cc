// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/cover_view_platform_delegate.h"

#include "clay/common/service/service_manager.h"
#include "clay/ui/component/page_view.h"

namespace clay {

CoverViewPlatformDelegate::CoverViewPlatformDelegate(int id,
                                                     PageView* page_view) {
  if (!page_view || !page_view->GetServiceManager()) {
    return;
  }
  Puppet<Owner::kUI, CoverViewPlatformService> platform_service =
      page_view->GetServiceManager()->GetService<CoverViewPlatformService>();
  if (!platform_service) {
    return;
  }
  platform_plugin_ = platform_service.CreateObjectInActorThread(
      [](auto& service) { return service.CreateCoverViewPlatformPlugin(); });
  platform_plugin_.Act([id](auto& plugin) { plugin.Initialize(id); });
}

void CoverViewPlatformDelegate::SetEventsPassThrough(bool events_pass_through) {
  if (platform_plugin_) {
    platform_plugin_.Act([events_pass_through](auto& plugin) {
      plugin.SetEventsPassThrough(events_pass_through);
    });
  }
}

void CoverViewPlatformDelegate::SetPreferredSize(int width, int height) {
  if (platform_plugin_ && width > 0 && height > 0) {
    platform_plugin_.Act([width, height](auto& plugin) {
      plugin.SetPreferredSize(width, height);
    });
  }
}

void CoverViewPlatformDelegate::OnAttachToTree() {
  if (platform_plugin_) {
    platform_plugin_.Act([](auto& plugin) { plugin.OnAttachToTree(); });
  }
}

void CoverViewPlatformDelegate::OnDetachFromTree() {
  if (platform_plugin_) {
    platform_plugin_.Act([](auto& plugin) { plugin.OnDetachFromTree(); });
  }
}

void CoverViewPlatformDelegate::OnDestroy() {
  if (platform_plugin_) {
    platform_plugin_.Act([](auto& plugin) { plugin.OnViewDestroy(); });
  }
}

}  // namespace clay
