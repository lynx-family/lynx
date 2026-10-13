// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/platform/headless/clay_headless_engine.h"

#include <assert.h>

#include <string>
#include <vector>

#include "base/include/fml/time/time_point.h"
#include "build/build_config.h"
#include "clay/fml/logging.h"
#include "clay/shell/common/switches.h"
#include "clay/shell/platform/embedder/embedder_engine.h"
#include "clay/shell/platform/headless/clay_headless_renderer.h"

namespace clay {

ClayHeadlessEngine::ClayHeadlessEngine(
    const char* icu_data_path,
    const ClayHeadlessRendererConfig* renderer_config,
    const ClayTaskRunnerDescription* platform_task_runner_description,
    void* user_data) {
  RunEngine(icu_data_path, renderer_config, platform_task_runner_description,
            user_data);
}

ClayHeadlessEngine::~ClayHeadlessEngine() {
  // Engine will reset rasterizer synchronously during `Shutdown`,
  // so there is no need to wait a latch here.
  fml::TaskRunner::RunNowOrPostTask(GetTaskRunners().GetRasterTaskRunner(),
                                    [&] { renderer_->CleanupGPUResources(); });

  StopEngine();
}

bool ClayHeadlessEngine::IsValid() const {
  return static_cast<bool>(!!engine_);
}

ClayHeadlessRenderer* ClayHeadlessEngine::Renderer() const {
  return renderer_->GetEngineRenderer();
}

bool ClayHeadlessEngine::RunEngine(
    const char* icu_data_path,
    const ClayHeadlessRendererConfig* renderer_config,
    const ClayTaskRunnerDescription* platform_task_runner_description,
    void* user_data) {
  user_data_ = user_data;
  // FlutterProjectArgs is expecting a full argv, so when processing it for
  // flags the first item is treated as the executable and ignored. Add a dummy
  // value so that all provided arguments are used.
  std::vector<const char*> argv = {"placeholder"};

  fml::CommandLine command_line;
  if (!argv.empty()) {
    command_line = fml::CommandLineFromArgcArgv(argv.size(), argv.data());
  }
  clay::Settings settings = clay::SettingsFromCommandLine(command_line);
  if (icu_data_path) {
    settings.icu_data_path = icu_data_path;
  }
  if (renderer_config) {
    settings.enable_software_rendering =
        renderer_config->type == kClayRendererTypeSoftware;
  }

  ClayTaskRunnerDescription platform_task_runner = {};
  platform_task_runner.struct_size = sizeof(ClayTaskRunnerDescription);
  // Configure task runners.
  if (platform_task_runner_description) {
    platform_task_runner.user_data = const_cast<ClayTaskRunnerDescription*>(
        platform_task_runner_description);
    platform_task_runner.runs_task_on_current_thread_callback =
        [](void* user_data) -> bool {
      ClayTaskRunnerDescription* desc =
          reinterpret_cast<ClayTaskRunnerDescription*>(user_data);
      return desc->runs_task_on_current_thread_callback(desc->user_data);
    };
    platform_task_runner.post_task_callback =
        [](ClayTask task, uint64_t target_time_nanos, void* user_data) -> void {
      ClayTaskRunnerDescription* desc =
          reinterpret_cast<ClayTaskRunnerDescription*>(user_data);
      ClayTask clay_task{reinterpret_cast<ClayTaskRunner>(task.runner),
                         task.task};
      desc->post_task_callback(clay_task, target_time_nanos, desc->user_data);
    };
  } else {
    // Create an event loop for the Headless. It is not running yet.
    event_loop_ = std::make_unique<clay::ClayEventLoop>(
        std::this_thread::get_id(),  // main thread
        [this](const auto* task) {
          if (!engine_->RunTask(task)) {
            FML_LOG(ERROR) << "Could not post an engine task.";
          }
        });

    platform_task_runner.user_data = this;
    platform_task_runner.runs_task_on_current_thread_callback =
        [](void* user_data) -> bool {
      return reinterpret_cast<ClayHeadlessEngine*>(user_data)
          ->event_loop_->RunsTasksOnCurrentThread();
    };
    platform_task_runner.post_task_callback =
        [](ClayTask task, uint64_t target_time_nanos, void* user_data) -> void {
      reinterpret_cast<ClayHeadlessEngine*>(user_data)->event_loop_->PostTask(
          task, target_time_nanos);
    };
  }
  ClayCustomTaskRunners custom_task_runners = {};
  custom_task_runners.struct_size = sizeof(ClayCustomTaskRunners);
  custom_task_runners.platform_task_runner = &platform_task_runner;

  clay::PlatformViewEmbedder::PlatformDispatchTable platform_dispatch_table =
      {};
  platform_dispatch_table.clipboard.set_clipboard_data_callback =
      [this](const std::u16string& data) {
        std::string u8string = lynx::base::U16StringToU8(data);
        SetClipboardData(u8string.c_str());
      };
  platform_dispatch_table.clipboard.get_clipboard_data_callback = [this]() {
    const char* clipboard_data = GetClipboardData();
    return lynx::base::U8StringToU16(clipboard_data ? clipboard_data : "");
  };

  platform_dispatch_table.textinput.set_text_input_client_callback =
      [this](int client_id, const char* input_action, const char* input_type) {
        SetTextInputClient(client_id, input_action, input_type);
      };
  platform_dispatch_table.textinput.clear_text_input_client_callback =
      [this]() { ClearTextInputClient(); };
#if defined(ENABLE_GTK_HOST)
  platform_dispatch_table.textinput.set_editing_state_callback =
      [this](uint64_t selection_base, uint64_t composing_extent,
             const std::string& affinity, const std::string& text,
             bool directional, uint64_t selection_extent,
             uint64_t composing_base) {
        if (gtk_text_input_.set_state)
          gtk_text_input_.set_state(selection_base, composing_extent, affinity,
                                    text, directional, selection_extent,
                                    composing_base);
      };
#endif
  platform_dispatch_table.textinput.set_editable_transform_callback =
      [this](const float transform[16]) { SetEditableTransform(transform); };
  platform_dispatch_table.textinput.set_marked_text_rect_callback =
      [this](float x, float y, float width, float height) {
        SetMarkedTextRect(x, y, width, height);
      };
  platform_dispatch_table.textinput.update_caret_position_callback =
      [this](float x, float y, float width, float height) {
        UpdateCaretPosition(x, y, width, height);
      };
  platform_dispatch_table.textinput.show_text_input_callback = [this]() {
    ShowTextInput();
  };
  platform_dispatch_table.textinput.hide_text_input_callback = [this]() {
    HideTextInput();
  };
  platform_dispatch_table.textinput.cursor_position_callback =
      [this](int position) { SetCursorPosition(position); };
  platform_dispatch_table.activate_system_cursor_callback =
      [this](int type, const std::string& path) {
        ActivateSystemCursor(type, path.c_str());
      };

  renderer_ = ClayHeadlessRenderer::Create(this, *renderer_config);
  if (!renderer_) {
    return false;
  }
#ifdef SHELL_ENABLE_GL
  auto* gl_renderer_delegate = renderer_->GetGLRendererDelegate();
  if (gl_renderer_delegate) {
    engine_ = EmbedderEngine::CreateEngine(settings, &custom_task_runners,
                                           gl_renderer_delegate,
                                           platform_dispatch_table, this, this);
  }
#endif
#ifdef SHELL_ENABLE_METAL
  if (!engine_) {
    auto* metal_renderer_delegate = renderer_->GetMetalRendererDelegate();
    if (metal_renderer_delegate) {
      engine_ = EmbedderEngine::CreateEngine(
          settings, &custom_task_runners,
          fml::MakeRefCounted<EmbedderSurfaceMetal>(metal_renderer_delegate),
          platform_dispatch_table, this, this);
    }
  }
#endif
  if (!engine_) {
    auto* software_renderer_delegate = renderer_->GetSoftwareRendererDelegate();
    if (software_renderer_delegate) {
      engine_ = EmbedderEngine::CreateEngine(
          settings, &custom_task_runners, software_renderer_delegate,
          platform_dispatch_table, this, this);
    }
  }
  if (engine_ != nullptr) {
    FML_LOG(INFO) << "HeadlessEngineRun Success!";
  } else {
    FML_LOG(ERROR) << "HeadlessEngineRun Failure!";
    return false;
  }

  // Step 1: Launch the shell.
  if (!engine_->LaunchShell()) {
    FML_LOG(ERROR) << "Could not launch the engine using supplied "
                      "initialization arguments.";
    return false;
  }

  // Step 2: Tell the platform view to initialize itself.
  if (!engine_->NotifyCreated()) {
    FML_LOG(ERROR) << "Could not create platform view components.";
    return false;
  }
  service_manager_ = engine_->GetServiceManager();
  if (!service_manager_) {
    FML_LOG(ERROR) << "Failed to get clay service manager";
    return false;
  }

  return true;
}

const char* ClayHeadlessEngine::GetClipboardData() const {
  if (!delegate_) {
    return "";
  }
  return delegate_->GetClipboardData();
}

void ClayHeadlessEngine::SetClipboardData(const char* data) {
  if (!delegate_) {
    return;
  }
  delegate_->SetClipboardData(data);
}

void ClayHeadlessEngine::ActivateSystemCursor(int type, const char* path) {
  if (!delegate_) {
    return;
  }
  delegate_->ActivateSystemCursor(type, path);
}

void ClayHeadlessEngine::UpdateCaretPosition(float x, float y, float width,
                                             float height) {
  if (!delegate_) {
    return;
  }
  delegate_->UpdateCaretPosition(x, y, width, height);
}

void ClayHeadlessEngine::ShowTextInput() {
  if (!delegate_) {
    return;
  }
  delegate_->ShowTextInput();
}

void ClayHeadlessEngine::HideTextInput() {
  if (!delegate_) {
    return;
  }
  delegate_->HideTextInput();
}

void ClayHeadlessEngine::SetCursorPosition(int position) {
  if (!delegate_) {
    return;
  }
  delegate_->SetCursorPosition(position);
}

void ClayHeadlessEngine::SetMarkedTextRect(float x, float y, float width,
                                           float height) {
  if (!delegate_) {
    return;
  }
  delegate_->SetMarkedTextRect(x, y, width, height);
}

void ClayHeadlessEngine::SetEditableTransform(
    const float transform_matrix[16]) {
  if (!delegate_) {
    return;
  }
  delegate_->SetEditableTransform(transform_matrix);
}

void ClayHeadlessEngine::SetTextInputClient(int client_id,
                                            const char* input_action,
                                            const char* input_type) {
  client_id_ = client_id;
#if defined(ENABLE_GTK_HOST)
  if (gtk_text_input_.set_client)
    gtk_text_input_.set_client(client_id, input_action, input_type);
#endif
}

void ClayHeadlessEngine::ClearTextInputClient() {
  client_id_ = -1;
#if defined(ENABLE_GTK_HOST)
  if (gtk_text_input_.clear_client) gtk_text_input_.clear_client();
#endif
}

#if defined(ENABLE_GTK_HOST)
bool ClayHeadlessEngine::UpdateGtkEditingState(
    int client, const std::string& text, int selection_base,
    int selection_extent, int composing_base, int composing_extent) {
  if (!IsValid() || client < 0 || client != client_id_) return false;
  engine_->GetShell().GetEngine()->GetPageView()->OnPlatformUpdateEditState(
      client, selection_base, composing_extent, "TextAffinity.downstream",
      text.c_str(), selection_extent, composing_base);
  return true;
}
#endif

bool ClayHeadlessEngine::StopEngine() {
  if (IsValid()) {
    engine_->NotifyDestroyed();
    engine_->CollectShell();
    // Drop the wrapper reference while the engine still owns live runners.
    service_manager_.reset();
    engine_ = nullptr;
    return true;
  }
  return false;
}

#if defined(ENABLE_GTK_HOST) && defined(ENABLE_SKITY)
bool ClayHeadlessEngine::SuspendGLSurface() {
#ifdef SHELL_ENABLE_GL
  if (!IsValid() || !renderer_ || !renderer_->GetGLRendererDelegate() ||
      !engine_->NotifyDestroyed())
    return false;
  auto output = engine_->GetShell().GetPlatformView()->GetOutputSurface();
  if (!output) return false;
  bool success = false;
  fml::AutoResetWaitableEvent latch;
  fml::TaskRunner::RunNowOrPostTask(
      GetTaskRunners().GetRasterTaskRunner(), [&] {
        success =
            static_cast<EmbedderSurfaceGL*>(output.get())->ResetMainContext();
        latch.Signal();
      });
  latch.Wait();
  return success;
#else
  return false;
#endif
}
bool ClayHeadlessEngine::ResumeGLSurface() {
#ifdef SHELL_ENABLE_GL
  return IsValid() && engine_->NotifyCreated();
#else
  return false;
#endif
}
#endif

void ClayHeadlessEngine::EnableDefaultFocusRing() {
  if (!IsValid()) {
    return;
  }
  engine_->GetShell().GetEngine()->SetDefaultFocusRingEnabled(true);
}

void ClayHeadlessEngine::EnablePerformanceOverlay() {
  if (!IsValid()) {
    return;
  }
  engine_->GetShell().GetEngine()->SetPerformanceOverlayEnabled(true);
}

void ClayHeadlessEngine::OnEnterForeground() {
  if (!IsValid()) {
    return;
  }
  engine_->GetShell().GetEngine()->OnEnterForeground();
}

void ClayHeadlessEngine::OnEnterBackground() {
  if (!IsValid()) {
    return;
  }
  engine_->GetShell().GetEngine()->OnEnterBackground();
}

void ClayHeadlessEngine::NotifyLowMemoryWarning() {
  if (!IsValid()) {
    return;
  }
  engine_->GetShell().NotifyLowMemoryWarning();
}

void ClayHeadlessEngine::NotifyLocaleChange() {
  if (!IsValid()) {
    return;
  }
  // todo.
}

void ClayHeadlessEngine::RequestPaint() {
  if (!IsValid()) {
    return;
  }
  engine_->GetShell().GetEngine()->RequestPaint();
}

void ClayHeadlessEngine::SetVisible(bool enable) {
  if (!IsValid()) {
    return;
  }
  engine_->GetShell().GetEngine()->SetVisible(enable);
}

void ClayHeadlessEngine::SetFontFaceCache(const char* font_family,
                                          const char* local_path) {
  if (!IsValid()) {
    return;
  }
  engine_->GetShell().GetEngine()->SetFontFaceCache(font_family, local_path);
}

void ClayHeadlessEngine::SendViewportMetrics(int32_t width, int32_t height,
                                             double pixel_ratio) {
  if (IsValid()) {
    clay::ViewportMetrics metrics;
    metrics.physical_width = width;
    metrics.physical_height = height;
    metrics.device_pixel_ratio = pixel_ratio;
    // Default logical pixel is 96
    metrics.device_density_dpi = pixel_ratio * 96.f;
    metrics.physical_view_inset_top = 0.0;
    metrics.physical_view_inset_right = 0.0;
    metrics.physical_view_inset_bottom = 0.0;
    metrics.physical_view_inset_left = 0.0;
    engine_->SetViewportMetrics(metrics);
  }
}

void ClayHeadlessEngine::SendPointerEvents(const ClayPointerEvent* events,
                                           size_t events_count) {
  if (IsValid()) {
    engine_->SendPointerEvent(events, events_count);
  }
}

void ClayHeadlessEngine::SendKeyEvent(const ClayKeyEvent* event,
                                      ClayKeyEventCallback callback,
                                      void* user_data) {
  if (IsValid()) {
    bool enter_key_up = event->type == kClayKeyEventTypeUp &&
                        event->physical == keycodes::kPhysicalEnter;
    uintptr_t params = reinterpret_cast<uintptr_t>(this) | (enter_key_up & 1);
    engine_->SendKeyEvent(
        event,
        [](bool handled, void* user_data) {
          auto w = reinterpret_cast<uintptr_t>(user_data);
          bool enter_key_up = w & 1;
          if (!handled && enter_key_up) {
            auto* self = reinterpret_cast<ClayHeadlessEngine*>(w & ~1);
            if (self->client_id_ != -1) {
              self->engine_->GetShell()
                  .GetEngine()
                  ->GetPageView()
                  ->OnPlatformPerformInputAction(self->client_id_);
            }
          }
        },
        reinterpret_cast<void*>(params));
  }
}

void ClayHeadlessEngine::PostPlatformThreadTask(ClayVoidCallback callback,
                                                void* callback_data) {
  if (IsValid()) {
    engine_->PostPlatformThreadTask(
        [callback, callback_data]() { callback(callback_data); });
  }
}

void* ClayHeadlessEngine::GetViewContext() {
  void* clay_view_context = nullptr;
  if (IsValid()) {
    clay_view_context = engine_->GetShell().GetEngine()->GetViewContext();
  }
  return clay_view_context;
}

void ClayHeadlessEngine::RunEventLoopWithTimeout(size_t timeout) {
  if (!event_loop_) {
    return;
  }
  uint32_t timeout_milliseconds = static_cast<uint32_t>(timeout);
  std::chrono::nanoseconds wait_duration =
      timeout_milliseconds == 0
          ? std::chrono::nanoseconds::max()
          : std::chrono::milliseconds(timeout_milliseconds);
  event_loop_->WaitForEvents(wait_duration);
}

bool ClayHeadlessEngine::RunTask(const ClayTask* task) {
  if (IsValid()) {
    return engine_->RunTask(task);
  }
  return false;
}

const TaskRunners& ClayHeadlessEngine::GetTaskRunners() const {
  FML_DCHECK(IsValid());
  return engine_->GetTaskRunners();
}

uint64_t ClayHeadlessEngine::GetEngineCurrentTime() {
  return fml::TimePoint::Now().ToEpochDelta().ToNanoseconds();
}

}  // namespace clay
