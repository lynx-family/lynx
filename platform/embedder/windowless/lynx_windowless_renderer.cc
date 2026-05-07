// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/log/logging.h"
#include "build/build_config.h"
#include "platform/embedder/windowless/lynx_windowless_renderer_priv.h"
#include "platform/embedder/windowless/windowless_ui_task_runner_delegate.h"

LYNX_EXTERN_C lynx_windowless_renderer_t*
lynx_windowless_renderer_create_with_finalizer(
    lynx_windowless_renderer_type_e type, void* user_data,
    void (*finalizer)(lynx_windowless_renderer_t*, void*)) {
  lynx::embedder::LynxWindowlessRenderer* renderer =
      new lynx::embedder::LynxWindowlessRenderer;
  renderer->type = type;
  renderer->user_data = user_data;
  renderer->finalizer = finalizer;
  // The ref count has been initialized to 1, there is no need to call AddRef.
  return reinterpret_cast<lynx_windowless_renderer_t*>(renderer);
}

LYNX_EXTERN_C void* lynx_windowless_renderer_get_user_data(
    lynx_windowless_renderer_t* renderer) {
  return reinterpret_cast<lynx::embedder::LynxWindowlessRenderer*>(renderer)
      ->user_data;
}

LYNX_EXTERN_C void lynx_windowless_renderer_bind_on_gl_make_current(
    lynx_windowless_renderer_t* renderer,
    on_gl_make_current on_gl_make_current) {
  reinterpret_cast<lynx::embedder::LynxWindowlessRenderer*>(renderer)
      ->on_gl_make_current = on_gl_make_current;
}

LYNX_EXTERN_C void lynx_windowless_renderer_bind_on_gl_clear_current(
    lynx_windowless_renderer_t* renderer,
    on_gl_clear_current on_gl_clear_current) {
  reinterpret_cast<lynx::embedder::LynxWindowlessRenderer*>(renderer)
      ->on_gl_clear_current = on_gl_clear_current;
}

LYNX_EXTERN_C void lynx_windowless_renderer_bind_on_gl_present(
    lynx_windowless_renderer_t* renderer, on_gl_present on_gl_present) {
  reinterpret_cast<lynx::embedder::LynxWindowlessRenderer*>(renderer)
      ->on_gl_present = on_gl_present;
}

LYNX_EXTERN_C void lynx_windowless_renderer_bind_on_gl_create_fbo(
    lynx_windowless_renderer_t* renderer, on_gl_create_fbo on_gl_create_fbo) {
  reinterpret_cast<lynx::embedder::LynxWindowlessRenderer*>(renderer)
      ->on_gl_create_fbo = on_gl_create_fbo;
}

LYNX_EXTERN_C void lynx_windowless_renderer_bind_on_gl_proc_resolver(
    lynx_windowless_renderer_t* renderer,
    on_gl_proc_resolver on_gl_proc_resolver) {
  reinterpret_cast<lynx::embedder::LynxWindowlessRenderer*>(renderer)
      ->on_gl_proc_resolver = on_gl_proc_resolver;
}

LYNX_EXTERN_C void lynx_windowless_renderer_bind_on_software_present(
    lynx_windowless_renderer_t* renderer,
    on_software_present on_software_present) {
  reinterpret_cast<lynx::embedder::LynxWindowlessRenderer*>(renderer)
      ->on_software_present = on_software_present;
}

LYNX_EXTERN_C void lynx_windowless_renderer_bind_on_accelerated_present(
    lynx_windowless_renderer_t* renderer,
    on_accelerated_present on_accelerated_present) {
  reinterpret_cast<lynx::embedder::LynxWindowlessRenderer*>(renderer)
      ->on_accelerated_present = on_accelerated_present;
}

LYNX_EXTERN_C bool lynx_windowless_renderer_get_accelerated_paint_info(
    lynx_windowless_renderer_t* renderer,
    lynx_accelerated_paint_info_t* paint_info) {
  return reinterpret_cast<lynx::embedder::LynxWindowlessRenderer*>(renderer)
      ->GetAcceleratedPaintInfo(paint_info);
}

LYNX_EXTERN_C void lynx_windowless_renderer_bind_on_post_task(
    lynx_windowless_renderer_t* renderer, on_post_task on_post_task) {
  reinterpret_cast<lynx::embedder::LynxWindowlessRenderer*>(renderer)
      ->on_post_task = on_post_task;
}

LYNX_EXTERN_C void lynx_windowless_renderer_run_task(
    lynx_windowless_renderer_t* renderer, lynx_task_t task) {
  if (reinterpret_cast<lynx::embedder::LynxWindowlessRenderer*>(renderer)
          ->run_task) {
    reinterpret_cast<lynx::embedder::LynxWindowlessRenderer*>(renderer)
        ->run_task(task);
  }
}

LYNX_EXTERN_C void lynx_windowless_renderer_send_pointer_event(
    lynx_windowless_renderer_t* renderer, lynx_pointer_event_t* event) {
  if (reinterpret_cast<lynx::embedder::LynxWindowlessRenderer*>(renderer)
          ->send_pointer_event) {
    reinterpret_cast<lynx::embedder::LynxWindowlessRenderer*>(renderer)
        ->send_pointer_event(event);
  }
}

LYNX_EXTERN_C void lynx_windowless_renderer_send_key_event(
    lynx_windowless_renderer_t* renderer, lynx_key_event_t* event) {
  if (reinterpret_cast<lynx::embedder::LynxWindowlessRenderer*>(renderer)
          ->send_key_event) {
    reinterpret_cast<lynx::embedder::LynxWindowlessRenderer*>(renderer)
        ->send_key_event(event);
  }
}

LYNX_EXTERN_C void lynx_windowless_renderer_bind_get_clipboard_data(
    lynx_windowless_renderer_t* renderer,
    get_clipboard_data get_clipboard_data) {
  reinterpret_cast<lynx::embedder::LynxWindowlessRenderer*>(renderer)
      ->get_clipboard_data = get_clipboard_data;
}

LYNX_EXTERN_C void lynx_windowless_renderer_bind_set_clipboard_data(
    lynx_windowless_renderer_t* renderer,
    set_clipboard_data set_clipboard_data) {
  reinterpret_cast<lynx::embedder::LynxWindowlessRenderer*>(renderer)
      ->set_clipboard_data = set_clipboard_data;
}

LYNX_EXTERN_C void lynx_windowless_renderer_bind_activate_system_cursor(
    lynx_windowless_renderer_t* renderer,
    activate_system_cursor activate_system_cursor) {
  reinterpret_cast<lynx::embedder::LynxWindowlessRenderer*>(renderer)
      ->activate_system_cursor = activate_system_cursor;
}

LYNX_EXTERN_C void lynx_windowless_renderer_bind_show_text_input(
    lynx_windowless_renderer_t* renderer, show_text_input show_text_input) {
  reinterpret_cast<lynx::embedder::LynxWindowlessRenderer*>(renderer)
      ->show_text_input = show_text_input;
}

LYNX_EXTERN_C void lynx_windowless_renderer_bind_update_caret_position(
    lynx_windowless_renderer_t* renderer,
    update_caret_position update_caret_position) {
  reinterpret_cast<lynx::embedder::LynxWindowlessRenderer*>(renderer)
      ->update_caret_position = update_caret_position;
}

LYNX_EXTERN_C void lynx_windowless_renderer_bind_set_cursor_position(
    lynx_windowless_renderer_t* renderer,
    set_cursor_position set_cursor_position) {
  reinterpret_cast<lynx::embedder::LynxWindowlessRenderer*>(renderer)
      ->set_cursor_position = set_cursor_position;
}

LYNX_EXTERN_C void lynx_windowless_renderer_bind_set_marked_text_rect(
    lynx_windowless_renderer_t* renderer,
    set_marked_text_rect set_marked_text_rect) {
  reinterpret_cast<lynx::embedder::LynxWindowlessRenderer*>(renderer)
      ->set_marked_text_rect = set_marked_text_rect;
}

LYNX_EXTERN_C void lynx_windowless_renderer_bind_set_editable_transform(
    lynx_windowless_renderer_t* renderer,
    set_editable_transform set_editable_transform) {
  reinterpret_cast<lynx::embedder::LynxWindowlessRenderer*>(renderer)
      ->set_editable_transform = set_editable_transform;
}

LYNX_EXTERN_C bool lynx_windowless_set_global_ui_task_runner(
    const lynx_windowless_ui_task_runner_config_t* config) {
  LOGD("[lynx_windowless_set_global_ui_task_runner] Called");
  bool result = lynx::embedder::SetGlobalUITaskRunnerDelegate(config);
  LOGD("[lynx_windowless_set_global_ui_task_runner] Returning");
  return result;
}

LYNX_EXTERN_C bool lynx_windowless_run_ui_task(lynx_task_t task) {
  LOGD("[lynx_windowless_run_ui_task] Called");
  lynx::embedder::WindowlessUITaskRunnerDelegate* delegate =
      lynx::embedder::GetGlobalUITaskRunnerDelegate();
  if (!delegate) {
    LOGW("[lynx_windowless_run_ui_task] delegate is null");
    return false;
  }
  bool result = delegate->RunTask(task.task);
  LOGD("[lynx_windowless_run_ui_task] Returning");
  return result;
}

LYNX_EXTERN_C void lynx_windowless_renderer_release(
    lynx_windowless_renderer_t* renderer) {
  // Unref the windowless renderer.
  reinterpret_cast<lynx::embedder::LynxWindowlessRenderer*>(renderer)
      ->Release();
}

namespace lynx {
namespace embedder {

LynxWindowlessRenderer::~LynxWindowlessRenderer() {
  if (sink_ref_) {
    ClaySharedImageSinkSetFrameAvailableCallback(sink_ref_, nullptr, nullptr);
    ClayReleaseSharedImageSink(sink_ref_);
    sink_ref_ = nullptr;
  }
  if (finalizer) {
    finalizer(reinterpret_cast<lynx_windowless_renderer_t*>(this), user_data);
  }
}

void LynxWindowlessRenderer::PostTask(lynx_task_t task,
                                      uint64_t interval_nanoseconds) {
  if (on_post_task) {
    on_post_task(reinterpret_cast<lynx_windowless_renderer_t*>(this), task,
                 interval_nanoseconds);
  }
}

ClayHeadlessRendererConfig* LynxWindowlessRenderer::GetRendererConfig() {
  if (type == kRendererTypeSoftware) {
    renderer_config_.type = kClayRendererTypeSoftware;
    renderer_config_.software.struct_size = sizeof(ClaySoftwareRendererConfig);
    renderer_config_.software.present_callback =
        [](void* user_data, const void* allocation, size_t raw_bytes,
           size_t height) -> bool {
      LynxWindowlessRenderer* renderer =
          reinterpret_cast<LynxWindowlessRenderer*>(user_data);
      if (renderer && renderer->on_software_present) {
        return renderer->on_software_present(
            reinterpret_cast<lynx_windowless_renderer_t*>(renderer), allocation,
            raw_bytes, height);
      }
      return false;
    };
  } else if (type == kRendererTypeGL || type == kRendererTypeGLDirect) {
    renderer_config_.type = kClayRendererTypeHostGL;
    renderer_config_.opengl.struct_size = sizeof(ClayOpenGLRendererConfig);
    renderer_config_.opengl.make_current = [](void* user_data) -> bool {
      LynxWindowlessRenderer* renderer =
          reinterpret_cast<LynxWindowlessRenderer*>(user_data);
      if (renderer && renderer->on_gl_make_current) {
        return renderer->on_gl_make_current(
            reinterpret_cast<lynx_windowless_renderer_t*>(renderer));
      }
      return false;
    };

    renderer_config_.opengl.clear_current = [](void* user_data) -> bool {
      LynxWindowlessRenderer* renderer =
          reinterpret_cast<LynxWindowlessRenderer*>(user_data);
      if (renderer && renderer->on_gl_clear_current) {
        return renderer->on_gl_clear_current(
            reinterpret_cast<lynx_windowless_renderer_t*>(renderer));
      }
      return false;
    };

    renderer_config_.opengl.present = [](void* user_data) -> bool {
      LynxWindowlessRenderer* renderer =
          reinterpret_cast<LynxWindowlessRenderer*>(user_data);
      if (renderer && renderer->on_gl_present) {
        return renderer->on_gl_present(
            reinterpret_cast<lynx_windowless_renderer_t*>(renderer));
      }
      return false;
    };

    renderer_config_.opengl.fbo_callback =
        [](void* user_data, const ClayFrameInfo* info) -> uint32_t {
      LynxWindowlessRenderer* renderer =
          reinterpret_cast<LynxWindowlessRenderer*>(user_data);
      if (renderer && renderer->on_gl_create_fbo) {
        return renderer->on_gl_create_fbo(
            reinterpret_cast<lynx_windowless_renderer_t*>(renderer),
            info->width, info->height);
      }
      return 0;
    };

    renderer_config_.opengl.gl_proc_resolver = [](void* user_data,
                                                  const char* what) -> void* {
      LynxWindowlessRenderer* renderer =
          reinterpret_cast<LynxWindowlessRenderer*>(user_data);
      if (renderer && renderer->on_gl_proc_resolver) {
        return renderer->on_gl_proc_resolver(
            reinterpret_cast<lynx_windowless_renderer_t*>(renderer), what);
      }
      return nullptr;
    };
    renderer_config_.opengl.enable_shared_image_sink = type == kRendererTypeGL;
    renderer_config_.opengl.shared_image_sink_buffer_mode =
        kClaySharedImageSinkBufferModeDoubleBuffer;
  } else if (type == kRendererTypeAccelerated) {
    if (!sink_ref_) {
      ClaySharedImageBackingType backing_type;
#if defined(OS_MACOSX)
      backing_type = kClaySharedImageBackingTypeIOSurface;
#elif defined(OS_WIN)
      backing_type = kClaySharedImageBackingTypeD3DTexture;
#elif defined(OS_LINUX)
      backing_type = kClaySharedImageBackingTypeShmImage;
#elif defined(OS_HARMONY)
      backing_type = kClaySharedImageBackingTypeNativeImage;
#else
      abort();
#endif

#if defined(OS_WIN)
      ClaySharedImageBackingPixelFormat pixel_format =
          kClaySharedImageBackingPixelFormatRGBA8;
#else
      ClaySharedImageBackingPixelFormat pixel_format =
          kClaySharedImageBackingPixelFormatNative8888;
#endif
      sink_ref_ =
          ClayCreateSharedImageSink(kClaySharedImageSinkBufferModeDoubleBuffer,
                                    backing_type, pixel_format);
      ClaySharedImageSinkSetFrameAvailableCallback(
          sink_ref_,
          [](void* user_data) {
            static_cast<LynxWindowlessRenderer*>(user_data)
                ->OnSharedImageFrameAvailable();
          },
          this);
    }
#if defined(OS_MACOSX)
    renderer_config_.type = kClayRendererTypeMetal;
#elif defined(OS_WIN)
    renderer_config_.type = kClayRendererTypeOpenGL;
#elif defined(OS_LINUX)
    renderer_config_.type = kClayRendererTypeOpenGL;
#elif defined(OS_HARMONY)
    renderer_config_.type = kClayRendererTypeOpenGL;
#endif
    renderer_config_.hardware.struct_size = sizeof(ClayHardwareRendererConfig);
    renderer_config_.hardware.sink_ref = sink_ref_;
    renderer_config_.hardware.disable_partial_repaint = true;
  }
  return &renderer_config_;
}

void LynxWindowlessRenderer::OnSharedImageFrameAvailable() {
  if (on_accelerated_present) {
    on_accelerated_present(reinterpret_cast<lynx_windowless_renderer_t*>(this));
  }
}

bool LynxWindowlessRenderer::GetAcceleratedPaintInfo(
    lynx_accelerated_paint_info_t* paint_info) {
  if (sink_ref_) {
    ClaySharedImageRef shared_image = nullptr;
    bool r = ClaySharedImageSinkUpdateFront(sink_ref_, nullptr, &shared_image);
    if (r && shared_image) {
      paint_info->struct_size = sizeof(lynx_accelerated_paint_info_t);
      ClaySize size;
      ClaySharedImageGetSize(shared_image, &size);
      paint_info->width = size.width;
      paint_info->height = size.height;
      ClaySharedImageBackingPixelFormat format =
          kClaySharedImageBackingPixelFormatNative8888;
      ClaySharedImageGetBacking(shared_image, nullptr, &format,
                                &paint_info->shared_texture_handle);
      if (format == kClaySharedImageBackingPixelFormatNative8888) {
        paint_info->color_type = kLynxColorTypeBGRA_8888;
      } else {
        paint_info->color_type = kLynxColorTypeRGBA_8888;
      }
      return true;
    }
  }
  return false;
}

}  // namespace embedder
}  // namespace lynx
