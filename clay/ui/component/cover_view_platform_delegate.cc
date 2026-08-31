// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/cover_view_platform_delegate.h"

#include <string>

#include "clay/common/service/service_manager.h"
#include "clay/ui/component/page_view.h"

namespace clay {

CoverViewPlatformDelegate::CoverViewPlatformDelegate(int id,
                                                     PageView* page_view)
    : weak_factory_(this) {
  if (!page_view || !page_view->GetServiceManager()) {
    return;
  }
  Puppet<Owner::kUI, OverlayService> overlay_service =
      page_view->GetServiceManager()->GetService<OverlayService>();
  if (!overlay_service) {
    return;
  }
  overlay_plugin_ = overlay_service.CreateObjectInActorThread(
      [](auto& service) { return service.CreateOverlayPlatformPlugin(); });
  overlay_plugin_.PostObjectToActorThread(
      fml::WeakPtr<OverlayPlatformPlugin::OverlayListener>(GetWeakPtr()),
      [id](auto& plugin, auto listener) {
        plugin.InitPlatformOverlay(std::move(listener), id, "cover-view",
                                   nullptr);
      });
}

void CoverViewPlatformDelegate::SetEventThrough(bool event_through) {
  if (overlay_plugin_) {
    overlay_plugin_.Act([event_through](auto& plugin) {
      plugin.SetEventThrough(event_through);
    });
  }
}

void CoverViewPlatformDelegate::SetPreferredSize(int width, int height) {
  if (overlay_plugin_ && width > 0 && height > 0) {
    overlay_plugin_.Act([width, height](auto& plugin) {
      plugin.SetPreferredSize(width, height);
    });
  }
}

void CoverViewPlatformDelegate::OnAttachToTree() {
  if (overlay_plugin_) {
    overlay_plugin_.Act([](auto& plugin) { plugin.OnAttachToTree(); });
  }
}

void CoverViewPlatformDelegate::OnDetachFromTree() {
  if (overlay_plugin_) {
    overlay_plugin_.Act([](auto& plugin) { plugin.OnDetachFromTree(); });
  }
}

void CoverViewPlatformDelegate::OnDestroy() {
  if (overlay_plugin_) {
    overlay_plugin_.Act([](auto& plugin) { plugin.OnViewDestroy(); });
  }
}

}  // namespace clay
