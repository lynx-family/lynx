// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0.
// Compiled as C: exercises the public ABI, not a C++ wrapper.
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform/linux/gtk/lynx_gtk_view_capi.h"
#define CHECK(test)                                                          \
  do {                                                                       \
    if (!(test)) {                                                           \
      fprintf(stderr, "GTK4_LIFECYCLE FAIL line=%d: %s\n", __LINE__, #test); \
      exit(5);                                                               \
    }                                                                        \
  } while (0)

static void pump(unsigned milliseconds) {
  gint64 end = g_get_monotonic_time() + milliseconds * 1000;
  while (g_get_monotonic_time() < end) {
    for (int i = 0; i < 100 && g_main_context_iteration(NULL, FALSE); ++i) {
    }
    g_usleep(1000);
  }
}
static gpointer wrong_thread(gpointer view) {
  CHECK(lynx_gtk_view_get_widget(view) == NULL);
  CHECK(!lynx_gtk_view_destroy(view));
  return NULL;
}
int main(int argc, char** argv) {
  if (argc < 3) return 2;
  gtk_init();
  char* bytes = NULL;
  gsize length = 0;
  CHECK(g_file_get_contents(argv[1], &bytes, &length, NULL));
  gboolean gl = argc > 3 && strcmp(argv[3], "gl") == 0;
  lynx_gtk_view_options_t options = {sizeof(options), 480, 400,
                                     argv[2],         gl,  false};
  options.enable_js = getenv("LYNX_GTK_ENABLE_JS") != NULL;
  const char* multiview_request = getenv("LYNX_GTK_MULTIVIEW_FILE");
  if (multiview_request) { options.width = 720; options.height = 600; }
  int cycles = 20;
  if (getenv("LYNX_GTK_CYCLES")) {
    char* end = NULL;
    unsigned long count = strtoul(getenv("LYNX_GTK_CYCLES"), &end, 10);
    CHECK(end && !*end && count > 0 && count <= 1000);
    cycles = (int)count;
  }
  gboolean early_close = getenv("LYNX_GTK_EARLY_CLOSE") != NULL;
  GtkWidget* windows[2];
  GtkWidget* boxes[2];
  for (int i = 0; i < 2; ++i) {
    windows[i] = gtk_window_new();
    g_object_ref_sink(windows[i]);
    gtk_window_set_default_size(GTK_WINDOW(windows[i]),
                                options.width, options.height);
    gtk_window_set_title(GTK_WINDOW(windows[i]),
                         i ? "Lynx lifecycle B" : "Lynx lifecycle A");
    boxes[i] = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_window_set_child(GTK_WINDOW(windows[i]), boxes[i]);
    gtk_window_present(GTK_WINDOW(windows[i]));
  }
  pump(100);
  CHECK(lynx_gtk_view_create(boxes[0], NULL) == NULL);
  options.width = NAN;
  CHECK(lynx_gtk_view_create(boxes[0], &options) == NULL);
  options.width = 480;
  for (int cycle = 0; cycle < cycles; ++cycle) {
    lynx_gtk_view_t* a = lynx_gtk_view_create(boxes[0], &options);
    lynx_gtk_view_t* b = lynx_gtk_view_create(boxes[1], &options);
    CHECK(a && b && lynx_gtk_view_get_engine(a));
    CHECK(lynx_gtk_view_get_widget(a) != lynx_gtk_view_get_widget(b));
    CHECK(lynx_gtk_view_uses_gl(a) == gl);
    CHECK(lynx_gtk_view_load_template(a, argv[1], (uint8_t*)bytes, length));
    CHECK(lynx_gtk_view_load_template(b, argv[1], (uint8_t*)bytes, length));
    if (cycle == 0) {
      GThread* thread = g_thread_new("wrong-thread", wrong_thread, a);
      g_thread_join(thread);
    }
    pump(early_close ? 1 : (cycle == 0 ? 1000 : 120));
    if (cycle == 0 && multiview_request) {
      gint64 deadline = g_get_monotonic_time() + 60000000;
      while (!g_file_test(multiview_request, G_FILE_TEST_EXISTS) &&
             g_get_monotonic_time() < deadline) pump(10);
      CHECK(g_file_test(multiview_request, G_FILE_TEST_EXISTS));
      remove(multiview_request);
    }
    CHECK(lynx_gtk_view_attach(a, boxes[0]));
    {
      CHECK(lynx_gtk_view_detach(a));
      CHECK(gtk_widget_get_parent(lynx_gtk_view_get_widget(a)) == NULL);
      CHECK(lynx_gtk_view_attach(a, boxes[1]));
      pump(early_close ? 1 : 80);
      gtk_window_destroy(GTK_WINDOW(windows[0]));
      g_object_unref(windows[0]);
      windows[0] = gtk_window_new();
      g_object_ref_sink(windows[0]);
      gtk_window_set_default_size(GTK_WINDOW(windows[0]), 480, 400);
      gtk_window_set_title(GTK_WINDOW(windows[0]), "Lynx lifecycle A");
      boxes[0] = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
      gtk_window_set_child(GTK_WINDOW(windows[0]), boxes[0]);
      gtk_window_present(GTK_WINDOW(windows[0]));
      CHECK(lynx_gtk_view_attach(a, boxes[0]));
    }
    gtk_widget_set_visible(windows[0], FALSE);
    pump(early_close ? 1 : 30);
    gtk_widget_set_visible(windows[0], TRUE);
    pump(early_close ? 1 : 30);
    CHECK(lynx_gtk_view_destroy(a));
    // Keep pumping with B alive to deliver A's previously queued work.
    pump(early_close ? 1 : 50);
    CHECK(lynx_gtk_view_destroy(b));
    CHECK(gtk_widget_get_first_child(boxes[0]) == NULL);
    CHECK(gtk_widget_get_first_child(boxes[1]) == NULL);
    pump(early_close ? 1 : 30);
    printf("GTK4_LIFECYCLE cycle=%d PASS\n", cycle + 1);
    fflush(stdout);
  }
  // Unattached software view and immediately disposed loading engine.
  if (!gl) {
    GtkWidget* unrooted = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    g_object_ref_sink(unrooted);
    lynx_gtk_view_t* c = lynx_gtk_view_create(unrooted, &options);
    CHECK(c &&
          lynx_gtk_view_load_template(c, argv[1], (uint8_t*)bytes, length));
    CHECK(lynx_gtk_view_destroy(c));
    g_object_unref(unrooted);
    pump(100);
  }
  for (int i = 0; i < 2; ++i) {
    gtk_window_destroy(GTK_WINDOW(windows[i]));
    g_object_unref(windows[i]);
  }
  g_free(bytes);
  pump(100);
  CHECK(lynx_gtk_view_destroy(NULL));
  printf("GTK4_LIFECYCLE %s PASS cycles=%d views=%d\n", gl ? "gl" : "software", cycles, cycles * 2);
  return 0;
}
