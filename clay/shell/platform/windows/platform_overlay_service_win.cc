// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/platform/windows/platform_overlay_service_win.h"

#include <Windows.h>
#include <dwmapi.h>

#include <cmath>

#include "base/include/fml/synchronization/count_down_latch.h"
#include "clay/fml/logging.h"
#include "clay/shell/platform/embedder/embedder_surface_gl.h"
#include "clay/shell/platform/embedder/platform_view_embedder.h"
#include "clay/shell/platform/windows/egl/manager.h"

namespace clay {

PlatformOverlayWin::PlatformOverlayWin(
    FlutterWindowsEngine* engine,
    fml::RefPtr<fml::TaskRunner> platform_task_runner,
    OverlayViewManager* manager)
    : engine_(engine),
      platform_task_runner_(platform_task_runner),
      manager_(manager) {
  embedder_surface_ = fml::RefPtr<EmbedderSurface>(
      new EmbedderSurfaceGL((static_cast<GPUSurfaceGLDelegate*>(this))));
}

PlatformOverlayWin::~PlatformOverlayWin() {
  if (!embedder_surface_) {
    return;
  }
  auto gr_context = embedder_surface_->GetMainGrContext();
  if (gr_context) {
#ifndef ENABLE_SKITY
    bool has_released = false;
#ifdef SHELL_ENABLE_GL
    if (gr_context->backend() == GrBackendApi::kOpenGL) {
      auto status = GLContextMakeCurrent();
      if (status && status->GetResult()) {
        gr_context->releaseResourcesAndAbandonContext();
        GLContextClearCurrent();
        has_released = true;
      }
    }
#endif
    if (!has_released) {
      gr_context->releaseResourcesAndAbandonContext();
    }
#endif
  }
}

void PlatformOverlayWin::PrepareSurface(const OverlayData& data) {
  int64_t view_id = data.view_id;
  int width = static_cast<int>(std::ceil(data.rect.Width()));
  int height = static_cast<int>(std::ceil(data.rect.Height()));
  if (view_id_.load() != view_id) {
    fml::CountDownLatch latch(1);
    fml::TaskRunner::RunNowOrPostTask(platform_task_runner_, [&]() {
      FML_DCHECK(platform_task_runner_->RunsTasksOnCurrentThread());
      auto view = manager_->GetView(view_id);
      OverlayWindowCreationRequest request = {.preferred_size = {width, height},
                                              .title = L""};
      if (view == nullptr) {
        view = manager_->CreateView(view_id, OverlayWindowType::kChild, request,
                                    engine_->view()->GetWindowHandle());
      } else {
        manager_->RemoveView(view_id);
        view = manager_->CreateView(view_id, OverlayWindowType::kChild, request,
                                    engine_->view()->GetWindowHandle());
      }
      view_id_.store(view_id);
      latch.CountDown();
    });
    latch.Wait();
  }
}

std::unique_ptr<GLContextResult> PlatformOverlayWin::GLContextMakeCurrent() {
  auto view = manager_->GetView(view_id_.load());
  if (!view) {
    return std::make_unique<GLContextDefaultResult>(false);
  }
  return std::make_unique<GLContextDefaultResult>(
      view->GetView()->MakeCurrent());
}

bool PlatformOverlayWin::GLContextClearCurrent() {
  if (!engine_->egl_manager()) {
    return false;
  }
  return engine_->egl_manager()->render_context()->ClearCurrent();
}

void PlatformOverlayWin::GLContextSetDamageRegion(
    const std::optional<skity::Rect>& region) {
  auto view = manager_->GetView(view_id_.load());
  if (!region.has_value() || !view) {
    return;
  }
  skity::Rect damage_rect = region.value();
  int left = static_cast<int>(std::floor(damage_rect.Left()));
  int top = static_cast<int>(std::floor(damage_rect.Top()));
  int right = static_cast<int>(std::ceil(damage_rect.Right()));
  int bottom = static_cast<int>(std::ceil(damage_rect.Bottom()));
  view->GetView()->SetDamageRegion({left, top, right - left, bottom - top});
}

bool PlatformOverlayWin::GLContextPresent(const GLPresentInfo& present_info) {
  auto view = manager_->GetView(view_id_.load());
  if (view) {
    return view->GetView()->SwapBuffers();
  }
  return false;
}

GLFBOInfo PlatformOverlayWin::GLContextFBO(GLFrameInfo frame_info) const {
  auto view = manager_->GetView(view_id_.load());
  if (!view) {
    return {.fbo_id = kWindowFrameBufferID, .existing_damage = std::nullopt};
  }
  auto damage_rect = view->GetView()->GetDamageRegion();
  return {.fbo_id = view->GetView()->GetFrameBufferId(frame_info.width,
                                                      frame_info.height),
          .existing_damage = damage_rect};
}

void PlatformOverlayWin::DisplayOverlaySurface(int x, int y, int width,
                                               int height) {
  FML_DCHECK(platform_task_runner_->RunsTasksOnCurrentThread());
  auto view = manager_->GetView(view_id_.load());
  if (view) {
    if (view->GetType() == OverlayWindowType::kChild) {
      view->UpdatePosition(x, y, width, height);
    }
    if (!IsWindowVisible(view->GetWindowHandle())) {
      ShowWindow(view->GetWindowHandle(), SW_SHOW);
    }
  }
}

void PlatformOverlayWin::BringToFront() const {
  FML_DCHECK(platform_task_runner_->RunsTasksOnCurrentThread());
  auto view = manager_->GetView(view_id_.load());
  if (!view) {
    return;
  }
  SetWindowPos(view->GetWindowHandle(), HWND_TOP, 0, 0, 0, 0,
               SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

void PlatformOverlayWin::RemoveFromParent() const {
  FML_DCHECK(platform_task_runner_->RunsTasksOnCurrentThread());
  auto view = manager_->GetView(view_id_.load());
  if (!view) {
    return;
  }
  HWND window_handle = view->GetWindowHandle();
  if (IsWindowVisible(window_handle)) {
    ShowWindow(window_handle, SW_HIDE);
  }
}

void PlatformOverlayWin::OnSurfaceUpdated() {
  // Windows GL overlays present through GLContextPresent().
}

std::vector<std::shared_ptr<PlatformOverlay>>
PlatformOverlayServiceWin::CreatePlatformOverlay(size_t num) {
  fml::CountDownLatch latch(1);
  std::vector<std::shared_ptr<PlatformOverlay>> overlays;
  fml::TaskRunner::RunNowOrPostTask(platform_task_runner_, [&]() {
    overlays.reserve(num);
    for (size_t i = 0; i < num; i++) {
      overlays.push_back(std::make_shared<PlatformOverlayWin>(
          overlay_view_manager_service_->GetEngine(), platform_task_runner_,
          overlay_view_manager_service_->GetOverlayWindowManager()));
    }
    latch.CountDown();
  });
  latch.Wait();
  return overlays;
}

void PlatformOverlayServiceWin::OnInit(
    clay::ServiceManager& service_manager,
    const clay::PlatformServiceContext& ctx) {
  platform_task_runner_ = service_manager.GetTaskRunners()
                              ->SelectTaskRunner<clay::Owner::kPlatform>();
  overlay_view_manager_service_ = ctx.platform_view->GetServiceManager()
                                      ->GetService<OverlayViewManagerService>();
}

void PlatformOverlayServiceWin::OnDestroy() { platform_task_runner_ = nullptr; }

std::shared_ptr<PlatformOverlayService> PlatformOverlayService::Create() {
  return std::make_shared<PlatformOverlayServiceWin>();
}

}  // namespace clay
