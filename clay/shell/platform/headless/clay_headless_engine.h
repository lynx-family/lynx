// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_PLATFORM_HEADLESS_CLAY_HEADLESS_ENGINE_H_
#define CLAY_SHELL_PLATFORM_HEADLESS_CLAY_HEADLESS_ENGINE_H_

#include <chrono>
#include <map>
#include <memory>

#include "base/include/fml/macros.h"
#include "clay/common/service/service_manager.h"
#include "clay/common/task_runners.h"
#include "clay/public/clay.h"
#include "clay/shell/platform/embedder/embedder_engine.h"
#include "clay/shell/platform/embedder/platform_view_embedder_delegate.h"
#include "clay/shell/platform/headless/clay_event_loop.h"
#include "clay/shell/platform/headless/public/embdder_headless_delegate.h"

namespace lynx {
class EmbedderLynxView;
}  // namespace lynx

namespace clay {

class ClayHeadlessRenderer;

class ClayHeadlessEngine : public clay::PlatformViewEmbedderDelegate {
 public:
  ClayHeadlessEngine(
      const char* icu_data_path,
      const ClayHeadlessRendererConfig* renderer_config,
      const ClayTaskRunnerDescription* platform_task_runner_description,
      void* user_data);
  ~ClayHeadlessEngine() override;

  bool IsValid() const;
  void* UserData() const { return user_data_; }
  ClayHeadlessRenderer* Renderer() const;

  void EnableDefaultFocusRing();
  void EnablePerformanceOverlay();
  void OnEnterForeground();
  void OnEnterBackground();
  void NotifyLowMemoryWarning();
  void NotifyLocaleChange();
  void RequestPaint();

  void SetVisible(bool enable);
  void SetFontFaceCache(const char* font_family, const char* local_path);

  void SendViewportMetrics(int32_t width, int32_t height, double pixel_ratio);
  void SendPointerEvents(const ClayPointerEvent* events, size_t events_count);
  void SendKeyEvent(const ClayKeyEvent* event, ClayKeyEventCallback callback,
                    void* user_data);
  void PostPlatformThreadTask(ClayVoidCallback callback, void* callback_data);
  void* GetViewContext();

  // Processes the next event for the engine, or returns early if |timeout| is
  // reached before the next event.
  void RunEventLoopWithTimeout(size_t timeout_milliseconds = 0);
  bool RunTask(const ClayTask* task);
  const TaskRunners& GetTaskRunners() const;

  static uint64_t GetEngineCurrentTime();

  void SetHeadlessDelegate(lynx::HeadlessDelegate* delegate) {
    delegate_ = delegate;
  }

  const std::shared_ptr<clay::ServiceManager>& GetServiceManager() const {
    return service_manager_;
  }

 private:
  bool RunEngine(
      const char* icu_data_path,
      const ClayHeadlessRendererConfig* renderer_config,
      const ClayTaskRunnerDescription* platform_task_runner_description,
      void* user_data);
  bool StopEngine();

  const char* GetClipboardData() const;
  void SetClipboardData(const char* data);
  void ActivateSystemCursor(int type, const char* path);
  void ShowTextInput();
  void HideTextInput();
  void SetMarkedTextRect(float x, float y, float width, float height);
  void SetEditableTransform(const float transform_matrix[16]);
  void SetTextInputClient(int client_id, const char* input_action,
                          const char* input_type);
  void ClearTextInputClient();

  void* user_data_ = nullptr;
  int client_id_ = -1;

  // The event loop for the main thread that allows for delayed task execution.
  std::unique_ptr<ClayEventLoop> event_loop_;

  // The interface between the Flutter rasterizer and the platform.
  std::unique_ptr<ClayHeadlessRenderer> renderer_;
  // The Flutter engine instance.
  std::unique_ptr<EmbedderEngine> engine_;

  std::shared_ptr<clay::ServiceManager> service_manager_;

  lynx::HeadlessDelegate* delegate_ = nullptr;

  BASE_DISALLOW_COPY_AND_ASSIGN(ClayHeadlessEngine);
};

}  // namespace clay

#endif  // CLAY_SHELL_PLATFORM_HEADLESS_CLAY_HEADLESS_ENGINE_H_
