// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/platform/embedder/platform_view_embedder.h"

#include <string>
#include <utility>

#include "base/include/fml/make_copyable.h"
#include "clay/net/loader/resource_loader_intercept.h"

namespace clay {

PlatformViewEmbedder::PlatformViewEmbedder(
    std::shared_ptr<clay::ServiceManager> service_manager,
    PlatformView::Delegate& delegate, const clay::TaskRunners& task_runners,
    EmbedderSurfaceSoftwareDelegate* software_delegate,
    PlatformDispatchTable platform_dispatch_table, void* user_data,
    PlatformViewEmbedderDelegate* embedder_delegate)
    : PlatformView(service_manager, delegate, task_runners),
      embedder_surface_(
          fml::MakeRefCounted<EmbedderSurfaceSoftware>(software_delegate)),
      platform_dispatch_table_(std::move(platform_dispatch_table)),
      embedder_delegate_(embedder_delegate) {}

#ifdef SHELL_ENABLE_GL
PlatformViewEmbedder::PlatformViewEmbedder(
    std::shared_ptr<clay::ServiceManager> service_manager,
    PlatformView::Delegate& delegate, const clay::TaskRunners& task_runners,
    GPUSurfaceGLDelegate* gl_delegate,
    PlatformDispatchTable platform_dispatch_table, void* user_data,
    PlatformViewEmbedderDelegate* embedder_delegate)
    : PlatformView(service_manager, delegate, task_runners),
      embedder_surface_(fml::MakeRefCounted<EmbedderSurfaceGL>(gl_delegate)),
      platform_dispatch_table_(std::move(platform_dispatch_table)),
      embedder_delegate_(embedder_delegate) {}
#endif

#ifdef SHELL_ENABLE_METAL
PlatformViewEmbedder::PlatformViewEmbedder(
    std::shared_ptr<clay::ServiceManager> service_manager,
    PlatformView::Delegate& delegate, const clay::TaskRunners& task_runners,
    fml::RefPtr<EmbedderSurfaceMetal> embedder_surface,
    PlatformDispatchTable platform_dispatch_table, void* user_data,
    PlatformViewEmbedderDelegate* embedder_delegate)
    : PlatformView(service_manager, delegate, task_runners),
      embedder_surface_(std::move(embedder_surface)),
      platform_dispatch_table_(std::move(platform_dispatch_table)),
      embedder_delegate_(embedder_delegate) {}
#endif

PlatformViewEmbedder::~PlatformViewEmbedder() = default;

void PlatformViewEmbedder::NotifyDestroyed() {
  PlatformView::NotifyDestroyed();
  // This context needs to be deallocated from the raster thread in order to
  // keep a coherent usage of egl from a single thread.
  fml::AutoResetWaitableEvent latch;
  fml::TaskRunner::RunNowOrPostTask(
      task_runners_.GetRasterTaskRunner(),
      [&latch, surface = embedder_surface_.get()]() {
        auto gr_context = surface->GetMainGrContext();
        if (gr_context) {
#ifndef ENABLE_SKITY
          bool has_released = false;
#ifdef SHELL_ENABLE_GL
          if (gr_context->backend() == GrBackendApi::kOpenGL) {
            auto surface_gl = static_cast<EmbedderSurfaceGL*>(surface);
            auto status = static_cast<GPUSurfaceGLDelegate*>(surface_gl)
                              ->GLContextMakeCurrent();
            if (status->GetResult()) {
              gr_context->releaseResourcesAndAbandonContext();
              static_cast<GPUSurfaceGLDelegate*>(surface_gl)
                  ->GLContextClearCurrent();
              has_released = true;
            }
          }
#endif
          if (!has_released) {
            gr_context->releaseResourcesAndAbandonContext();
          }
#endif
        }
        latch.Signal();
      });
  latch.Wait();
}

#ifdef ENABLE_ACCESSIBILITY
void PlatformViewEmbedder::UpdateSemantics(
    const clay::SemanticsUpdateNodes& update_nodes) {
#if 0
  if (platform_dispatch_table_.update_semantics_callback != nullptr) {
    platform_dispatch_table_.update_semantics_callback(std::move(update),
                                                       std::move(actions));
  }
#endif
}
#endif

// |PlatformView|
std::unique_ptr<std::vector<std::string>>
PlatformViewEmbedder::ComputePlatformResolvedLocales(
    const std::vector<std::string>& supported_locale_data) {
  if (platform_dispatch_table_.compute_platform_resolved_locale_callback !=
      nullptr) {
    return platform_dispatch_table_.compute_platform_resolved_locale_callback(
        supported_locale_data);
  }
  std::unique_ptr<std::vector<std::string>> out =
      std::make_unique<std::vector<std::string>>();
  return out;
}

// |PlatformView|
void PlatformViewEmbedder::OnPreEngineRestart() const {
  if (platform_dispatch_table_.on_pre_engine_restart_callback != nullptr) {
    platform_dispatch_table_.on_pre_engine_restart_callback();
  }
}

// |PlatformView|
void PlatformViewEmbedder::SetClipboardData(const std::u16string& data) {
  if (platform_dispatch_table_.clipboard.set_clipboard_data_callback !=
      nullptr) {
    platform_dispatch_table_.clipboard.set_clipboard_data_callback(data);
  }
}

// |PlatformView|
std::u16string PlatformViewEmbedder::GetClipboardData() {
  if (platform_dispatch_table_.clipboard.get_clipboard_data_callback !=
      nullptr) {
    return platform_dispatch_table_.clipboard.get_clipboard_data_callback();
  }
  return std::u16string();
}

// |PlatformView|
void PlatformViewEmbedder::SetTextInputClient(int client_id,
                                              const char* input_action,
                                              const char* input_type) {
  if (platform_dispatch_table_.textinput.set_text_input_client_callback !=
      nullptr) {
    platform_dispatch_table_.textinput.set_text_input_client_callback(
        client_id, input_action, input_type);
  }
}

// |PlatformView|
void PlatformViewEmbedder::ClearTextInputClient() {
  if (platform_dispatch_table_.textinput.clear_text_input_client_callback !=
      nullptr) {
    platform_dispatch_table_.textinput.clear_text_input_client_callback();
  }
}

// |PlatformView|
void PlatformViewEmbedder::SetEditableTransform(
    const float transform_matrix[16]) {
  if (platform_dispatch_table_.textinput.set_editable_transform_callback !=
      nullptr) {
    platform_dispatch_table_.textinput.set_editable_transform_callback(
        transform_matrix);
  }
}

// |PlatformView|
void PlatformViewEmbedder::SetEditingState(
    uint64_t selection_base, uint64_t composing_extent,
    const std::string& selection_affinity, const std::string& text,
    bool selection_directional, uint64_t selection_extent,
    uint64_t composing_base) {
  if (platform_dispatch_table_.textinput.set_editing_state_callback !=
      nullptr) {
    platform_dispatch_table_.textinput.set_editing_state_callback(
        selection_base, composing_extent, selection_affinity, text,
        selection_directional, selection_extent, composing_base);
  }
}

// |PlatformView|
void PlatformViewEmbedder::SetCaretRect(float x, float y, float width,
                                        float height) {
  if (platform_dispatch_table_.textinput.set_caret_rect_callback != nullptr) {
    platform_dispatch_table_.textinput.set_caret_rect_callback(x, y, width,
                                                               height);
  }
}

// |PlatformView|
void PlatformViewEmbedder::setMarkedTextRect(float x, float y, float width,
                                             float height) {
  if (platform_dispatch_table_.textinput.set_marked_text_rect_callback !=
      nullptr) {
    platform_dispatch_table_.textinput.set_marked_text_rect_callback(
        x, y, width, height);
  }
}

// |PlatformView|
void PlatformViewEmbedder::ShowTextInput() {
  if (platform_dispatch_table_.textinput.show_text_input_callback != nullptr) {
    platform_dispatch_table_.textinput.show_text_input_callback();
  }
}

// |PlatformView|
void PlatformViewEmbedder::HideTextInput() {
  if (platform_dispatch_table_.textinput.hide_text_input_callback != nullptr) {
    platform_dispatch_table_.textinput.hide_text_input_callback();
  }
}

// |PlatformView|
std::string PlatformViewEmbedder::InputFilter(const std::string& input,
                                              const std::string& pattern) {
  if (platform_dispatch_table_.textinput.input_filter_callback != nullptr) {
    return platform_dispatch_table_.textinput.input_filter_callback(input,
                                                                    pattern);
  } else {
    return input;
  }
}

// |PlatformView|
void PlatformViewEmbedder::WindowMove() {
  if (platform_dispatch_table_.window_move_callback != nullptr) {
    platform_dispatch_table_.window_move_callback();
  }
}

// |PlatformView|
void PlatformViewEmbedder::ActivateSystemCursor(int type,
                                                const std::string& path) {
  if (platform_dispatch_table_.activate_system_cursor_callback != nullptr) {
    platform_dispatch_table_.activate_system_cursor_callback(type, path);
  }
}

std::string PlatformViewEmbedder::ShouldInterceptUrl(
    const std::string& origin_url, bool should_decode) {
  if (embedder_delegate_) {
    return embedder_delegate_->ShouldInterceptUrl(origin_url, should_decode);
  }
  return origin_url;
}

std::shared_ptr<clay::ResourceLoaderIntercept>
PlatformViewEmbedder::GetResourceLoaderIntercept() {
  if (embedder_delegate_) {
    return embedder_delegate_->GetResourceLoaderIntercept();
  }
  return nullptr;
}

fml::RefPtr<OutputSurface> PlatformViewEmbedder::GetOutputSurface() const {
  return embedder_surface_;
}

}  // namespace clay
