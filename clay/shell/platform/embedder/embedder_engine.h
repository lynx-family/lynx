// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_PLATFORM_EMBEDDER_EMBEDDER_ENGINE_H_
#define CLAY_SHELL_PLATFORM_EMBEDDER_EMBEDDER_ENGINE_H_

#include <memory>
#include <unordered_map>

#include "base/include/fml/macros.h"
#include "clay/common/settings.h"
#include "clay/common/thread_host.h"
#include "clay/public/clay.h"
#include "clay/shell/common/shell.h"
#include "clay/shell/platform/embedder/embedder_thread_host.h"
#include "clay/shell/platform/embedder/platform_view_embedder.h"
namespace clay {

struct ShellArgs;

// The object that is returned to the embedder as an opaque pointer to the
// instance of the Flutter engine.
class EmbedderEngine {
 public:
#ifdef SHELL_ENABLE_METAL
  static std::unique_ptr<EmbedderEngine> CreateEngine(
      Settings& settings, const ClayCustomTaskRunners* custom_task_runners,
      fml::RefPtr<clay::EmbedderSurfaceMetal> embedder_surface,
      const clay::PlatformViewEmbedder::PlatformDispatchTable&
          platform_dispatch_table,
      PlatformViewEmbedderDelegate* platform_view_delegate, void* user_data);
#endif
#ifdef SHELL_ENABLE_GL
  static std::unique_ptr<EmbedderEngine> CreateEngine(
      Settings& settings, const ClayCustomTaskRunners* custom_task_runners,
      GPUSurfaceGLDelegate* gl_delegate,
      const clay::PlatformViewEmbedder::PlatformDispatchTable&
          platform_dispatch_table,
      PlatformViewEmbedderDelegate* platform_view_delegate, void* user_data);
#endif

  static std::unique_ptr<EmbedderEngine> CreateEngine(
      Settings& settings, const ClayCustomTaskRunners* custom_task_runners,
      EmbedderSurfaceSoftwareDelegate* software_delegate,
      const clay::PlatformViewEmbedder::PlatformDispatchTable&
          platform_dispatch_table,
      PlatformViewEmbedderDelegate* platform_view_delegate, void* user_data);

  EmbedderEngine(
      std::shared_ptr<clay::ServiceManager> service_manager,
      std::unique_ptr<EmbedderThreadHost> thread_host,
      const TaskRunners& task_runners, const Settings& settings,
      const Shell::CreateCallback<PlatformView>& on_create_platform_view,
      const Shell::CreateCallbackFnPtr<Rasterizer>& on_create_rasterizer);

  ~EmbedderEngine();

  bool LaunchShell();

  bool CollectShell();

  const TaskRunners& GetTaskRunners() const;

  bool NotifyCreated();

  bool NotifyDestroyed();

  bool IsValid() const;

  bool SetViewportMetrics(const clay::ViewportMetrics& metrics);

  bool SendPointerEvent(const ClayPointerEvent* pointers, size_t events_count);
  bool SendKeyEvent(const ClayKeyEvent* event, ClayKeyEventCallback callback,
                    void* user_data);

  bool DispatchPointerDataPacket(
      std::unique_ptr<clay::PointerDataPacket> packet);
  bool DispatchKeyDataPacket(std::unique_ptr<clay::KeyDataPacket> packet,
                             PlatformView::Delegate::KeyDataResponse callback);

  bool OnVsyncEvent(intptr_t baton, fml::TimePoint frame_start_time,
                    fml::TimePoint frame_target_time);

  bool ReloadSystemFonts();

  bool PostRenderThreadTask(lynx::base::closure task);

  bool PostPlatformThreadTask(lynx::base::closure task);

  bool PostUIThreadTask(lynx::base::closure task);

  bool RunTask(const ClayTask* task);

  bool ScheduleFrame();

  Shell& GetShell();

  const std::shared_ptr<clay::ServiceManager>& GetServiceManager() const;

 private:
  const std::shared_ptr<clay::ServiceManager> service_manager_;
  const std::unique_ptr<EmbedderThreadHost> thread_host_;
  TaskRunners task_runners_;
  std::unique_ptr<ShellArgs> shell_args_;
  std::unique_ptr<Shell> shell_;

  BASE_DISALLOW_COPY_AND_ASSIGN(EmbedderEngine);
};

}  // namespace clay

#endif  // CLAY_SHELL_PLATFORM_EMBEDDER_EMBEDDER_ENGINE_H_
