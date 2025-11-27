// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/platform/embedder/embedder_engine.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/include/fml/make_copyable.h"
#include "clay/shell/platform/embedder/embedder_struct_macros.h"
#include "clay/shell/platform/embedder/vsync_waiter_embedder.h"
#include "clay/ui/window/key_data_helper.h"
#include "clay/ui/window/pointer_data_helper.h"

namespace clay {

namespace {

std::unique_ptr<EmbedderThreadHost> CreateThreadHost(
    const ClayCustomTaskRunners* custom_task_runners) {
  void (*thread_priority_setter)(fml::Thread::ThreadPriority);
  thread_priority_setter = custom_task_runners
                               ? custom_task_runners->thread_priority_setter
                               : nullptr;
  auto thread_config_callback =
      [thread_priority_setter](const fml::Thread::ThreadConfig& config) {
        fml::Thread::SetCurrentThreadName(config);
        if (!thread_priority_setter) {
          return;
        }
        thread_priority_setter(config.priority);
      };
  auto thread_host =
      clay::EmbedderThreadHost::CreateEmbedderOrEngineManagedThreadHost(
          custom_task_runners, thread_config_callback);

  if (!thread_host || !thread_host->IsValid()) {
    FML_LOG(ERROR) << "Could not set up or infer thread configuration to run "
                      "the Flutter engine on.";
    return nullptr;
  }
  auto task_runners = thread_host->GetTaskRunners();

  if (!task_runners.IsValid()) {
    FML_LOG(ERROR) << "Task runner configuration was invalid.";
    return nullptr;
  }
  return thread_host;
}

static std::unique_ptr<clay::Rasterizer> CreateRasterizer(
    std::shared_ptr<clay::ServiceManager> service_manager) {
  return std::make_unique<clay::Rasterizer>(service_manager);
};
}  // namespace

struct ShellArgs {
  Settings settings;
  Shell::CreateCallback<PlatformView> on_create_platform_view;
  Shell::CreateCallbackFnPtr<Rasterizer> on_create_rasterizer;
  ShellArgs(const Settings& p_settings,
            Shell::CreateCallback<PlatformView> p_on_create_platform_view,
            Shell::CreateCallbackFnPtr<Rasterizer> p_on_create_rasterizer)
      : settings(p_settings),
        on_create_platform_view(std::move(p_on_create_platform_view)),
        on_create_rasterizer(std::move(p_on_create_rasterizer)) {}
};

#ifdef SHELL_ENABLE_METAL
std::unique_ptr<EmbedderEngine> EmbedderEngine::CreateEngine(
    Settings& settings, const ClayCustomTaskRunners* custom_task_runners,
    fml::RefPtr<clay::EmbedderSurfaceMetal> embedder_surface,
    const PlatformViewEmbedder::PlatformDispatchTable& platform_dispatch_table,
    PlatformViewEmbedderDelegate* platform_view_delegate, void* user_data) {
  auto thread_host = CreateThreadHost(custom_task_runners);
  if (!thread_host) {
    return nullptr;
  }
  auto task_runners = thread_host->GetTaskRunners();

  std::shared_ptr<clay::ServiceManager> clay_service_manager =
      clay::ServiceManager::Create(
          {task_runners.GetPlatformTaskRunner(), task_runners.GetUITaskRunner(),
           task_runners.GetRasterTaskRunner(), task_runners.GetIOTaskRunner()});

  auto on_create_platform_view = fml::MakeCopyable(
      [=, embedder_surface = embedder_surface](
          std::shared_ptr<clay::ServiceManager> service_manager,
          clay::Shell& shell) mutable {
        return std::make_unique<clay::PlatformViewEmbedder>(
            service_manager,              // service manager
            shell,                        // delegate
            shell.GetTaskRunners(),       // task runners
            std::move(embedder_surface),  // embedder surface
            platform_dispatch_table,      // platform dispatch table
            user_data,                    // engine
            platform_view_delegate        // PlatformViewEmbedderDelegate
        );                                // NOLINT(whitespace/parens)
      });

  clay::Shell::CreateCallbackFnPtr<clay::Rasterizer> on_create_rasterizer =
      &CreateRasterizer;

  // Create the engine but don't launch the shell or run the root isolate.
  auto embedder_engine =
      std::make_unique<clay::EmbedderEngine>(clay_service_manager,     //
                                             std::move(thread_host),   //
                                             std::move(task_runners),  //
                                             std::move(settings),      //
                                             on_create_platform_view,  //
                                             on_create_rasterizer      //
      );  // NOLINT(whitespace/parens)
  return embedder_engine;
}
#endif

#ifdef SHELL_ENABLE_GL
std::unique_ptr<EmbedderEngine> EmbedderEngine::CreateEngine(
    Settings& settings, const ClayCustomTaskRunners* custom_task_runners,
    GPUSurfaceGLDelegate* gl_delegate,
    const PlatformViewEmbedder::PlatformDispatchTable& platform_dispatch_table,
    PlatformViewEmbedderDelegate* platform_view_delegate, void* user_data) {
  auto thread_host = CreateThreadHost(custom_task_runners);
  if (!thread_host) {
    return nullptr;
  }
  auto task_runners = thread_host->GetTaskRunners();

  std::shared_ptr<clay::ServiceManager> clay_service_manager =
      clay::ServiceManager::Create(
          {task_runners.GetPlatformTaskRunner(), task_runners.GetUITaskRunner(),
           task_runners.GetRasterTaskRunner(), task_runners.GetIOTaskRunner()});

  auto on_create_platform_view = fml::MakeCopyable(
      [=, gl_delegate = gl_delegate](
          std::shared_ptr<clay::ServiceManager> service_manager,
          clay::Shell& shell) mutable {
        return std::make_unique<clay::PlatformViewEmbedder>(
            service_manager,          // service manager
            shell,                    // delegate
            shell.GetTaskRunners(),   // task runners
            gl_delegate,              // GPUSurfaceGLDelegate
            platform_dispatch_table,  // platform dispatch table
            user_data,                // engine
            platform_view_delegate    // PlatformViewEmbedderDelegate
        );                            // NOLINT(whitespace/parens)
      });

  clay::Shell::CreateCallbackFnPtr<clay::Rasterizer> on_create_rasterizer =
      &CreateRasterizer;

  // Create the engine but don't launch the shell or run the root isolate.
  auto embedder_engine =
      std::make_unique<clay::EmbedderEngine>(clay_service_manager,     //
                                             std::move(thread_host),   //
                                             std::move(task_runners),  //
                                             std::move(settings),      //
                                             on_create_platform_view,  //
                                             on_create_rasterizer      //
      );  // NOLINT(whitespace/parens)
  return embedder_engine;
}
#endif

std::unique_ptr<EmbedderEngine> EmbedderEngine::CreateEngine(
    Settings& settings, const ClayCustomTaskRunners* custom_task_runners,
    EmbedderSurfaceSoftwareDelegate* software_delegate,
    const clay::PlatformViewEmbedder::PlatformDispatchTable&
        platform_dispatch_table,
    PlatformViewEmbedderDelegate* platform_view_delegate, void* user_data) {
  auto thread_host = CreateThreadHost(custom_task_runners);
  if (!thread_host) {
    return nullptr;
  }
  auto task_runners = thread_host->GetTaskRunners();

  std::shared_ptr<clay::ServiceManager> clay_service_manager =
      clay::ServiceManager::Create(
          {task_runners.GetPlatformTaskRunner(), task_runners.GetUITaskRunner(),
           task_runners.GetRasterTaskRunner(), task_runners.GetIOTaskRunner()});

  auto on_create_platform_view = fml::MakeCopyable(
      [=, software_delegate = software_delegate](
          std::shared_ptr<clay::ServiceManager> service_manager,
          clay::Shell& shell) mutable {
        return std::make_unique<clay::PlatformViewEmbedder>(
            service_manager,          // service manager
            shell,                    // delegate
            shell.GetTaskRunners(),   // task runners
            software_delegate,        // EmbedderSurfaceSoftwareDelegate
            platform_dispatch_table,  // platform dispatch table
            user_data,                // engine
            platform_view_delegate    // PlatformViewEmbedderDelegate
        );                            // NOLINT(whitespace/parens)
      });

  clay::Shell::CreateCallbackFnPtr<clay::Rasterizer> on_create_rasterizer =
      &CreateRasterizer;

  // Create the engine but don't launch the shell or run the root isolate.
  auto embedder_engine =
      std::make_unique<clay::EmbedderEngine>(clay_service_manager,     //
                                             std::move(thread_host),   //
                                             std::move(task_runners),  //
                                             std::move(settings),      //
                                             on_create_platform_view,  //
                                             on_create_rasterizer      //
      );  // NOLINT(whitespace/parens)
  return embedder_engine;
}

EmbedderEngine::EmbedderEngine(
    std::shared_ptr<clay::ServiceManager> service_manager,
    std::unique_ptr<EmbedderThreadHost> thread_host,
    const clay::TaskRunners& task_runners, const clay::Settings& settings,
    const Shell::CreateCallback<PlatformView>& on_create_platform_view,
    const Shell::CreateCallbackFnPtr<Rasterizer>& on_create_rasterizer)
    : service_manager_(service_manager),
      thread_host_(std::move(thread_host)),
      task_runners_(task_runners),
      shell_args_(std::make_unique<ShellArgs>(settings, on_create_platform_view,
                                              on_create_rasterizer)) {}

EmbedderEngine::~EmbedderEngine() {}

bool EmbedderEngine::LaunchShell() {
  if (!shell_args_) {
    FML_DLOG(ERROR) << "Invalid shell arguments.";
    return false;
  }

  if (shell_) {
    FML_DLOG(ERROR) << "Shell already initialized";
  }

  shell_ = Shell::Create(service_manager_, task_runners_, shell_args_->settings,
                         shell_args_->on_create_platform_view,
                         shell_args_->on_create_rasterizer);

  // Reset the args no matter what. They will never be used to initialize a
  // shell again.
  shell_args_.reset();

  return IsValid();
}

bool EmbedderEngine::CollectShell() {
  shell_.reset();
  return IsValid();
}

bool EmbedderEngine::IsValid() const { return static_cast<bool>(shell_); }

const TaskRunners& EmbedderEngine::GetTaskRunners() const {
  return task_runners_;
}

bool EmbedderEngine::NotifyCreated() {
  if (!IsValid()) {
    return false;
  }

  shell_->GetPlatformView()->NotifyCreated();
  return true;
}

bool EmbedderEngine::NotifyDestroyed() {
  if (!IsValid()) {
    return false;
  }

  shell_->GetPlatformView()->NotifyDestroyed();
  return true;
}

bool EmbedderEngine::SetViewportMetrics(const clay::ViewportMetrics& metrics) {
  if (!IsValid()) {
    return false;
  }

  auto platform_view = shell_->GetPlatformView();
  if (!platform_view) {
    return false;
  }
  platform_view->SetViewportMetrics(metrics);
  return true;
}

bool EmbedderEngine::SendPointerEvent(const ClayPointerEvent* pointers,
                                      size_t events_count) {
  if (pointers == nullptr || events_count == 0) {
    return false;
  }

  auto packet = std::make_unique<clay::PointerDataPacket>(events_count);

  const ClayPointerEvent* current = pointers;

  for (size_t i = 0; i < events_count; ++i) {
    clay::PointerData pointer_data;
    pointer_data.Clear();
    // this is current in use only on android embedding.
    pointer_data.embedder_id = 0;
    pointer_data.time_stamp = SAFE_ACCESS(current, timestamp, 0);
    pointer_data.change = PointerDataHelper::ToPointerDataChange(
        SAFE_ACCESS(current, phase, ClayPointerPhase::kClayPointerPhaseCancel));
    pointer_data.physical_x = SAFE_ACCESS(current, x, 0.0);
    pointer_data.physical_y = SAFE_ACCESS(current, y, 0.0);
    // Delta will be generated in pointer_data_packet_converter.cc.
    pointer_data.physical_delta_x = 0.0;
    pointer_data.physical_delta_y = 0.0;
    pointer_data.device = SAFE_ACCESS(current, device, 0);
    // Pointer identifier will be generated in
    // pointer_data_packet_converter.cc.
    pointer_data.pointer_identifier = 0;
    pointer_data.signal_kind = PointerDataHelper::ToPointerDataSignalKind(
        SAFE_ACCESS(current, signal_kind, kClayPointerSignalKindNone));
    pointer_data.scroll_delta_x = SAFE_ACCESS(current, scroll_delta_x, 0.0);
    pointer_data.scroll_delta_y = SAFE_ACCESS(current, scroll_delta_y, 0.0);
    ClayPointerDeviceKind device_kind = SAFE_ACCESS(current, device_kind, 0);
    // For backwards compatibility with embedders written before the device
    // kind and buttons were exposed, if the device kind is not set treat it
    // as a mouse, with a synthesized primary button state based on the phase.
    if (device_kind == 0) {
      pointer_data.kind = clay::PointerData::DeviceKind::kMouse;
      pointer_data.buttons =
          PointerDataHelper::PointerDataButtonsForLegacyEvent(
              pointer_data.change);

    } else {
      pointer_data.kind = PointerDataHelper::ToPointerDataKind(device_kind);
      if (pointer_data.kind == clay::PointerData::DeviceKind::kTouch) {
        // For touch events, set the button internally rather than requiring
        // it at the API level, since it's a confusing construction to expose.
        if (pointer_data.change == clay::PointerData::Change::kDown ||
            pointer_data.change == clay::PointerData::Change::kMove) {
          pointer_data.buttons = clay::kPointerButtonTouchContact;
        }
      } else {
        // Buttons use the same mask values, so pass them through directly.
        pointer_data.buttons = SAFE_ACCESS(current, buttons, 0);
      }
    }
    pointer_data.pan_x = SAFE_ACCESS(current, pan_x, 0.0);
    pointer_data.pan_y = SAFE_ACCESS(current, pan_y, 0.0);
    // Delta will be generated in pointer_data_packet_converter.cc.
    pointer_data.pan_delta_x = 0.0;
    pointer_data.pan_delta_y = 0.0;
    pointer_data.scale = SAFE_ACCESS(current, scale, 0.0);
    pointer_data.rotation = SAFE_ACCESS(current, rotation, 0.0);
    pointer_data.is_precise_scroll =
        SAFE_ACCESS(current, is_precise_scroll, true);
    packet->SetPointerData(i, pointer_data);
    current = reinterpret_cast<const ClayPointerEvent*>(
        reinterpret_cast<const uint8_t*>(current) + current->struct_size);
  }
  return DispatchPointerDataPacket(std::move(packet));
}

bool EmbedderEngine::SendKeyEvent(const ClayKeyEvent* event,
                                  ClayKeyEventCallback callback,
                                  void* user_data) {
  if (event == nullptr) {
    return false;
  }

  const char* character = SAFE_ACCESS(event, character, nullptr);

  clay::KeyData key_data;
  key_data.Clear();
  key_data.timestamp = static_cast<uint64_t>(SAFE_ACCESS(event, timestamp, 0));
  key_data.type = KeyDataHelper::MapKeyEventType(
      SAFE_ACCESS(event, type, ClayKeyEventType::kClayKeyEventTypeUp));
  key_data.physical = SAFE_ACCESS(event, physical, 0);
  key_data.logical = SAFE_ACCESS(event, logical, 0);
  key_data.synthesized = SAFE_ACCESS(event, synthesized, false);

  auto packet = std::make_unique<clay::KeyDataPacket>(key_data, character);
  auto response = [callback, user_data](bool handled) {
    if (callback != nullptr) {
      callback(handled, user_data);
    }
  };

  // Clay direct dispatch a key data packet
  return DispatchKeyDataPacket(std::move(packet), response);
}

bool EmbedderEngine::DispatchPointerDataPacket(
    std::unique_ptr<clay::PointerDataPacket> packet) {
  if (!IsValid() || !packet) {
    return false;
  }

  auto platform_view = shell_->GetPlatformView();
  if (!platform_view) {
    return false;
  }

  platform_view->DispatchPointerDataPacket(std::move(packet));
  return true;
}

bool EmbedderEngine::DispatchKeyDataPacket(
    std::unique_ptr<clay::KeyDataPacket> packet,
    PlatformView::Delegate::KeyDataResponse callback) {
  if (!IsValid() || !packet) {
    return false;
  }

  auto platform_view = shell_->GetPlatformView();
  if (!platform_view) {
    return false;
  }

  platform_view->DispatchKeyDataPacket(std::move(packet), std::move(callback));
  return true;
}

bool EmbedderEngine::OnVsyncEvent(intptr_t baton,
                                  fml::TimePoint frame_start_time,
                                  fml::TimePoint frame_target_time) {
  if (!IsValid()) {
    return false;
  }

  return VsyncWaiterEmbedder::OnEmbedderVsync(
      task_runners_, baton, frame_start_time, frame_target_time);
}

bool EmbedderEngine::ReloadSystemFonts() {
  if (!IsValid()) {
    return false;
  }

  return shell_->ReloadSystemFonts();
}

bool EmbedderEngine::PostRenderThreadTask(lynx::base::closure task) {
  if (!IsValid()) {
    return false;
  }

  shell_->GetTaskRunners().GetRasterTaskRunner()->PostTask(std::move(task));
  return true;
}

bool EmbedderEngine::PostPlatformThreadTask(lynx::base::closure task) {
  if (!IsValid()) {
    return false;
  }

  shell_->GetTaskRunners().GetPlatformTaskRunner()->PostTask(std::move(task));
  return true;
}

bool EmbedderEngine::PostUIThreadTask(lynx::base::closure task) {
  if (!IsValid()) {
    return false;
  }
  shell_->GetTaskRunners().GetUITaskRunner()->PostTask(std::move(task));
  return true;
}

bool EmbedderEngine::RunTask(const ClayTask* task) {
  // The shell doesn't need to be running or valid for access to the thread
  // host. This is why there is no `IsValid` check here. This allows embedders
  // to perform custom task runner interop before the shell is running.
  if (task == nullptr) {
    return false;
  }
  return thread_host_->PostTask(reinterpret_cast<int64_t>(task->runner),
                                task->task);
}

bool EmbedderEngine::ScheduleFrame() {
  if (!IsValid()) {
    return false;
  }

  auto platform_view = shell_->GetPlatformView();
  if (!platform_view) {
    return false;
  }
  platform_view->ScheduleFrame();
  return true;
}

Shell& EmbedderEngine::GetShell() {
  FML_DCHECK(shell_);
  return *shell_.get();
}

const std::shared_ptr<clay::ServiceManager>& EmbedderEngine::GetServiceManager()
    const {
  FML_DCHECK(service_manager_);
  return service_manager_;
}

}  // namespace clay
