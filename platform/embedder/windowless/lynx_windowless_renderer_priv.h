// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_WINDOWLESS_LYNX_WINDOWLESS_RENDERER_PRIV_H_
#define PLATFORM_EMBEDDER_WINDOWLESS_LYNX_WINDOWLESS_RENDERER_PRIV_H_

#include "base/include/fml/memory/ref_counted.h"
#include "clay/public/clay.h"
#include "platform/embedder/public/capi/lynx_windowless_renderer_capi.h"

namespace lynx {
namespace embedder {
class LynxWindowlessRenderer
    : public lynx::fml::RefCountedThreadSafe<LynxWindowlessRenderer> {
 public:
  lynx_windowless_renderer_type_e type = kRendererTypeSoftware;
  void* user_data = nullptr;
  void (*finalizer)(lynx_windowless_renderer_t*, void*) = nullptr;

  on_gl_make_current on_gl_make_current = nullptr;
  on_gl_clear_current on_gl_clear_current = nullptr;
  on_gl_present on_gl_present = nullptr;
  on_gl_create_fbo on_gl_create_fbo = nullptr;
  on_gl_proc_resolver on_gl_proc_resolver = nullptr;

  on_software_present on_software_present = nullptr;

  on_accelerated_present on_accelerated_present = nullptr;

  on_post_task on_post_task = nullptr;

  get_clipboard_data get_clipboard_data = nullptr;
  set_clipboard_data set_clipboard_data = nullptr;
  activate_system_cursor activate_system_cursor = nullptr;
  show_text_input show_text_input = nullptr;
  update_caret_position update_caret_position = nullptr;
  set_cursor_position set_cursor_position = nullptr;
  set_marked_text_rect set_marked_text_rect = nullptr;
  set_editable_transform set_editable_transform = nullptr;

  std::function<void(lynx_task_t task)> run_task = nullptr;
  std::function<void(lynx_pointer_event_t* event)> send_pointer_event = nullptr;
  std::function<void(lynx_key_event_t* event)> send_key_event = nullptr;

  ~LynxWindowlessRenderer();

  void PostTask(lynx_task_t task, uint64_t interval_nanoseconds);

  ClayHeadlessRendererConfig* GetRendererConfig();

  void OnSharedImageFrameAvailable();

  bool GetAcceleratedPaintInfo(lynx_accelerated_paint_info_t* paint_info);

 private:
  ClayHeadlessRendererConfig renderer_config_ = {};
  ClaySharedImageSinkRef sink_ref_ = nullptr;
};
}  // namespace embedder
}  // namespace lynx

#endif  // PLATFORM_EMBEDDER_WINDOWLESS_LYNX_WINDOWLESS_RENDERER_PRIV_H_
