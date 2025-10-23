// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_EXAMPLE_GLFW_SHELL_IMPL_H_
#define CLAY_EXAMPLE_GLFW_SHELL_IMPL_H_

#include <memory>

#include "build/build_config.h"
#include "clay/common/service/service_manager.h"
#include "clay/common/task_runners.h"
#include "clay/common/thread_host.h"
#include "clay/example/glfw/surface_gl_impl.h"
#include "clay/example/glfw/thread_host_holder.h"
#include "clay/public/clay.h"
#include "clay/shell/common/engine.h"
#include "clay/shell/common/platform_view.h"
#include "clay/shell/common/rasterizer.h"
#include "clay/shell/common/shell.h"
#include "clay/ui/window/viewport_metrics.h"

namespace clay {
namespace example {

class ShellImpl {
 public:
  ShellImpl(const char* icu_data_path, SurfaceDelegate* delegate,
            ClayTaskRunnerDescription* platform_task_runner_desc);
  ~ShellImpl();

  clay::Shell& GetShell() { return *shell_.get(); }
  const clay::TaskRunners& GetTaskRunners() const { return task_runners_; }
  const std::shared_ptr<clay::ServiceManager>& GetServiceManager() const {
    return service_manager_;
  }

  void NotifyCreated() {
    if (shell_) {
      shell_->GetPlatformView()->NotifyCreated();
    }
  }
  void NotifyDestroyed() {
    if (shell_) {
      shell_->GetPlatformView()->NotifyDestroyed();
    }
  }
  void SetViewportMetrics(const clay::ViewportMetrics& metrics) {
    if (shell_) {
      shell_->GetPlatformView()->SetViewportMetrics(metrics);
    }
  }

  void ScheduleFrame() {
    if (shell_) {
      shell_->GetPlatformView()->ScheduleFrame();
    }
  }

  bool RunTask(const ClayTask* task);

  void SendViewportMetrics(int32_t width, int32_t height, double pixel_ratio);
  void SendPointerEvents(const ClayPointerEvent* events, size_t events_count);
  void DrawUI();

 private:
  const std::unique_ptr<ThreadHostHolder> thread_host_;
  clay::TaskRunners task_runners_;
  std::shared_ptr<clay::ServiceManager> service_manager_;
  std::unique_ptr<clay::Shell> shell_;

  BASE_DISALLOW_COPY_AND_ASSIGN(ShellImpl);
};

}  // namespace example
}  // namespace clay

#endif  // CLAY_EXAMPLE_GLFW_SHELL_IMPL_H_
