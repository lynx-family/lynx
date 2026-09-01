// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_PLATFORM_WINDOWS_COVER_VIEW_PLATFORM_PLUGIN_WIN_H_
#define CLAY_SHELL_PLATFORM_WINDOWS_COVER_VIEW_PLATFORM_PLUGIN_WIN_H_

#include <memory>

#include "clay/common/service/service_manager.h"
#include "clay/shell/platform/windows/overlay_view_manager_service.h"
#include "clay/ui/platform/cover_view_platform_service.h"

namespace clay {

class CoverViewPlatformPluginWin final : public CoverViewPlatformPlugin {
 public:
  CoverViewPlatformPluginWin(FlutterWindowsEngine* engine,
                             OverlayViewManager* manager);
  ~CoverViewPlatformPluginWin() override = default;

  void Initialize(int id) override;
  void SetPreferredSize(int width, int height) override;
  void OnAttachToTree() override {}
  void OnDetachFromTree() override;
  void OnViewDestroy() override;

 private:
  void ChangeVisibility(bool visible);
  void EnsureView();

  int64_t node_id_ = -1;
  int preferred_width_ = 0;
  int preferred_height_ = 0;
  FlutterWindowsEngine* engine_ = nullptr;
  OverlayViewManager* manager_ = nullptr;
  std::shared_ptr<OverlayViewController> view_;
};

class CoverViewPlatformServiceWin final : public CoverViewPlatformService {
 public:
  std::unique_ptr<CoverViewPlatformPlugin> CreateCoverViewPlatformPlugin()
      override;

  void OnInit(ServiceManager& service_manager,
              const PlatformServiceContext& ctx) override;
  void OnDestroy() override;

 private:
  Puppet<Owner::kPlatform, OverlayViewManagerService>
      overlay_view_manager_service_;
};

}  // namespace clay

#endif  // CLAY_SHELL_PLATFORM_WINDOWS_COVER_VIEW_PLATFORM_PLUGIN_WIN_H_
