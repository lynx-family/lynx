// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_PLATFORM_WINDOWS_FLUTTER_WINDOWS_ENGINE_H_
#define CLAY_SHELL_PLATFORM_WINDOWS_FLUTTER_WINDOWS_ENGINE_H_

#include <chrono>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "base/include/closure.h"
#include "clay/common/service/service_manager.h"
#include "clay/fml/mapping.h"
#include "clay/shell/platform/embedder/embedder_engine.h"
#include "clay/shell/platform/embedder/embedder_surface_gl.h"
#include "clay/shell/platform/embedder/embedder_surface_software.h"
#include "clay/shell/platform/embedder/platform_view_embedder_delegate.h"
#include "clay/shell/platform/windows/cursor_handler.h"
#include "clay/shell/platform/windows/egl/manager.h"
#include "clay/shell/platform/windows/flutter_project_bundle.h"
#include "clay/shell/platform/windows/keyboard_handler_base.h"
#include "clay/shell/platform/windows/keyboard_key_embedder_handler.h"
#include "clay/shell/platform/windows/platform_handler.h"
#include "clay/shell/platform/windows/task_runner.h"
#include "clay/shell/platform/windows/text_input_plugin.h"
#include "clay/shell/platform/windows/window_proc_delegate_manager.h"
#include "third_party/rapidjson/document.h"

namespace clay {

class FlutterWindowsView;
class OverlayViewManagerService;

// A unique identifier for a view.
using FlutterViewId = int64_t;

// The ID that the current view will have.
constexpr FlutterViewId kImplicitViewId = 0;

// Update the thread priority for the Windows engine.
static void WindowsPlatformThreadPrioritySetter(
    fml::Thread::ThreadPriority priority) {
  // TODO(99502): Add support for tracing to the windows embedding so we can
  // mark thread priorities and success/failure.
  switch (priority) {
    case fml::Thread::ThreadPriority::LOW:
    case fml::Thread::ThreadPriority::BACKGROUND: {
      SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
      break;
    }
    case fml::Thread::ThreadPriority::HIGH: {
      SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
      break;
    }
    case fml::Thread::ThreadPriority::NORMAL: {
      // For normal or default priority we do not need to set the priority
      // class.
      break;
    }
  }
}

// Manages state associated with the underlying FlutterEngine that isn't
// related to its display.
//
// In most cases this will be associated with a FlutterView, but if not will
// run in headless mode.
class FlutterWindowsEngine : public PlatformViewEmbedderDelegate,
                             public GPUSurfaceGLDelegate,
                             public EmbedderSurfaceSoftwareDelegate {
 public:
  // Creates a new Flutter engine object configured to run |project|.
  explicit FlutterWindowsEngine(const FlutterProjectBundle& project);

  virtual ~FlutterWindowsEngine();

  // Prevent copying.
  FlutterWindowsEngine(FlutterWindowsEngine const&) = delete;
  FlutterWindowsEngine& operator=(FlutterWindowsEngine const&) = delete;

  // Starts running the entrypoint function specifed in the project bundle. If
  // unspecified, defaults to main().
  //
  // Returns false if the engine couldn't be started.
  bool Run();

  // Starts running the engine with the given entrypoint. If the empty string
  // is specified, defaults to the entrypoint function specified in the project
  // bundle, or main() if both are unspecified.
  //
  // Returns false if the engine couldn't be started or if conflicting,
  // non-default values are passed here and in the project bundle..
  //
  // DEPRECATED: Prefer setting the entrypoint in the FlutterProjectBundle
  // passed to the constructor and calling the no-parameter overload.
  bool Run(std::string_view entrypoint);

  // Returns true if the engine is currently running.
  bool running() const { return engine_ != nullptr; }

  // Stops the engine. This invalidates the pointer returned by engine().
  //
  // Returns false if stopping the engine fails, or if it was not running.
  bool Stop();

  // Sets the view that is displaying this engine's content.
  void SetView(FlutterWindowsView* view);

  // The view displaying this engine's content, if any. This will be null for
  // headless engines.
  FlutterWindowsView* view() { return view_; }

  // Sets switches member to the given switches.
  void SetSwitches(const std::vector<std::string>& switches);

  TaskRunner* task_runner() { return task_runner_.get(); }

  // The EGL manager object. If this is nullptr, then we are
  // rendering using software instead of OpenGL.
  egl::Manager* egl_manager() const { return egl_manager_.get(); }

  WindowProcDelegateManager* window_proc_delegate_manager() {
    return window_proc_delegate_manager_.get();
  }

  // Informs the engine that the window metrics have changed.
  void SendWindowMetricsEvent(const clay::ViewportMetrics& metrics);

  // Informs the engine of an incoming pointer event.
  void SendPointerEvent(const ClayPointerEvent& event);
  bool SendPointerEvent(FlutterWindowsView* source,
                        const ClayPointerEvent& event);

  // Informs the engine of an incoming key event.
  void SendKeyEvent(const ClayKeyEvent& event, ClayKeyEventCallback callback,
                    void* user_data);

  KeyboardHandlerBase* keyboard_key_handler() {
    return keyboard_key_handler_.get();
  }
  TextInputPlugin* text_input_plugin() { return text_input_plugin_.get(); }

  // Informs the engine that the system font list has changed.
  void ReloadSystemFonts();

  // Informs the engine that a new frame is needed to redraw the content.
  void ScheduleFrame();
  void* GetViewContext();
  void EnableDefaultFocusRing();
  void EnablePerformanceOverlay();
  // Set the callback that is called when the next frame is drawn.
  void SetNextFrameCallback(fml::closure callback);

  // Posts the given callback onto the raster thread.
  bool PostRasterThreadTask(fml::closure callback) const;
  // Posts the given callback onto the ui thread.
  bool PostUIThreadTask(const fml::closure& callback) const;

  // Invoke on the embedder's vsync callback to schedule a frame.
  void OnVsync(intptr_t baton);

  // Update the high contrast feature state.
  void UpdateHighContrastEnabled(bool enabled);

  // Returns true if the high contrast feature is enabled.
  bool high_contrast_enabled() const { return high_contrast_enabled_; }

  // Returns the executable name for this process or "Flutter" if unknown.
  std::string GetExecutableName() const;

  void OnEnterForeground();
  void OnEnterBackground();
  void RequestPaint();

  void SetVisible(bool enable);
  void SetFontFaceCache(const char* font_family, const char* local_path);
  std::string ShouldInterceptUrl(const std::string& origin_url,
                                 bool should_decode) override;
  std::shared_ptr<clay::ResourceLoaderIntercept> GetResourceLoaderIntercept()
      override;
  void SetPlatformViewEmbedderDelegate(PlatformViewEmbedderDelegate* delegate) {
    platform_view_delegate_ = delegate;
  }

  const std::shared_ptr<clay::ServiceManager>& GetServiceManager() const {
    return service_manager_;
  }

  // Called when a WM_DWMCOMPOSITIONCHANGED message is received.
  void OnDwmCompositionChanged();

  void UpdateEditState(int client_id, uint64_t selection_base,
                       uint64_t composing_extent,
                       const char* selection_affinity, const char* text,
                       uint64_t selection_extent, uint64_t composing_base);
  void PerformInputAction(int client_id);

  void PerformDragEnterAndOver(FloatPoint point);
  void PerformDragLeave();
  void PerformDragDrop(
      FloatPoint point, std::string drag_type, std::string content_text,
      const std::list<std::unordered_map<std::string, std::string>>&
          content_files);

  std::unique_ptr<FlutterWindowsView> CreateOverlayView(
      std::unique_ptr<WindowBindingHandler> window);

  void RemoveOverlayView(FlutterViewId view_id);

  FlutterWindowsView* GetOverlayView(FlutterViewId view_id);

 protected:
  // Creates the keyboard key handler.
  //
  // Exposing this method allows unit tests to override in order to
  // capture information.
  virtual std::unique_ptr<KeyboardHandlerBase> CreateKeyboardKeyHandler(
      KeyboardKeyEmbedderHandler::GetKeyStateHandler get_key_state,
      KeyboardKeyEmbedderHandler::MapVirtualKeyToScanCode map_vk_to_scan);

  // Creates the text input plugin.
  //
  // Exposing this method allows unit tests to override in order to
  // capture information.
  virtual std::unique_ptr<TextInputPlugin> CreateTextInputPlugin();

  // Invoked by the engine right before the engine is restarted.
  //
  // This should reset necessary states to as if the engine has just been
  // created. This is typically caused by a hot restart (Shift-R in CLI.)
  void OnPreEngineRestart();

  // |GPUSurfaceGLDelegate|
  std::unique_ptr<GLContextResult> GLContextMakeCurrent() override;

  // |GPUSurfaceGLDelegate|
  bool GLContextClearCurrent() override;

  // |GPUSurfaceGLDelegate|
  void GLContextSetDamageRegion(
      const std::optional<skity::Rect>& region) override;

  // |GPUSurfaceGLDelegate|
  bool GLContextPresent(const GLPresentInfo& present_info) override;

  // |GPUSurfaceGLDelegate|
  GLFBOInfo GLContextFBO(GLFrameInfo frame_info) const override;

  // |GPUSurfaceGLDelegate|
  bool GLContextFBOResetAfterPresent() const override;

  // |GPUSurfaceGLDelegate|
  GLProcResolver GetGLProcResolver() const override;

  // |GPUSurfaceGLDelegate|
  SurfaceFrame::FramebufferInfo GLContextFramebufferInfo() const override;

  // |EmbedderSurfaceSoftwareDelegate|
  bool OnPresentBackingStore(const void* allocation, size_t row_bytes,
                             size_t height) override;

 private:
  // Sends system locales to the engine.
  //
  // Should be called just after the engine is run, and after any relevant
  // system changes.
  void SendSystemLocales();

  // Create the keyboard & text input sub-systems.
  //
  // This requires that a view is attached to the engine.
  // Calling this method again resets the keyboard state.
  void InitializeKeyboard();

  FlutterWindowsView* GetViewById(FlutterViewId view_id);
  void ClearPointerOwnersForView(FlutterViewId view_id);

  std::unique_ptr<EmbedderEngine> engine_;

  std::unique_ptr<FlutterProjectBundle> project_;

  // The ID that the next view will have.
  FlutterViewId next_view_id_ = kImplicitViewId;

  // The view displaying the content running in this engine, if any.
  FlutterWindowsView* view_ = nullptr;

  // The views displaying the content running in this engine, if any.
  //
  // This is read and mutated by the platform thread. This is read by the raster
  // thread to present content to a view.
  //
  // Reads to this object on non-platform threads must be protected
  // by acquiring a shared lock on |views_mutex_|.
  //
  // Writes to this object must only happen on the platform thread
  // and must be protected by acquiring an exclusive lock on |views_mutex_|.
  std::unordered_map<FlutterViewId, FlutterWindowsView*> overlay_views_;

  // The Windows view ID that currently owns each pointer device.
  std::unordered_map<int32_t, FlutterViewId> pointer_owner_views_;

  // The mutex that protects the |views_| map.
  //
  // The raster thread acquires a shared lock to present to a view.
  //
  // The platform thread acquires a shared lock to access the view.
  // The platform thread acquires an exclusive lock before adding
  // a view to the engine or after removing a view from the engine.
  mutable std::shared_mutex views_mutex_;

  // Task runner for tasks posted from the engine.
  std::unique_ptr<TaskRunner> task_runner_;

  // An object used for intializing Angle and creating / destroying render
  // surfaces. Surface creation functionality requires a valid render_target.
  // May be nullptr if ANGLE failed to initialize.
  std::unique_ptr<egl::Manager> egl_manager_;

  // Handler for cursor events.
  std::unique_ptr<CursorHandler> cursor_handler_;

  // Handler for the flutter/platform.
  std::unique_ptr<PlatformHandler> platform_handler_;

  // Handlers for keyboard events from Windows.
  std::unique_ptr<KeyboardHandlerBase> keyboard_key_handler_;

  // Handlers for text events from Windows.
  std::unique_ptr<TextInputPlugin> text_input_plugin_;

  // The approximate time between vblank events.
  int64_t FrameInterval();

  // The start time used to align frames.
  int64_t start_time_ = 0;

  bool high_contrast_enabled_ = false;

  // The manager for WindowProc delegate registration and callbacks.
  std::unique_ptr<WindowProcDelegateManager> window_proc_delegate_manager_;

  // The on frame drawn callback.
  fml::closure next_frame_callback_;

  PlatformViewEmbedderDelegate* platform_view_delegate_ = nullptr;

  std::shared_ptr<clay::ServiceManager> service_manager_;

  std::shared_ptr<OverlayViewManagerService> overlay_view_manager_service_;
};

}  // namespace clay

#endif  // CLAY_SHELL_PLATFORM_WINDOWS_FLUTTER_WINDOWS_ENGINE_H_
