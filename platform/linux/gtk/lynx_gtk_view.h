// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0.
#ifndef PLATFORM_LINUX_GTK_LYNX_GTK_VIEW_H_
#define PLATFORM_LINUX_GTK_LYNX_GTK_VIEW_H_
#include <gtk/gtk.h>

#include <memory>

#include "platform/embedder/public/lynx_view.h"
namespace lynx::embedder {
class LynxUIRendererGtk;
}
namespace lynx::gtk {
// GTK UI-thread adapter. Owns the engine and a strong widget reference.
// Borrowed widget()/lynx_view() handles are valid only for this View lifetime.
// Destroy or detach before destroying its GTK window. Do not manually unparent
// the rendering widget.
class View {
 public:
  // Register once on the application's GTK UI thread before creating a view.
  static bool InitializeTaskRunner();
  // UI thread. Keeps page/JS state; completes surface recreation
  // asynchronously.
  bool RecreateGLContext();
  bool InjectGLFailureForTesting();
  View(GtkWidget* parent_box, bool gl, pub::LynxView::Builder& builder);
  ~View();
  View(const View&) = delete;
  View& operator=(const View&) = delete;
  // Views may move between GtkBoxes on the same GDK display, including an
  // unrealized parent. GL retains the texture-producing display context.
  bool Attach(GtkWidget* parent_box);
  bool Detach();
  GtkWidget* widget() const;
  pub::LynxView* lynx_view() const { return view_.get(); }
  bool uses_gl() const;
  uint64_t FrameCountForTesting() const;
  // Test-only normalized pointer path, after the native GTK adapter.
  bool SendPointerEventForTesting(lynx_pointer_event_t* event);

 private:
  friend class lynx::embedder::LynxUIRendererGtk;
  class Renderer;
  std::shared_ptr<Renderer> renderer_;
  std::unique_ptr<pub::LynxView> view_;
};
}  // namespace lynx::gtk
#endif
