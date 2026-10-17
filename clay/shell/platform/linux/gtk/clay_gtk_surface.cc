// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0.
#include "clay/shell/platform/linux/gtk/clay_gtk_surface.h"

#include <epoxy/gl.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
namespace clay {
GtkSurface::GtkSurface(bool gl) : gl_(gl) {
  trace_frames_ = g_getenv("LYNX_GTK_FRAME_TRACE") != nullptr;
  widget_ = gl ? gtk_gl_area_new() : gtk_picture_new();
  g_object_ref_sink(widget_);
  gtk_widget_set_hexpand(widget_, TRUE);
  gtk_widget_set_vexpand(widget_, TRUE);
  gtk_widget_set_focusable(widget_, TRUE);
  if (gl) {
#ifdef GDK_VERSION_4_22
#if GDK_VERSION_MIN_REQUIRED >= GDK_VERSION_4_22
    gtk_gl_area_set_allowed_apis(GTK_GL_AREA(widget_), GDK_GL_API_GL);
#else
    gtk_gl_area_set_use_es(GTK_GL_AREA(widget_), FALSE);
#endif
#else
    gtk_gl_area_set_use_es(GTK_GL_AREA(widget_), FALSE);
#endif
    gtk_gl_area_set_required_version(GTK_GL_AREA(widget_), 3, 3);
    gtk_gl_area_set_auto_render(GTK_GL_AREA(widget_), FALSE);
    g_signal_connect(widget_, "realize",
                     G_CALLBACK(+[](GtkWidget*, gpointer data) {
                       auto* self = static_cast<GtkSurface*>(data);
                       if (self->context_) self->ValidateGLConsumer();
                     }),
                     this);
    g_signal_connect(widget_, "unrealize",
                     G_CALLBACK(+[](GtkWidget*, gpointer data) {
                       static_cast<GtkSurface*>(data)->UnrealizeGL();
                     }),
                     this);
    g_signal_connect(
        widget_, "render",
        G_CALLBACK(+[](GtkGLArea*, GdkGLContext*, gpointer data) -> gboolean {
          return static_cast<GtkSurface*>(data)->DrawGL();
        }),
        this);
  } else {
    gtk_picture_set_can_shrink(GTK_PICTURE(widget_), TRUE);
#ifdef GDK_VERSION_4_22
#if GDK_VERSION_MIN_REQUIRED >= GDK_VERSION_4_22
    gtk_picture_set_content_fit(GTK_PICTURE(widget_), GTK_CONTENT_FIT_FILL);
#else
    gtk_picture_set_keep_aspect_ratio(GTK_PICTURE(widget_), FALSE);
#endif
#else
    gtk_picture_set_keep_aspect_ratio(GTK_PICTURE(widget_), FALSE);
#endif
  }
}
double GtkSurface::DeviceScale() const {
#ifdef GDK_VERSION_4_22
#if GDK_VERSION_MIN_REQUIRED >= GDK_VERSION_4_22
  if (auto* native = gtk_widget_get_native(widget_)) {
    if (auto* surface = gtk_native_get_surface(native))
      return gdk_surface_get_scale(surface);
  }
#endif
#endif
  return gtk_widget_get_scale_factor(widget_);
}
bool GtkSurface::InitializeGL() {
  gtk_gl_area_make_current(GTK_GL_AREA(widget_));
  if (gtk_gl_area_get_error(GTK_GL_AREA(widget_))) return false;
  GError* error = nullptr;
  // Rendering into textures must survive removal/destruction of the original
  // window. A display context has no window surface/drawable dependency.
  context_ =
      gdk_display_create_gl_context(gtk_widget_get_display(widget_), &error);
  if (context_) {
    gdk_gl_context_set_allowed_apis(context_, GDK_GL_API_GL);
    gdk_gl_context_set_required_version(context_, 3, 3);
  }
  bool ok = context_ && gdk_gl_context_realize(context_, &error) &&
            gdk_gl_context_is_shared(
                context_, gtk_gl_area_get_context(GTK_GL_AREA(widget_)));
  if (error) {
    std::fprintf(stderr, "GTK4 GL init: %s\n", error->message);
    g_error_free(error);
  }
  std::fprintf(stderr, "GTK4 GL sharing: %s\n", ok ? "PASS" : "FAIL");
  if (!ok && context_) {
    g_object_unref(context_);
    context_ = nullptr;
  }
  return ok;
}
bool GtkSurface::ValidateGLConsumer() {
  gtk_gl_area_make_current(GTK_GL_AREA(widget_));
  if (gtk_gl_area_get_error(GTK_GL_AREA(widget_))) return false;
  if (!gdk_gl_context_is_shared(
          context_, gtk_gl_area_get_context(GTK_GL_AREA(widget_)))) {
    GError* error =
        g_error_new_literal(GDK_GL_ERROR, GDK_GL_ERROR_NOT_AVAILABLE,
                            "GTK GL context sharing was lost");
    gtk_gl_area_set_error(GTK_GL_AREA(widget_), error);
    g_error_free(error);
    g_printerr("GTK4 GL reattach sharing FAIL\n");
    return false;
  }
  gtk_gl_area_queue_render(GTK_GL_AREA(widget_));
  g_print("GTK4 GL reattach sharing PASS\n");
  return true;
}
bool GtkSurface::RecreateGL() {
  if (!gl_ || !gtk_widget_get_realized(widget_)) return false;
  UnrealizeGL();  // Retire the consumer's context-local framebuffer first.
  bool retired = true;
  if (context_) {
    if (!MakeCurrent()) return false;
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& slot : slots_) {
      if (slot.produced) glDeleteSync(slot.produced);
      if (slot.consumed) glDeleteSync(slot.consumed);
      glDeleteFramebuffers(1, &slot.fbo);
      glDeleteRenderbuffers(1, &slot.stencil);
      glDeleteTextures(1, &slot.texture);
      // Deletion must retire the names before a new producer is created.
      retired = !glIsTexture(slot.texture) && !glIsFramebuffer(slot.fbo) &&
                !glIsRenderbuffer(slot.stencil) && retired;
      slot = Slot{};
    }
    writing_ = ready_ = front_ = -1;
    gdk_gl_context_clear_current();
    g_object_unref(context_);
    context_ = nullptr;
  }
  logged_ = false;
  if (!retired) {
    g_printerr("GTK4 GL old producer resources retirement FAIL\n");
    return false;
  }
  g_print("GTK4 GL old producer resources retired PASS\n");
  return InitializeGL();
}
void GtkSurface::UnrealizeGL() {
  // This handler runs before GtkGLArea releases its context. FBO names are
  // context-local: delete the old consumer FBO, retain shared producer
  // textures.
  gtk_gl_area_make_current(GTK_GL_AREA(widget_));
  std::lock_guard<std::mutex> lock(mutex_);
  if (!gtk_gl_area_get_error(GTK_GL_AREA(widget_)))
    glDeleteFramebuffers(1, &read_fbo_);
  read_fbo_ = 0;
  gdk_gl_context_clear_current();
}
GtkSurface::~GtkSurface() {
  if (update_source_) {
    g_source_destroy(update_source_);
    g_source_unref(update_source_);
  }
  g_signal_handlers_disconnect_by_data(widget_, this);
  // The adapter releases the Lynx view first, joining engine/raster work.
  if (context_) {
    gdk_gl_context_make_current(context_);
    for (auto& slot : slots_) {
      if (slot.produced) glDeleteSync(slot.produced);
      if (slot.consumed) glDeleteSync(slot.consumed);
      glDeleteFramebuffers(1, &slot.fbo);
      glDeleteRenderbuffers(1, &slot.stencil);
      glDeleteTextures(1, &slot.texture);
    }
    gdk_gl_context_clear_current();
    g_object_unref(context_);
  }
  if (gl_ && gtk_widget_get_realized(widget_)) {
    gtk_gl_area_make_current(GTK_GL_AREA(widget_));
    if (!gtk_gl_area_get_error(GTK_GL_AREA(widget_)))
      glDeleteFramebuffers(1, &read_fbo_);
    gdk_gl_context_clear_current();
  }
  g_object_unref(widget_);
}
bool GtkSurface::MakeCurrent() {
  if (!context_) return false;
  if (injected_failure_.exchange(false)) {
    ReportGLFailure();
    return false;
  }
  gdk_gl_context_make_current(context_);
  if (gdk_gl_context_get_current() != context_) {
    ReportGLFailure();
    return false;
  }
  return true;
}
void GtkSurface::SetRecoveryCallback(std::function<bool()> callback) {
  std::lock_guard<std::mutex> lock(mutex_);
  recovery_callback_ = std::move(callback);
}
void GtkSurface::ReportGLFailure() {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!recovery_callback_) return;
    if (recovering_) {
      failure_during_recovery_ = true;
      return;
    }
    if (recovery_pending_) return;
    ++consecutive_failures_;
    recovery_pending_ = true;
  }
  QueueUpdate();
}
bool GtkSurface::ClearCurrent() {
  gdk_gl_context_clear_current();
  return true;
}
unsigned GtkSurface::CreateFBO(int width, int height) {
  if (width <= 0 || height <= 0 || width > 8192 || height > 8192) return 0;
  std::lock_guard<std::mutex> lock(mutex_);
  if (writing_ < 0) {
    for (int i = 0; i < 3; ++i)
      if (slots_[i].state == State::kFree) {
        writing_ = i;
        break;
      }
  }
  if (writing_ < 0) return 0;
  auto& slot = slots_[writing_];
  slot.state = State::kWriting;
  peak_slots_ = std::max(
      peak_slots_, static_cast<unsigned>(std::count_if(
                       slots_.begin(), slots_.end(), [](const Slot& item) {
                         return item.state != State::kFree;
                       })));
  // Queue a GPU-side wait before writing storage sampled by GTK's context.
  if (slot.consumed) {
    glWaitSync(slot.consumed, 0, GL_TIMEOUT_IGNORED);
    glDeleteSync(slot.consumed);
    slot.consumed = nullptr;
  }
  if (slot.produced) {
    glDeleteSync(slot.produced);
    slot.produced = nullptr;
  }
  if (!slot.texture) {
    glGenTextures(1, &slot.texture);
    glGenFramebuffers(1, &slot.fbo);
    glGenRenderbuffers(1, &slot.stencil);
  }
  // Skia caches GL bindings across frames. FBO allocation must not corrupt
  // those bindings (notably the glyph atlas on the active texture unit).
  GLint texture_binding, renderbuffer_binding, draw_binding, read_binding;
  glGetIntegerv(GL_TEXTURE_BINDING_2D, &texture_binding);
  glGetIntegerv(GL_RENDERBUFFER_BINDING, &renderbuffer_binding);
  glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &draw_binding);
  glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &read_binding);
  glBindTexture(GL_TEXTURE_2D, slot.texture);
  if (slot.width != width || slot.height != height) {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindRenderbuffer(GL_RENDERBUFFER, slot.stencil);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    slot.width = width;
    slot.height = height;
  }
  glBindFramebuffer(GL_FRAMEBUFFER, slot.fbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         slot.texture, 0);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                            GL_RENDERBUFFER, slot.stencil);
  const bool complete =
      glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
  glBindTexture(GL_TEXTURE_2D, texture_binding);
  glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer_binding);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, draw_binding);
  glBindFramebuffer(GL_READ_FRAMEBUFFER, read_binding);
  if (!complete) {
    // Failed allocation must be retried rather than caching invalid storage.
    slot.width = slot.height = 0;
    slot.state = State::kFree;
    writing_ = -1;
  }
  return complete ? slot.fbo : 0;
}
bool GtkSurface::PresentGL() {
  if (writing_ < 0) return false;
  {
    std::unique_lock<std::mutex> lock(mutex_);
    auto& slot = slots_[writing_];
    slot.produced = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    if (!slot.produced) {
      lock.unlock();
      ReportGLFailure();
      return false;
    }
    // The consumer can wait only after this context submits the fence.
    glFlush();
    if (ready_ >= 0) {
      slots_[ready_].state = State::kFree;
      ++dropped_frames_;
    }
    ready_ = writing_;
    writing_ = -1;
    slots_[ready_].state = State::kReady;
    ++presented_frames_;
    slots_[ready_].frame_id = presented_frames_;
    slots_[ready_].ready_us = g_get_monotonic_time();
    if (trace_frames_ && presented_frames_ <= 1000)
      g_print(
          "GTK4_FRAME produced=%lu ready_us=%ld peak_slots=%u dropped=%lu\n",
          presented_frames_, slots_[ready_].ready_us, peak_slots_,
          dropped_frames_);
  }
  QueueUpdate();
  return true;
}
bool GtkSurface::DrawGL() {
  std::unique_lock<std::mutex> lock(mutex_);
  if (ready_ >= 0) {
    if (front_ >= 0) slots_[front_].state = State::kFree;
    front_ = ready_;
    ready_ = -1;
    slots_[front_].state = State::kFront;
  }
  if (front_ < 0) return TRUE;
  auto& frame = slots_[front_];
  if (frame.produced) {
    glWaitSync(frame.produced, 0, GL_TIMEOUT_IGNORED);
    glDeleteSync(frame.produced);
    frame.produced = nullptr;
  }
  GLint target = 0, previous_read = 0;
  glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &target);
  glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &previous_read);
  const bool scissor = glIsEnabled(GL_SCISSOR_TEST);
  if (!read_fbo_) glGenFramebuffers(1, &read_fbo_);
  glBindFramebuffer(GL_READ_FRAMEBUFFER, read_fbo_);
  glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                         GL_TEXTURE_2D, frame.texture, 0);
  GLint viewport[4];
  glGetIntegerv(GL_VIEWPORT, viewport);
  glDisable(GL_SCISSOR_TEST);
  glBlitFramebuffer(0, 0, frame.width, frame.height, 0, 0, viewport[2],
                    viewport[3], GL_COLOR_BUFFER_BIT, GL_LINEAR);
  // A retained front may be sampled repeatedly. Its latest fence covers all
  // earlier samples in this context; the producer waits before reusing storage.
  if (frame.consumed) glDeleteSync(frame.consumed);
  frame.consumed = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
  if (!frame.consumed) {
    // Preserve ownership even if a driver cannot create a sync object.
    glFinish();
    std::fprintf(stderr,
                 "GTK4 GL fence allocation failed; synchronous fallback\n");
  }
  glFlush();
  GLenum error = glGetError();
  glBindFramebuffer(GL_READ_FRAMEBUFFER, previous_read);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, target);
  if (scissor) glEnable(GL_SCISSOR_TEST);
  if (error) {
    std::fprintf(stderr, "GTK4 GL present error: %u\n", error);
    lock.unlock();
    ReportGLFailure();
    return TRUE;
  }
  consecutive_failures_ = 0;
  if (trace_frames_ && frame.frame_id != last_traced_frame_ &&
      frame.frame_id <= 1000) {
    const auto now = g_get_monotonic_time();
    last_traced_frame_ = frame.frame_id;
    g_print("GTK4_FRAME consumed=%lu host_us=%ld latency_us=%ld\n",
            frame.frame_id, now, now - frame.ready_us);
  }
  if (!logged_ && !error) {
    std::fprintf(stderr,
                 "GTK4 GL texture present PASS driver=%s cpu_readback=0 "
                 "sync=gpu_fences\n",
                 glGetString(GL_RENDERER));
    logged_ = true;
  }
  return TRUE;
}
bool GtkSurface::PresentSoftware(const void* pixels, size_t stride,
                                 size_t height) {
  if (!pixels || !stride || stride % 4 || stride / 4 > 8192 || !height ||
      height > 8192)
    return false;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    pixels_.resize(stride * height);
    std::memcpy(pixels_.data(), pixels, pixels_.size());
    stride_ = stride;
    height_ = height;
    ++presented_frames_;
  }
  QueueUpdate();
  return true;
}
uint64_t GtkSurface::FrameCountForTesting() {
  std::lock_guard<std::mutex> lock(mutex_);
  return presented_frames_;
}
void GtkSurface::QueueUpdate() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (update_pending_) return;
  update_pending_ = true;
  auto* weak = new std::weak_ptr<GtkSurface>(weak_from_this());
  auto* source = g_idle_source_new();
  // Native GTK children can keep redraw sources ready. Presenting a Lynx
  // frame is UI work, not optional idle work: use the host task priority so
  // the render surface keeps updating while native controls are visible.
  g_source_set_priority(source, G_PRIORITY_DEFAULT);
  g_source_set_callback(
      source,
      +[](gpointer data) -> gboolean {
        if (auto self = static_cast<std::weak_ptr<GtkSurface>*>(data)->lock())
          self->Update();
        return G_SOURCE_REMOVE;
      },
      weak,
      +[](gpointer data) {
        delete static_cast<std::weak_ptr<GtkSurface>*>(data);
      });
  // Attach rather than invoke: invocation may execute on the raster thread.
  update_source_ = source;
  g_source_attach(source, g_main_context_default());
}
void GtkSurface::Update() {
  std::unique_lock<std::mutex> lock(mutex_);
  update_pending_ = false;
  g_source_unref(update_source_);
  update_source_ = nullptr;
  if (recovery_pending_) {
    recovery_pending_ = false;
    recovering_ = true;
    failure_during_recovery_ = false;
    auto recover = recovery_callback_;
    const bool retry_allowed = consecutive_failures_ <= 3;
    // Recovery waits for the raster thread, which also needs this mutex.
    lock.unlock();
    const bool success = retry_allowed && recover && recover();
    lock.lock();
    recovering_ = false;
    if (!success) {
      recovery_callback_ = nullptr;  // No unbounded retries on a lost driver.
      GError* error =
          g_error_new_literal(GDK_GL_ERROR, GDK_GL_ERROR_NOT_AVAILABLE,
                              "Graphics context recovery failed");
      gtk_gl_area_set_error(GTK_GL_AREA(widget_), error);
      g_error_free(error);
      g_printerr("GTK4 GL automatic recovery FAIL\n");
      return;
    }
    g_print("GTK4 GL recoverable failure handled PASS\n");
    if (failure_during_recovery_) {
      // A first frame can fail while ResumeGLSurface is still returning.
      // Preserve that failure instead of leaving the renderer stalled.
      lock.unlock();
      ReportGLFailure();
      return;
    }
  }
  if (gl_) {
    gtk_gl_area_queue_render(GTK_GL_AREA(widget_));
    return;
  }
  if (pixels_.empty()) return;
  GBytes* bytes = g_bytes_new(pixels_.data(), pixels_.size());
  auto* texture = gdk_memory_texture_new(
      stride_ / 4, height_, GDK_MEMORY_B8G8R8A8_PREMULTIPLIED, bytes, stride_);
  gtk_picture_set_paintable(GTK_PICTURE(widget_), GDK_PAINTABLE(texture));
  g_object_unref(texture);
  g_bytes_unref(bytes);
  if (!logged_) {
    std::fprintf(stderr, "GTK4 software present PASS\n");
    logged_ = true;
  }
}
}  // namespace clay
