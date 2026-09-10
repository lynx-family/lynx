// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_PLATFORM_DARWIN_MACOS_COVER_VIEW_PLATFORM_PLUGIN_MAC_H_
#define CLAY_SHELL_PLATFORM_DARWIN_MACOS_COVER_VIEW_PLATFORM_PLUGIN_MAC_H_

#include <memory>

#include "clay/common/service/service_manager.h"
#include "clay/shell/platform/darwin/macos/framework/Source/overlay_view_controller_service.h"
#include "clay/ui/platform/cover_view_platform_service.h"

namespace clay {

class CoverViewPlatformPluginMac final : public CoverViewPlatformPlugin {
 public:
  explicit CoverViewPlatformPluginMac(ClayOverlayView* overlay_view);
  ~CoverViewPlatformPluginMac() override = default;

  void Initialize(int id) override;
  void SetEventsPassThrough(bool events_pass_through) override;
  void OnAttachToTree() override {}
  void OnDetachFromTree() override;
  void OnViewDestroy() override;

 private:
  int64_t node_id_ = -1;
  bool events_pass_through_ = false;
  __weak ClayOverlayView* overlay_view_ = nil;
};

class CoverViewPlatformServiceMac final : public CoverViewPlatformService {
 public:
  std::unique_ptr<CoverViewPlatformPlugin> CreateCoverViewPlatformPlugin()
      override;

  void OnInit(ServiceManager& service_manager,
              const PlatformServiceContext& ctx) override;
  void OnDestroy() override;

 private:
  Puppet<Owner::kPlatform, OverlayViewControllerService>
      overlay_view_controller_service_;
};

}  // namespace clay

#endif  // CLAY_SHELL_PLATFORM_DARWIN_MACOS_COVER_VIEW_PLATFORM_PLUGIN_MAC_H_
