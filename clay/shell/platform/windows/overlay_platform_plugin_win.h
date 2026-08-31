// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_PLATFORM_WINDOWS_OVERLAY_PLATFORM_PLUGIN_WIN_H_
#define CLAY_SHELL_PLATFORM_WINDOWS_OVERLAY_PLATFORM_PLUGIN_WIN_H_

#include <memory>
#include <string>

#include "clay/common/service/service_manager.h"
#include "clay/shell/platform/windows/overlay_view_manager_service.h"
#include "clay/ui/platform/overlay_service.h"

namespace clay {

class OverlayPlatformPluginWin final : public OverlayPlatformPlugin {
 public:
  OverlayPlatformPluginWin(FlutterWindowsEngine* engine,
                           OverlayViewManager* manager);
  ~OverlayPlatformPluginWin() override = default;

  void ChangeVisibility(bool visible) override;
  void SetLevel(int level) override {}
  void SetCutOutMode(bool is_cut_out) override {}
  void SetAndroidSoftInputMode(std::string mode) override {}
  void SetAndroidNativeEventPass(bool is_pass) override {}
  void SetStatusBarTranslucent(bool is_translucent) override {}
  void SetStatusBarTranslucentStyle(std::string style) override {}
  void SetAndroidFullScreen(bool is_full_screen) override {}
  void SetEventThrough(bool event_through) override;
  void SetPreferredSize(int width, int height) override;

  bool ShouldHandleTreeLifecycle() const override { return true; }
  bool RequiresExternalViewPlugin() const override { return false; }
  void OnAttachToTree() override {}
  void OnDetachFromTree() override;
  void OnViewDestroy() override;

  void InitPlatformOverlay(
      std::shared_ptr<Actor<fml::WeakPtr<OverlayListener>>> overlay_listener,
      int id, std::string tag, ExternalViewPlugin* recording_plugin) override;

 private:
  void EnsureView();

  int64_t node_id_ = -1;
  int preferred_width_ = 0;
  int preferred_height_ = 0;
  bool event_through_ = false;
  FlutterWindowsEngine* engine_ = nullptr;
  OverlayViewManager* manager_ = nullptr;
  std::shared_ptr<OverlayViewController> view_;
};

class OverlayPlatformServiceWin final : public OverlayService {
 public:
  std::unique_ptr<OverlayPlatformPlugin> CreateOverlayPlatformPlugin() override;

  void OnInit(ServiceManager& service_manager,
              const PlatformServiceContext& ctx) override;
  void OnDestroy() override;

 private:
  Puppet<Owner::kPlatform, OverlayViewManagerService>
      overlay_view_manager_service_;
};

}  // namespace clay

#endif  // CLAY_SHELL_PLATFORM_WINDOWS_OVERLAY_PLATFORM_PLUGIN_WIN_H_
