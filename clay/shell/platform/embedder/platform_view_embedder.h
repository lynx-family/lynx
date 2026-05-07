// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_PLATFORM_EMBEDDER_PLATFORM_VIEW_EMBEDDER_H_
#define CLAY_SHELL_PLATFORM_EMBEDDER_PLATFORM_VIEW_EMBEDDER_H_

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "base/include/fml/macros.h"
#include "clay/flow/embedded_views.h"
#include "clay/public/clay.h"
#include "clay/shell/common/platform_view.h"
#include "clay/shell/platform/embedder/embedder_surface.h"
#include "clay/shell/platform/embedder/embedder_surface_software.h"
#include "clay/shell/platform/embedder/platform_view_embedder_delegate.h"
#include "clay/shell/platform/embedder/vsync_waiter_embedder.h"

#ifdef SHELL_ENABLE_GL
#include "clay/shell/platform/embedder/embedder_surface_gl.h"
#endif

#ifdef SHELL_ENABLE_METAL
#include "clay/shell/platform/embedder/embedder_surface_metal.h"
#endif

namespace clay {

class PlatformViewEmbedder final : public PlatformView {
 public:
  using ComputePlatformResolvedLocaleCallback =
      std::function<std::unique_ptr<std::vector<std::string>>(
          const std::vector<std::string>& supported_locale_data)>;
  using OnPreEngineRestartCallback = std::function<void()>;
  using SetClipboardDataCallback =
      std::function<void(const std::u16string& data)>;
  using GetClipboardDataCallback = std::function<std::u16string()>;
  using SetTextInputClientCallback = std::function<void(
      int client_id, const char* input_action, const char* input_type)>;
  using ClearTextInputClientCallback = std::function<void()>;
  using SetEditableTransformCallback =
      std::function<void(const float transform_matrix[16])>;
  using SetEditingStateCallback =
      std::function<void(uint64_t selection_base, uint64_t composing_extent,
                         const std::string& selection_affinity,
                         const std::string& text, bool selection_directional,
                         uint64_t selection_extent, uint64_t composing_base)>;
  using SetCaretRectCallback =
      std::function<void(float x, float y, float width, float height)>;
  using UpdateCaretPositionCallback =
      std::function<void(float x, float y, float width, float height)>;
  using SetMarkedTextRectCallback =
      std::function<void(float x, float y, float width, float height)>;
  using ShowTextInputCallback = std::function<void()>;
  using HideTextInputCallback = std::function<void()>;
  using FilterInputCallback =
      std::function<std::string(const std::string&, const std::string&)>;
  using SetCursorPositionCallback = std::function<void(int position)>;
  using WindowMoveCallback = std::function<void()>;
  using ActivateSystemCursorCallback =
      std::function<void(int type, const std::string& path)>;

  struct PlatformDispatchTable {
    ComputePlatformResolvedLocaleCallback
        compute_platform_resolved_locale_callback;
    OnPreEngineRestartCallback on_pre_engine_restart_callback;  // optional

    struct ClipboardDispatchTable {
      SetClipboardDataCallback set_clipboard_data_callback;
      GetClipboardDataCallback get_clipboard_data_callback;
    } clipboard;

    struct TextInputDispatchTable {
      SetTextInputClientCallback set_text_input_client_callback;
      ClearTextInputClientCallback clear_text_input_client_callback;
      SetEditableTransformCallback set_editable_transform_callback;
      SetEditingStateCallback set_editing_state_callback;
      SetCaretRectCallback set_caret_rect_callback;
      UpdateCaretPositionCallback update_caret_position_callback;
      SetMarkedTextRectCallback set_marked_text_rect_callback;
      ShowTextInputCallback show_text_input_callback;
      HideTextInputCallback hide_text_input_callback;
      FilterInputCallback input_filter_callback;
      SetCursorPositionCallback cursor_position_callback;
    } textinput;
    WindowMoveCallback window_move_callback;
    ActivateSystemCursorCallback activate_system_cursor_callback;
  };

  // Create a platform view that sets up a software rasterizer.
  PlatformViewEmbedder(std::shared_ptr<clay::ServiceManager> service_manager,
                       PlatformView::Delegate& delegate,
                       const clay::TaskRunners& task_runners,
                       EmbedderSurfaceSoftwareDelegate* software_delegate,
                       PlatformDispatchTable platform_dispatch_table,
                       void* user_data,
                       PlatformViewEmbedderDelegate* embedder_delegate);

#ifdef SHELL_ENABLE_GL
  // Creates a platform view that sets up an OpenGL rasterizer.
  PlatformViewEmbedder(std::shared_ptr<clay::ServiceManager> service_manager,
                       PlatformView::Delegate& delegate,
                       const clay::TaskRunners& task_runners,
                       GPUSurfaceGLDelegate* gl_delegate,
                       PlatformDispatchTable platform_dispatch_table,
                       void* user_data,
                       PlatformViewEmbedderDelegate* embedder_delegate);
#endif

#ifdef SHELL_ENABLE_METAL
  // Creates a platform view that sets up an metal rasterizer.
  PlatformViewEmbedder(std::shared_ptr<clay::ServiceManager> service_manager,
                       PlatformView::Delegate& delegate,
                       const clay::TaskRunners& task_runners,
                       fml::RefPtr<EmbedderSurfaceMetal> embedder_surface,
                       PlatformDispatchTable platform_dispatch_table,
                       void* user_data,
                       PlatformViewEmbedderDelegate* embedder_delegate);
#endif

  ~PlatformViewEmbedder() override;

  // |PlatformView|
  void NotifyDestroyed() override;

  std::string ShouldInterceptUrl(const std::string& origin_url,
                                 bool should_decode) override;

  std::shared_ptr<clay::ResourceLoaderIntercept> GetResourceLoaderIntercept()
      override;

  fml::RefPtr<OutputSurface> GetOutputSurface() const override;

 private:
  fml::RefPtr<EmbedderSurface> embedder_surface_;
  PlatformDispatchTable platform_dispatch_table_;

  PlatformViewEmbedderDelegate* embedder_delegate_ = nullptr;

  // |PlatformView|
  void OnPreEngineRestart() const override;
  // |PlatformView|
  void SetClipboardData(const std::u16string& data) override;
  // |PlatformView|
  std::u16string GetClipboardData() override;
  // |PlatformView|
  void SetTextInputClient(int client_id, const char* input_action,
                          const char* input_type) override;
  // |PlatformView|
  void ClearTextInputClient() override;
  // |PlatformView|
  void SetEditableTransform(const float transform_matrix[16]) override;
  // |PlatformView|
  void SetEditingState(uint64_t selection_base, uint64_t composing_extent,
                       const std::string& selection_affinity,
                       const std::string& text, bool selection_directional,
                       uint64_t selection_extent,
                       uint64_t composing_base) override;
  // |PlatformView|
  void SetCaretRect(float x, float y, float width, float height) override;
  // |PlatformView|
  void UpdateCaretPosition(float x, float y, float width,
                           float height) override;
  // |PlatformView|
  void setMarkedTextRect(float x, float y, float width, float height) override;
  // |PlatformView|
  void ShowTextInput() override;
  // |PlatformView|
  void HideTextInput() override;
  // |PlatformView|
  void SetCursorPosition(int position) override;
  // |PlatformView|
  std::string InputFilter(const std::string& input,
                          const std::string& pattern) override;
  // |PlatformView|
  void WindowMove() override;
  // |PlatformView|
  void ActivateSystemCursor(int type, const std::string& path) override;

  // |PlatformView|
  std::unique_ptr<std::vector<std::string>> ComputePlatformResolvedLocales(
      const std::vector<std::string>& supported_locale_data) override;

  BASE_DISALLOW_COPY_AND_ASSIGN(PlatformViewEmbedder);
};

}  // namespace clay

#endif  // CLAY_SHELL_PLATFORM_EMBEDDER_PLATFORM_VIEW_EMBEDDER_H_
