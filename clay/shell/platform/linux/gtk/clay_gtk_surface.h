// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0.
#ifndef CLAY_SHELL_PLATFORM_LINUX_GTK_CLAY_GTK_SURFACE_H_
#define CLAY_SHELL_PLATFORM_LINUX_GTK_CLAY_GTK_SURFACE_H_
#include <epoxy/gl.h>
#include <gtk/gtk.h>

#include <array>
#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>
namespace clay {
// UI owns the widget; the engine must stop its raster thread before
// destruction. GPU frames use three bounded shared textures. No CPU readback is
// performed.
class GtkSurface : public std::enable_shared_from_this<GtkSurface> {
 public:
  explicit GtkSurface(bool gl);
  ~GtkSurface();
  GtkWidget* widget() const { return widget_; }
  double DeviceScale() const;
  bool InitializeGL();  // UI thread, after attaching/realizing the widget.
  // UI thread, only after the engine has stopped and released GPU resources.
  bool RecreateGL();
  void SetRecoveryCallback(std::function<bool()> callback);
  void InjectGLFailureForTesting() { injected_failure_ = true; }
  bool MakeCurrent();
  bool ClearCurrent();
  unsigned CreateFBO(int width, int height);
  bool PresentGL();
  bool PresentSoftware(const void* pixels, size_t stride, size_t height);
  void QueueUpdate();
  uint64_t FrameCountForTesting();

 private:
  enum class State { kFree, kWriting, kReady, kFront };
  struct Slot {
    unsigned texture = 0, fbo = 0, stencil = 0;
    int width = 0, height = 0;
    State state = State::kFree;
    GLsync produced = nullptr;
    GLsync consumed = nullptr;
    uint64_t frame_id = 0;
    gint64 ready_us = 0;
  };
  bool DrawGL();
  bool ValidateGLConsumer();
  void UnrealizeGL();
  void Update();
  void ReportGLFailure();
  GtkWidget* widget_;
  bool gl_;
  GdkGLContext* context_ = nullptr;
  std::mutex mutex_;
  std::array<Slot, 3> slots_;
  int writing_ = -1, ready_ = -1, front_ = -1;
  unsigned read_fbo_ = 0;
  std::vector<unsigned char> pixels_;
  size_t stride_ = 0, height_ = 0;
  bool update_pending_ = false;
  GSource* update_source_ = nullptr;
  bool logged_ = false;
  uint64_t presented_frames_ = 0;
  std::function<bool()> recovery_callback_;
  bool recovery_pending_ = false, recovering_ = false;
  bool failure_during_recovery_ = false;
  unsigned consecutive_failures_ = 0;
  std::atomic<bool> injected_failure_{false};
  bool trace_frames_ = false;
  unsigned peak_slots_ = 0;
  uint64_t dropped_frames_ = 0, last_traced_frame_ = 0;
};
}  // namespace clay
#endif
