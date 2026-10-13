// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0.
#ifndef PLATFORM_LINUX_GTK_LYNX_GTK_NATIVE_PLANE_H_
#define PLATFORM_LINUX_GTK_LYNX_GTK_NATIVE_PLANE_H_
#include <gtk/gtk.h>
namespace lynx::gtk {
// GTK picks descendants before calling contains on their parent. Keep the
// hosting plane targetable so its children receive input, but reject the plane
// itself so empty areas let events reach the Lynx render surface underneath.
inline GtkWidget* CreateNativePlane() {
  static const GType type = g_type_register_static_simple(
      GTK_TYPE_FIXED, "LynxGtkNativePlane", sizeof(GtkFixedClass),
      +[](gpointer klass, gpointer) {
        GTK_WIDGET_CLASS(klass)->contains =
            +[](GtkWidget*, double, double) -> gboolean { return false; };
      },
      sizeof(GtkFixed), nullptr, GTypeFlags(0));
  return GTK_WIDGET(g_object_new(type, nullptr));
}
}  // namespace lynx::gtk
#endif
