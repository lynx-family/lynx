// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_PLATFORM_WINDOWS_PLATFORM_OVERLAY_SERVICE_WIN_H_
#define CLAY_SHELL_PLATFORM_WINDOWS_PLATFORM_OVERLAY_SERVICE_WIN_H_

#include <atomic>
#include <memory>
#include <vector>

#include "clay/common/service/service.h"
#include "clay/common/service/service_manager.h"
#include "clay/shell/common/services/compositor/compositor_service.h"
#include "clay/shell/common/services/compositor/platform_overlay_service.h"
#include "clay/shell/platform/embedder/embedder_surface.h"
#include "clay/shell/platform/windows/flutter_window.h"
#include "clay/shell/platform/windows/flutter_windows_engine.h"
#include "clay/shell/platform/windows/overlay_view_manager_service.h"
#include "clay/shell/platform/windows/window_binding_handler.h"

namespace clay {

class PlatformOverlayWin final : public PlatformOverlay,
                                 public GPUSurfaceGLDelegate {
 public:
  PlatformOverlayWin(FlutterWindowsEngine* engine,
                     fml::RefPtr<fml::TaskRunner> platform_task_runner,
                     OverlayViewManager* manager);
  ~PlatformOverlayWin() override;
  void PrepareSurface(const OverlayData& data) override;
  std::unique_ptr<GLContextResult> GLContextMakeCurrent() override;
  bool GLContextClearCurrent() override;
  void GLContextSetDamageRegion(
      const std::optional<skity::Rect>& region) override;
  bool GLContextPresent(const GLPresentInfo& present_info) override;
  GLFBOInfo GLContextFBO(GLFrameInfo frame_info) const override;
  void BringToFront() const;
  void RemoveFromParent() const;
  void DisplayOverlaySurface(int x, int y, int width, int height);

 private:
  void OnSurfaceUpdated() override;
  fml::RefPtr<OutputSurface> GetOutputSurface() const override {
    return embedder_surface_;
  }

  FlutterWindowsEngine* engine_ = nullptr;
  fml::RefPtr<EmbedderSurface> embedder_surface_;
  fml::RefPtr<fml::TaskRunner> platform_task_runner_;
  std::atomic<int64_t> view_id_ = -1;
  OverlayViewManager* manager_ = nullptr;
};

class PlatformOverlayServiceWin final : public PlatformOverlayService {
 private:
  std::vector<std::shared_ptr<PlatformOverlay>> CreatePlatformOverlay(
      size_t num) override;
  void OnInit(clay::ServiceManager& service_manager,
              const clay::PlatformServiceContext& ctx) override;
  void OnDestroy() override;

  fml::RefPtr<fml::TaskRunner> platform_task_runner_;
  clay::Puppet<clay::Owner::kPlatform, OverlayViewManagerService>
      overlay_view_manager_service_;
};

}  // namespace clay

#endif  // CLAY_SHELL_PLATFORM_WINDOWS_PLATFORM_OVERLAY_SERVICE_WIN_H_
