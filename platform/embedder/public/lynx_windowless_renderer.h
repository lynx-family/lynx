// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_PUBLIC_LYNX_WINDOWLESS_RENDERER_H_
#define PLATFORM_EMBEDDER_PUBLIC_LYNX_WINDOWLESS_RENDERER_H_

#include <cstdbool>
#include <memory>

#include "capi/lynx_types.h"
#include "capi/lynx_windowless_renderer_capi.h"

namespace lynx {
namespace pub {

class LynxWindowlessRenderer
    : public std::enable_shared_from_this<LynxWindowlessRenderer> {
 public:
  explicit LynxWindowlessRenderer(lynx_windowless_renderer_type_e type)
      : type_(type) {}

  void InitIfNeeded() {
    if (windowless_renderer_) {
      return;
    }
    windowless_renderer_ = lynx_windowless_renderer_create_with_finalizer(
        type_, new std::weak_ptr<LynxWindowlessRenderer>(weak_from_this()),
        [](lynx_windowless_renderer_t* renderer, void* user_data) {
          std::weak_ptr<LynxWindowlessRenderer>* weak_ptr =
              reinterpret_cast<std::weak_ptr<LynxWindowlessRenderer>*>(
                  user_data);
          delete weak_ptr;
        });
    if (type_ == kRendererTypeSoftware) {
      lynx_windowless_renderer_bind_on_software_present(
          windowless_renderer_,
          [](lynx_windowless_renderer_t* renderer, const void* allocation,
             size_t row_bytes, size_t height) -> bool {
            std::shared_ptr<LynxWindowlessRenderer> renderer_wrapper =
                LynxWindowlessRenderer::Unwrap(
                    lynx_windowless_renderer_get_user_data(renderer));
            if (renderer_wrapper) {
              return renderer_wrapper->OnSoftwarePresent(allocation, row_bytes,
                                                         height);
            }
            return false;
          });
    } else if (type_ == kRendererTypeGL || type_ == kRendererTypeGLDirect) {
      lynx_windowless_renderer_bind_on_gl_make_current(
          windowless_renderer_,
          [](lynx_windowless_renderer_t* renderer) -> bool {
            std::shared_ptr<LynxWindowlessRenderer> renderer_wrapper =
                LynxWindowlessRenderer::Unwrap(
                    lynx_windowless_renderer_get_user_data(renderer));
            if (renderer_wrapper) {
              return renderer_wrapper->OnGLMakeCurrent();
            }
            return false;
          });
      lynx_windowless_renderer_bind_on_gl_clear_current(
          windowless_renderer_,
          [](lynx_windowless_renderer_t* renderer) -> bool {
            std::shared_ptr<LynxWindowlessRenderer> renderer_wrapper =
                LynxWindowlessRenderer::Unwrap(
                    lynx_windowless_renderer_get_user_data(renderer));
            if (renderer_wrapper) {
              return renderer_wrapper->OnGLClearCurrent();
            }
            return false;
          });

      lynx_windowless_renderer_bind_on_gl_present(
          windowless_renderer_,
          [](lynx_windowless_renderer_t* renderer) -> bool {
            std::shared_ptr<LynxWindowlessRenderer> renderer_wrapper =
                LynxWindowlessRenderer::Unwrap(
                    lynx_windowless_renderer_get_user_data(renderer));
            if (renderer_wrapper) {
              return renderer_wrapper->OnGLPresent();
            }
            return false;
          });
      lynx_windowless_renderer_bind_on_gl_create_fbo(
          windowless_renderer_,
          [](lynx_windowless_renderer_t* renderer, int width,
             int height) -> uint32_t {
            std::shared_ptr<LynxWindowlessRenderer> renderer_wrapper =
                LynxWindowlessRenderer::Unwrap(
                    lynx_windowless_renderer_get_user_data(renderer));
            if (renderer_wrapper) {
              return renderer_wrapper->OnGLCreateFBO(width, height);
            }
            return 0;
          });
      lynx_windowless_renderer_bind_on_gl_proc_resolver(
          windowless_renderer_,
          [](lynx_windowless_renderer_t* renderer, const char* name) -> void* {
            std::shared_ptr<LynxWindowlessRenderer> renderer_wrapper =
                LynxWindowlessRenderer::Unwrap(
                    lynx_windowless_renderer_get_user_data(renderer));
            if (renderer_wrapper) {
              return renderer_wrapper->OnGLProcResolver(name);
            }
            return nullptr;
          });
    } else if (type_ == kRendererTypeAccelerated) {
      lynx_windowless_renderer_bind_on_accelerated_present(
          windowless_renderer_,
          [](lynx_windowless_renderer_t* renderer) -> bool {
            std::shared_ptr<LynxWindowlessRenderer> renderer_wrapper =
                LynxWindowlessRenderer::Unwrap(
                    lynx_windowless_renderer_get_user_data(renderer));
            if (renderer_wrapper) {
              return renderer_wrapper->OnAcceleratedPresent();
            }
            return false;
          });
    }

    lynx_windowless_renderer_bind_on_post_task(
        windowless_renderer_,
        [](lynx_windowless_renderer_t* renderer, lynx_task_t task,
           uint64_t interval_nanoseconds) {
          std::shared_ptr<LynxWindowlessRenderer> renderer_wrapper =
              LynxWindowlessRenderer::Unwrap(
                  lynx_windowless_renderer_get_user_data(renderer));
          if (renderer_wrapper) {
            renderer_wrapper->OnPostTask(task, interval_nanoseconds);
          }
        });
    lynx_windowless_renderer_bind_get_clipboard_data(
        windowless_renderer_,
        [](lynx_windowless_renderer_t* renderer) -> const char* {
          std::shared_ptr<LynxWindowlessRenderer> renderer_wrapper =
              LynxWindowlessRenderer::Unwrap(
                  lynx_windowless_renderer_get_user_data(renderer));
          if (renderer_wrapper) {
            return renderer_wrapper->GetClipboardData();
          }
          return "";
        });
    lynx_windowless_renderer_bind_set_clipboard_data(
        windowless_renderer_,
        [](lynx_windowless_renderer_t* renderer, const char* data) -> void {
          std::shared_ptr<LynxWindowlessRenderer> renderer_wrapper =
              LynxWindowlessRenderer::Unwrap(
                  lynx_windowless_renderer_get_user_data(renderer));
          if (renderer_wrapper) {
            renderer_wrapper->SetClipboardData(data);
          }
        });
    lynx_windowless_renderer_bind_activate_system_cursor(
        windowless_renderer_,
        [](lynx_windowless_renderer_t* renderer, lynx_cursor_type_e cursor_type,
           const char* path) -> void {
          std::shared_ptr<LynxWindowlessRenderer> renderer_wrapper =
              LynxWindowlessRenderer::Unwrap(
                  lynx_windowless_renderer_get_user_data(renderer));
          if (renderer_wrapper) {
            renderer_wrapper->ActivateSystemCursor(cursor_type, path);
          }
        });
    lynx_windowless_renderer_bind_show_text_input(
        windowless_renderer_,
        [](lynx_windowless_renderer_t* renderer, bool show) -> void {
          std::shared_ptr<LynxWindowlessRenderer> renderer_wrapper =
              LynxWindowlessRenderer::Unwrap(
                  lynx_windowless_renderer_get_user_data(renderer));
          if (renderer_wrapper) {
            renderer_wrapper->ShowTextInput(show);
          }
        });
    lynx_windowless_renderer_bind_set_marked_text_rect(
        windowless_renderer_,
        [](lynx_windowless_renderer_t* renderer, float x, float y, float w,
           float h) -> void {
          std::shared_ptr<LynxWindowlessRenderer> renderer_wrapper =
              LynxWindowlessRenderer::Unwrap(
                  lynx_windowless_renderer_get_user_data(renderer));
          if (renderer_wrapper) {
            renderer_wrapper->SetMarkedTextRect(x, y, w, h);
          }
        });
    lynx_windowless_renderer_bind_set_editable_transform(
        windowless_renderer_,
        [](lynx_windowless_renderer_t* renderer,
           const float transform[16]) -> void {
          std::shared_ptr<LynxWindowlessRenderer> renderer_wrapper =
              LynxWindowlessRenderer::Unwrap(
                  lynx_windowless_renderer_get_user_data(renderer));
          if (renderer_wrapper) {
            renderer_wrapper->SetEditableTransform(transform);
          }
        });
  }

  virtual ~LynxWindowlessRenderer() {
    if (windowless_renderer_) {
      lynx_windowless_renderer_release(windowless_renderer_);
    }
  }

  /* ----- Callbacks for Software renderer type ----- */
  virtual bool OnSoftwarePresent(const void* allocation, size_t row_bytes,
                                 size_t height) {
    return false;
  }
  /* ----- Callbacks for Software renderer type ----- */

  /* ----- Callbacks for GL renderer type ----- */
  virtual bool OnGLMakeCurrent() { return false; }
  virtual bool OnGLClearCurrent() { return false; }
  virtual bool OnGLPresent() { return false; }
  virtual uint32_t OnGLCreateFBO(int width, int height) { return 0; }
  virtual void* OnGLProcResolver(const char* name) { return nullptr; }
  /* ----- Callbacks for GL renderer type ----- */

  /* ----- Callbacks for shared texture renderer type ----- */
  virtual bool OnAcceleratedPresent() { return false; }
  /* ----- Callbacks for shared texture renderer type ----- */

  virtual void OnPostTask(lynx_task_t task, uint64_t interval_nanoseconds) {}

  void RunTask(lynx_task_t task) {
    if (windowless_renderer_) {
      lynx_windowless_renderer_run_task(windowless_renderer_, task);
    }
  }

  bool GetAcceleratedPaintInfo(lynx_accelerated_paint_info_t* paint_info) {
    if (windowless_renderer_) {
      return lynx_windowless_renderer_get_accelerated_paint_info(
          windowless_renderer_, paint_info);
    }
    return false;
  }

  void SendPointerEvent(lynx_pointer_event_t* event) {
    if (windowless_renderer_) {
      lynx_windowless_renderer_send_pointer_event(windowless_renderer_, event);
    }
  }

  void SendKeyEvent(lynx_key_event_t* event) {
    if (windowless_renderer_) {
      lynx_windowless_renderer_send_key_event(windowless_renderer_, event);
    }
  }

  /* ----- Callbacks for platform function ----- */
  virtual const char* GetClipboardData() { return ""; }
  virtual void SetClipboardData(const char* data) {}
  virtual void ActivateSystemCursor(lynx_cursor_type_e cursor_type,
                                    const char* path) {}
  virtual void ShowTextInput(bool show) {}
  virtual void SetMarkedTextRect(float x, float y, float width, float height) {}
  virtual void SetEditableTransform(const float transform[16]) {}
  /* ----- Callbacks for platform function ----- */

  lynx_windowless_renderer_t* Impl() { return windowless_renderer_; }

  static std::shared_ptr<LynxWindowlessRenderer> Unwrap(void* user_data) {
    std::weak_ptr<LynxWindowlessRenderer>* weak_ptr =
        reinterpret_cast<std::weak_ptr<LynxWindowlessRenderer>*>(user_data);
    std::shared_ptr<LynxWindowlessRenderer> shared_renderer =
        weak_ptr ? weak_ptr->lock() : nullptr;
    return shared_renderer;
  }

 protected:
  lynx_windowless_renderer_type_e type_;
  lynx_windowless_renderer_t* windowless_renderer_ = nullptr;
};

}  // namespace pub
}  // namespace lynx

#endif  // PLATFORM_EMBEDDER_PUBLIC_LYNX_WINDOWLESS_RENDERER_H_
