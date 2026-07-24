// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/platform/windows/flutter_windows_engine.h"

#include <dwmapi.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <list>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>

#include "clay/fml/logging.h"
#include "clay/fml/paths.h"
#include "clay/fml/platform/win/wstring_conversion.h"
#include "clay/shell/common/services/drag_drop_service.h"
#include "clay/shell/common/switches.h"
#include "clay/shell/platform/common/path_utils.h"
#include "clay/shell/platform/windows/flutter_windows_view.h"
#include "clay/shell/platform/windows/overlay_view_manager_service.h"
#include "clay/shell/platform/windows/overlay_windows_view.h"
#include "clay/shell/platform/windows/task_runner.h"
#include "clay/shell/platform/windows/text_input_plugin.h"
#include "clay/ui/common/isolate.h"

// winbase.h defines GetCurrentTime as a macro.
#undef GetCurrentTime

namespace clay {

namespace {

// Lifted from vsync_waiter_fallback.cc
static int64_t SnapToNextTick(int64_t value, int64_t tick_phase,
                              int64_t tick_interval) {
  int64_t offset = (tick_phase - value) % tick_interval;
  if (offset != 0) offset = offset + tick_interval;
  return value + offset;
}

}  // namespace

FlutterWindowsEngine::FlutterWindowsEngine(const FlutterProjectBundle& project)
    : PlatformViewEmbedderDelegate(),
      project_(std::make_unique<FlutterProjectBundle>(project)) {
  task_runner_ =
      std::make_unique<TaskRunner>(
          []() -> uint64_t {
            return fml::TimePoint::Now().ToEpochDelta().ToNanoseconds();
          },
          [this](const auto* task) {
            if (!engine_) {
              FML_LOG(ERROR)
                  << "Cannot post an engine task when engine is not running.";
              return;
            }
            if (!engine_->RunTask(task)) {
              FML_LOG(ERROR) << "Failed to post an engine task.";
            }
          });

  if (!project_->use_software_rendering()) {
    egl_manager_ = egl::Manager::Create();
  }
  window_proc_delegate_manager_ = std::make_unique<WindowProcDelegateManager>();
  window_proc_delegate_manager_->RegisterTopLevelWindowProcDelegate(
      [](HWND hwnd, UINT msg, WPARAM wpar, LPARAM lpar, void* user_data,
         LRESULT* result) -> bool {
        FML_DCHECK(user_data);
        FlutterWindowsEngine* that =
            static_cast<FlutterWindowsEngine*>(user_data);
        if (msg == WM_DWMCOMPOSITIONCHANGED) {
          // DWM composition can be disabled on Windows 7.
          // Notify the engine as this can result in screen tearing.
          that->OnDwmCompositionChanged();
          return true;
        }
        return false;
      },
      static_cast<void*>(this));

  cursor_handler_ = std::make_unique<CursorHandler>(this);
  platform_handler_ = std::make_unique<PlatformHandler>(this);
}

FlutterWindowsEngine::~FlutterWindowsEngine() { Stop(); }

void FlutterWindowsEngine::SetSwitches(
    const std::vector<std::string>& switches) {
  project_->SetSwitches(switches);
}

bool FlutterWindowsEngine::Run() { return Run(""); }

bool FlutterWindowsEngine::Run(std::string_view entrypoint) {
  if (!project_->HasValidPaths()) {
    FML_LOG(ERROR) << "Missing or unresolvable paths to assets.";
    return false;
  }
  std::string icu_path_string = project_->icu_path().u8string();

  // FlutterProjectArgs is expecting a full argv, so when processing it for
  // flags the first item is treated as the executable and ignored. Add a dummy
  // value so that all provided arguments are used.
  std::string executable_name = GetExecutableName();
  std::vector<const char*> argv = {executable_name.c_str()};
  std::vector<std::string> switches = project_->GetSwitches();
  std::transform(
      switches.begin(), switches.end(), std::back_inserter(argv),
      [](const std::string& arg) -> const char* { return arg.c_str(); });

  fml::CommandLine command_line;
  if (!argv.empty()) {
    command_line = fml::CommandLineFromArgcArgv(argv.size(), argv.data());
  }
  clay::Settings settings = clay::SettingsFromCommandLine(command_line);
  settings.icu_data_path = icu_path_string;
  settings.enable_default_focus_ring = false;
  settings.enable_performance_overlay = false;

  // Configure task runners.
  ClayTaskRunnerDescription platform_task_runner = {};
  platform_task_runner.struct_size = sizeof(ClayTaskRunnerDescription);
  platform_task_runner.user_data = task_runner_.get();
  platform_task_runner.runs_task_on_current_thread_callback =
      [](void* user_data) -> bool {
    return static_cast<TaskRunner*>(user_data)->RunsTasksOnCurrentThread();
  };
  platform_task_runner.post_task_callback =
      [](ClayTask task, uint64_t target_time_nanos, void* user_data) -> void {
    static_cast<TaskRunner*>(user_data)->PostClayTask(task, target_time_nanos);
  };
  ClayCustomTaskRunners custom_task_runners = {};
  custom_task_runners.struct_size = sizeof(ClayCustomTaskRunners);
  custom_task_runners.platform_task_runner = &platform_task_runner;
  custom_task_runners.thread_priority_setter =
      &WindowsPlatformThreadPrioritySetter;

  clay::PlatformViewEmbedder::PlatformDispatchTable platform_dispatch_table =
      {};
  platform_dispatch_table.on_pre_engine_restart_callback = [this]() {
    OnPreEngineRestart();
  };

  platform_dispatch_table.clipboard.set_clipboard_data_callback =
      [this](const std::u16string& data) {
        std::string u8string = lynx::base::U16StringToU8(data);
        platform_handler_->SetPlainText(u8string);
      };
  platform_dispatch_table.clipboard.get_clipboard_data_callback = [this]() {
    return lynx::base::U8StringToU16(platform_handler_->GetPlainText());
  };
  platform_dispatch_table.textinput.set_text_input_client_callback =
      [this](int client_id, const char* input_action, const char* input_type) {
        if (text_input_plugin_) {
          text_input_plugin_->SetTextInputClient(client_id, input_action,
                                                 input_type);
        }
      };
  platform_dispatch_table.textinput.clear_text_input_client_callback =
      [this]() {
        if (text_input_plugin_) {
          text_input_plugin_->ClearTextInputClient();
        }
      };
  platform_dispatch_table.textinput.set_editable_transform_callback =
      [this](const float transform[16]) {
        if (text_input_plugin_) {
          text_input_plugin_->SetEditableTransform(transform);
        }
      };
  platform_dispatch_table.textinput.set_editing_state_callback =
      [this](uint64_t selection_base, uint64_t composing_extent,
             const std::string& selection_affinity, const std::string& text,
             bool selection_directional, uint64_t selection_extent,
             uint64_t composing_base) {
        if (text_input_plugin_) {
          text_input_plugin_->SetEditingState(
              selection_base, composing_extent, selection_affinity.c_str(),
              text.c_str(), selection_directional, selection_extent,
              composing_base);
        }
      };
  platform_dispatch_table.textinput.set_caret_rect_callback =
      [this](float x, float y, float width, float height) {
        // INFO: this method is not implemented in textInputPlugin yet.
      };
  platform_dispatch_table.textinput.set_marked_text_rect_callback =
      [this](float x, float y, float width, float height) {
        if (text_input_plugin_) {
          text_input_plugin_->SetMarkedTextRect(x, y, width, height);
        }
      };
  platform_dispatch_table.textinput.show_text_input_callback = [this]() {
    if (text_input_plugin_) {
      text_input_plugin_->ShowTextInput();
    }
  };
  platform_dispatch_table.textinput.hide_text_input_callback = [this]() {
    if (text_input_plugin_) {
      text_input_plugin_->HideTextInput();
    }
  };
  platform_dispatch_table.textinput.input_filter_callback =
      [this](const std::string& input,
             const std::string& pattern) -> std::string {
    if (text_input_plugin_) {
      return text_input_plugin_->FilterInput(input, pattern);
    } else {
      return input;
    }
  };
  platform_dispatch_table.window_move_callback = [this]() {
    if (view_) {
      view_->MoveWindow();
    }
  };
  platform_dispatch_table.activate_system_cursor_callback =
      [this](int type, const std::string& path) {
        if (cursor_handler_) {
          cursor_handler_->ActivateSystemCursor(type, path.c_str());
        }
      };

  if (egl_manager()) {
    engine_ = clay::EmbedderEngine::CreateEngine(
        settings, &custom_task_runners, (GPUSurfaceGLDelegate*)this,
        platform_dispatch_table, this, this);
  } else {
    engine_ = clay::EmbedderEngine::CreateEngine(
        settings, &custom_task_runners, (EmbedderSurfaceSoftwareDelegate*)this,
        platform_dispatch_table, this, this);
  }

  if (!engine_) {
    FML_LOG(ERROR) << "Failed to start Flutter engine";
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

  overlay_view_manager_service_ =
      std::make_shared<OverlayViewManagerService>(this);
  // |view_| can be nullptr in headless mode; avoid dereferencing it here.
  service_manager_->RegisterService<OverlayViewManagerService>(
      overlay_view_manager_service_);

  // Configure device frame rate displayed via devtools.
  std::vector<std::unique_ptr<clay::Display>> displays;
  displays.push_back(std::make_unique<clay::Display>(
      0, round(1.0 / (static_cast<double>(FrameInterval()) / 1000000000.0))));
  engine_->GetShell().OnDisplayUpdates(clay::DisplayUpdateType::kStartup,
                                       std::move(displays));

  SendSystemLocales();

  // settings_plugin_->StartWatching();
  // settings_plugin_->SendSettings();

  return true;
}

bool FlutterWindowsEngine::Stop() {
  if (engine_) {
    engine_->NotifyDestroyed();
    engine_->CollectShell();
    engine_ = nullptr;
    return true;
  }
  return false;
}

void FlutterWindowsEngine::SetView(FlutterWindowsView* view) {
  view_ = view;
  InitializeKeyboard();
}

void FlutterWindowsEngine::OnVsync(intptr_t baton) {
  int64_t current_time = fml::TimePoint::Now().ToEpochDelta().ToNanoseconds();
  int64_t frame_interval = FrameInterval();
  auto next = SnapToNextTick(current_time, start_time_, frame_interval);
  auto start_time =
      fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromNanoseconds(next));
  auto target_time = fml::TimePoint::FromEpochDelta(
      fml::TimeDelta::FromNanoseconds(next + frame_interval));
  engine_->OnVsyncEvent(baton, start_time, target_time);
}

void FlutterWindowsEngine::InitializeKeyboard() {
  if (view_ == nullptr) {
    FML_LOG(ERROR) << "Cannot initialize keyboard on Windows headless mode.";
  }

  KeyboardKeyEmbedderHandler::GetKeyStateHandler get_key_state = GetKeyState;
  KeyboardKeyEmbedderHandler::MapVirtualKeyToScanCode map_vk_to_scan =
      [](UINT virtual_key, bool extended) {
        return MapVirtualKey(virtual_key,
                             extended ? MAPVK_VK_TO_VSC_EX : MAPVK_VK_TO_VSC);
      };
  keyboard_key_handler_ =
      std::move(CreateKeyboardKeyHandler(get_key_state, map_vk_to_scan));
  text_input_plugin_ = std::move(CreateTextInputPlugin());
}

FlutterWindowsView* FlutterWindowsEngine::GetViewById(FlutterViewId view_id) {
  if (view_id == kImplicitViewId) {
    return view_;
  }
  return GetOverlayView(view_id);
}

void FlutterWindowsEngine::ClearPointerOwnersForView(FlutterViewId view_id) {
  for (auto it = pointer_owner_views_.begin();
       it != pointer_owner_views_.end();) {
    if (it->second == view_id) {
      it = pointer_owner_views_.erase(it);
    } else {
      ++it;
    }
  }
}

std::unique_ptr<KeyboardHandlerBase>
FlutterWindowsEngine::CreateKeyboardKeyHandler(
    KeyboardKeyEmbedderHandler::GetKeyStateHandler get_key_state,
    KeyboardKeyEmbedderHandler::MapVirtualKeyToScanCode map_vk_to_scan) {
  auto send_event = [this](const ClayKeyEvent& event,
                           ClayKeyEventCallback callback, void* user_data) {
    return SendKeyEvent(event, callback, user_data);
  };
  auto keyboard_key_handler = std::make_unique<KeyboardKeyHandler>();
  keyboard_key_handler->AddDelegate(
      std::make_unique<KeyboardKeyEmbedderHandler>(send_event, get_key_state,
                                                   map_vk_to_scan));
  return keyboard_key_handler;
}

std::unique_ptr<TextInputPlugin> FlutterWindowsEngine::CreateTextInputPlugin() {
  return std::make_unique<TextInputPlugin>(view_, this);
}

int64_t FlutterWindowsEngine::FrameInterval() {
  int64_t interval = 16600000;

  DWM_TIMING_INFO timing_info = {};
  timing_info.cbSize = sizeof(timing_info);
  HRESULT result = DwmGetCompositionTimingInfo(NULL, &timing_info);
  if (result == S_OK && timing_info.rateRefresh.uiDenominator > 0 &&
      timing_info.rateRefresh.uiNumerator > 0) {
    interval = static_cast<double>(timing_info.rateRefresh.uiDenominator *
                                   1000000000.0) /
               static_cast<double>(timing_info.rateRefresh.uiNumerator);
  }

  return interval;
}

void FlutterWindowsEngine::SendWindowMetricsEvent(
    const clay::ViewportMetrics& metrics) {
  if (engine_) {
    engine_->SetViewportMetrics(metrics);
  }
}

void FlutterWindowsEngine::SendPointerEvent(const ClayPointerEvent& event) {
  if (engine_) {
    engine_->SendPointerEvent(&event, 1);
  }
}

bool FlutterWindowsEngine::SendPointerEvent(FlutterWindowsView* source,
                                            const ClayPointerEvent& event) {
  if (!source || !engine_) {
    return false;
  }

  const int32_t pointer_id = event.device;
  const FlutterViewId source_view_id = source->view_id();
  auto owner_it = pointer_owner_views_.find(pointer_id);

  if (event.phase == ClayPointerPhase::kClayPointerPhaseAdd) {
    if (owner_it != pointer_owner_views_.end() &&
        owner_it->second != source_view_id) {
      if (FlutterWindowsView* owner = GetViewById(owner_it->second)) {
        owner->ReleasePointer(pointer_id);
      } else {
        pointer_owner_views_.erase(owner_it);
      }
    }
    pointer_owner_views_[pointer_id] = source_view_id;
  } else if (owner_it != pointer_owner_views_.end() &&
             owner_it->second != source_view_id) {
    return false;
  }

  SendPointerEvent(event);

  if (event.phase == ClayPointerPhase::kClayPointerPhaseRemove) {
    owner_it = pointer_owner_views_.find(pointer_id);
    if (owner_it != pointer_owner_views_.end() &&
        owner_it->second == source_view_id) {
      pointer_owner_views_.erase(owner_it);
    }
  }
  return true;
}

void FlutterWindowsEngine::SendKeyEvent(const ClayKeyEvent& event,
                                        ClayKeyEventCallback callback,
                                        void* user_data) {
  if (engine_) {
    engine_->SendKeyEvent(&event, callback, user_data);
  }
}

void FlutterWindowsEngine::ReloadSystemFonts() {
  if (engine_ == nullptr) {
    return;
  }
  engine_->ReloadSystemFonts();
}

void* FlutterWindowsEngine::GetViewContext() {
  if (engine_ == nullptr) {
    return nullptr;
  }
  return engine_->GetShell().GetEngine()->GetViewContext();
}

void FlutterWindowsEngine::ScheduleFrame() {
  if (engine_ == nullptr) {
    return;
  }
  engine_->GetShell().GetEngine()->RequestPaint();
}

void FlutterWindowsEngine::SetNextFrameCallback(fml::closure callback) {
  if (engine_ == nullptr) {
    return;
  }
  fml::WeakPtr<clay::PlatformView> weak_platform_view =
      engine_->GetShell().GetPlatformView();
  if (!weak_platform_view) {
    return;
  }
  weak_platform_view->SetNextFrameCallback(
      [this, callback = std::move(callback)]() {
        task_runner_->PostTask(std::move(callback));
      });
}

void FlutterWindowsEngine::SendSystemLocales() {}

bool FlutterWindowsEngine::PostRasterThreadTask(fml::closure callback) const {
  struct Captures {
    fml::closure callback;
  };
  auto captures = new Captures();
  captures->callback = std::move(callback);
  if (engine_) {
    auto task = [captures]() {
      captures->callback();
      delete captures;
    };
    if (engine_->PostRenderThreadTask(std::move(task))) {
      return true;
    }
  }
  delete captures;
  return false;
}

void FlutterWindowsEngine::OnPreEngineRestart() {
  // Reset the keyboard's state on hot restart.
  if (view_) {
    InitializeKeyboard();
  }
}

std::string FlutterWindowsEngine::GetExecutableName() const {
  std::pair<bool, std::string> result = fml::paths::GetExecutablePath();
  if (result.first) {
    const std::string& executable_path = result.second;
    size_t last_separator = executable_path.find_last_of("/\\");
    if (last_separator == std::string::npos ||
        last_separator == executable_path.size() - 1) {
      return executable_path;
    }
    return executable_path.substr(last_separator + 1);
  }
  return "Flutter";
}

void FlutterWindowsEngine::UpdateHighContrastEnabled(bool enabled) {
  // high_contrast_enabled_ = enabled;
  // int flags = EnabledAccessibilityFeatures();
  // if (enabled) {
  //   flags |=
  //       FlutterAccessibilityFeature::kFlutterAccessibilityFeatureHighContrast;
  // } else {
  //   flags &=
  //       ~FlutterAccessibilityFeature::kFlutterAccessibilityFeatureHighContrast;
  // }
  // UpdateAccessibilityFeatures(static_cast<FlutterAccessibilityFeature>(flags));
}

void FlutterWindowsEngine::EnableDefaultFocusRing() {
  if (engine_ == nullptr) {
    return;
  }
  engine_->GetShell().GetEngine()->SetDefaultFocusRingEnabled(true);
}
void FlutterWindowsEngine::EnablePerformanceOverlay() {
  if (engine_ == nullptr) {
    return;
  }
  engine_->GetShell().GetEngine()->SetPerformanceOverlayEnabled(true);
}

void FlutterWindowsEngine::OnEnterForeground() {
  if (engine_ == nullptr) {
    return;
  }
  engine_->GetShell().GetEngine()->OnEnterForeground();
}

void FlutterWindowsEngine::OnEnterBackground() {
  if (engine_ == nullptr) {
    return;
  }
  engine_->GetShell().GetEngine()->OnEnterBackground();
}

void FlutterWindowsEngine::RequestPaint() {
  if (engine_ == nullptr) {
    return;
  }
  engine_->GetShell().GetEngine()->RequestPaint();
}

void FlutterWindowsEngine::SetVisible(bool enable) {
  if (engine_ == nullptr) {
    return;
  }
  engine_->GetShell().GetEngine()->SetVisible(enable);
}

bool FlutterWindowsEngine::PostUIThreadTask(
    const fml::closure& callback) const {
  struct Captures {
    fml::closure callback;
  };
  auto captures = new Captures();
  captures->callback = std::move(callback);
  if (engine_) {
    auto task = [captures]() {
      captures->callback();
      delete captures;
    };
    if (engine_->PostUIThreadTask(std::move(task))) {
      return true;
    }
  }
  delete captures;
  return false;
}

void FlutterWindowsEngine::SetFontFaceCache(const char* font_family,
                                            const char* local_path) {
  if (engine_ == nullptr) {
    return;
  }
  engine_->GetShell().GetEngine()->SetFontFaceCache(font_family, local_path);
}

std::string FlutterWindowsEngine::ShouldInterceptUrl(
    const std::string& origin_url, bool should_decode) {
  if (platform_view_delegate_) {
    return platform_view_delegate_->ShouldInterceptUrl(origin_url,
                                                       should_decode);
  }
  return PlatformViewEmbedderDelegate::ShouldInterceptUrl(origin_url,
                                                          should_decode);
}

std::shared_ptr<clay::ResourceLoaderIntercept>
FlutterWindowsEngine::GetResourceLoaderIntercept() {
  if (platform_view_delegate_) {
    return platform_view_delegate_->GetResourceLoaderIntercept();
  }
  return PlatformViewEmbedderDelegate::GetResourceLoaderIntercept();
}

void FlutterWindowsEngine::OnDwmCompositionChanged() {
  view_->OnDwmCompositionChanged();
}

void FlutterWindowsEngine::UpdateEditState(
    int client_id, uint64_t selection_base, uint64_t composing_extent,
    const char* selection_affinity, const char* text, uint64_t selection_extent,
    uint64_t composing_base) {
  if (engine_ == nullptr) {
    return;
  }
  engine_->GetShell().GetEngine()->GetPageView()->OnPlatformUpdateEditState(
      client_id, selection_base, composing_extent, selection_affinity, text,
      selection_extent, composing_base);
}

void FlutterWindowsEngine::PerformInputAction(int client_id) {
  if (engine_ == nullptr) {
    return;
  }
  engine_->GetShell().GetEngine()->GetPageView()->OnPlatformPerformInputAction(
      client_id);
}

void FlutterWindowsEngine::PerformDragEnterAndOver(FloatPoint point) {
  if (engine_ == nullptr) {
    return;
  }
  clay::Puppet<clay::Owner::kPlatform, clay::DragDropService>
      drag_drop_service = service_manager_->GetService<DragDropService>();
  drag_drop_service.Act(
      [point](auto& impl) { impl.OnPlatformDragEnterAndOver(point); });
}

void FlutterWindowsEngine::PerformDragLeave() {
  if (engine_ == nullptr) {
    return;
  }
  clay::Puppet<clay::Owner::kPlatform, clay::DragDropService>
      drag_drop_service = service_manager_->GetService<DragDropService>();
  drag_drop_service.Act([](auto& impl) { impl.OnPlatformDragLeave(); });
}

void FlutterWindowsEngine::PerformDragDrop(
    FloatPoint point, std::string drag_type, std::string content_text,
    const std::list<std::unordered_map<std::string, std::string>>&
        content_files) {
  if (engine_ == nullptr) {
    return;
  }
  clay::Puppet<clay::Owner::kPlatform, clay::DragDropService>
      drag_drop_service = service_manager_->GetService<DragDropService>();
  drag_drop_service.Act(
      [point, drag_type, content_text, content_files](auto& impl) {
        impl.OnPlatformDragDrop(point, drag_type, content_text, content_files);
      });
}

std::unique_ptr<GLContextResult> FlutterWindowsEngine::GLContextMakeCurrent() {
  if (!view_) {
    return std::make_unique<GLContextDefaultResult>(false);
  }
  return std::make_unique<GLContextDefaultResult>(view_->MakeCurrent());
}

bool FlutterWindowsEngine::GLContextClearCurrent() {
  if (!egl_manager()) {
    return false;
  }
  return egl_manager()->render_context()->ClearCurrent();
}

void FlutterWindowsEngine::GLContextSetDamageRegion(
    const std::optional<skity::Rect>& region) {
  if (!region.has_value() || !view_) {
    return;
  }
  skity::Rect damage_rect = region.value();
  int left = static_cast<int>(std::floor(damage_rect.Left()));
  int top = static_cast<int>(std::floor(damage_rect.Top()));
  int right = static_cast<int>(std::ceil(damage_rect.Right()));
  int bottom = static_cast<int>(std::ceil(damage_rect.Bottom()));
  view_->SetDamageRegion({left, top, right - left, bottom - top});
}

bool FlutterWindowsEngine::GLContextPresent(const GLPresentInfo& present_info) {
  if (!view_) {
    return false;
  }
  return view_->SwapBuffers();
}

GLFBOInfo FlutterWindowsEngine::GLContextFBO(GLFrameInfo frame_info) const {
  if (!view_) {
    return {.fbo_id = kWindowFrameBufferID, .existing_damage = std::nullopt};
  }
  auto damage_rect = view_->GetDamageRegion();
  return {
      .fbo_id = view_->GetFrameBufferId(frame_info.width, frame_info.height),
      .existing_damage = damage_rect};
}

bool FlutterWindowsEngine::GLContextFBOResetAfterPresent() const {
  return true;
}

GPUSurfaceGLDelegate::GLProcResolver FlutterWindowsEngine::GetGLProcResolver()
    const {
  return [](const char* name) -> void* {
    return reinterpret_cast<void*>(eglGetProcAddress(name));
  };
}

SurfaceFrame::FramebufferInfo FlutterWindowsEngine::GLContextFramebufferInfo()
    const {
  auto info = SurfaceFrame::FramebufferInfo{};
  info.supports_readback = true;
  info.supports_partial_repaint = true;
  // Unspecified damage (nullopt) invokes full-frame rasterization (no partial
  // redraw); use an empty skity::Rect to indicate no existing damage.
  info.existing_damage = skity::Rect();
  return info;
}

bool FlutterWindowsEngine::OnPresentBackingStore(const void* allocation,
                                                 size_t row_bytes,
                                                 size_t height) {
  if (view_) {
    return view_->PresentSoftwareBitmap(allocation, row_bytes, height);
  }
  return false;
}

std::unique_ptr<FlutterWindowsView> FlutterWindowsEngine::CreateOverlayView(
    std::unique_ptr<WindowBindingHandler> window) {
  FlutterViewId view_id = 0;
  {
    std::unique_lock write_lock(views_mutex_);
    view_id = ++next_view_id_;
  }
  std::unique_ptr<FlutterWindowsView> view =
      std::make_unique<OverlayWindowsView>(view_id, std::move(window));
  view->SetEngine(this);
  view->CreateRenderSurface();
  if (!view->GetEngine()->running()) {
    if (!view->GetEngine()->Run()) {
      NOTREACHED();
    }
  }
  {
    std::unique_lock write_lock(views_mutex_);
    overlay_views_[view_id] = view.get();
  }
  return view;
}

void FlutterWindowsEngine::RemoveOverlayView(FlutterViewId view_id) {
  std::unique_lock write_lock(views_mutex_);
  auto it = overlay_views_.find(view_id);
  if (it != overlay_views_.end()) {
    it->second->ReleaseAllPointers();
    ClearPointerOwnersForView(view_id);
    overlay_views_.erase(it);
  }
}

FlutterWindowsView* FlutterWindowsEngine::GetOverlayView(
    FlutterViewId view_id) {
  std::shared_lock read_lock(views_mutex_);
  auto iterator = overlay_views_.find(view_id);
  if (iterator == overlay_views_.end()) {
    return nullptr;
  }

  return iterator->second;
}

}  // namespace clay
