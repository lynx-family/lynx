// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0.
#ifndef PLATFORM_LINUX_GTK_LYNX_GTK_VIEW_CAPI_H_
#define PLATFORM_LINUX_GTK_LYNX_GTK_VIEW_CAPI_H_
#include <gtk/gtk.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "capi/lynx_export.h"

LYNX_EXTERN_C_BEGIN
// View calls run on the GTK UI thread, after gtk_init(). The host owns the
// default GLib main context. Destroy or detach views before destroying their
// containing GTK windows.
typedef struct lynx_gtk_view_t lynx_gtk_view_t;
struct lynx_view_t;
// Initialize once on the owning GLib/GTK UI thread, before any Lynx engine.
// gtk_init_check() may fail for a display-free windowless app; this only binds
// the UI task runner and checks the GTK runtime version, without opening a
// view.
LYNX_CAPI_EXPORT bool lynx_gtk_initialize(void);
typedef struct lynx_gtk_view_options_t {
  size_t struct_size;
  float width;
  float height;
  const char* icu_data_path;  // Copied during create; nonempty path required.
  bool prefer_gl;  // Falls back to software if the attached widget cannot use
                   // GL.
  bool enable_js;
} lynx_gtk_view_options_t;
// parent must be a GtkBox. The view owns its engine and a widget reference;
// parenting adds GTK's reference. NULL indicates invalid options/host setup.
LYNX_CAPI_EXPORT lynx_gtk_view_t* lynx_gtk_view_create(
    GtkWidget* parent, const lynx_gtk_view_options_t* options);
// Borrowed handles, valid until destroy. Do not release the engine separately,
// or manually remove/destroy the rendering widget.
LYNX_CAPI_EXPORT GtkWidget* lynx_gtk_view_get_widget(lynx_gtk_view_t* view);
LYNX_CAPI_EXPORT struct lynx_view_t* lynx_gtk_view_get_engine(
    lynx_gtk_view_t* view);
LYNX_CAPI_EXPORT bool lynx_gtk_view_uses_gl(lynx_gtk_view_t* view);
// Views can move to another GtkBox on the same GDK display, or detach. GL
// retains its display-owned producer context and recreates the widget context.
LYNX_CAPI_EXPORT bool lynx_gtk_view_attach(lynx_gtk_view_t* view,
                                           GtkWidget* parent);
LYNX_CAPI_EXPORT bool lynx_gtk_view_detach(lynx_gtk_view_t* view);
// Copies the bundle bytes before returning. true means accepted, not loaded.
LYNX_CAPI_EXPORT bool lynx_gtk_view_load_template(lynx_gtk_view_t* view,
                                                  const char* url,
                                                  const uint8_t* bytes,
                                                  size_t length);
// NULL is accepted. A false return means wrong thread; the handle remains live.
LYNX_CAPI_EXPORT bool lynx_gtk_view_destroy(lynx_gtk_view_t* view);
LYNX_EXTERN_C_END
#endif
