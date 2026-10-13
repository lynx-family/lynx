// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0.
#include <cmath>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>

#ifdef GTK_HOST_SKITY
#include "clay/third_party/txt/src/txt/font_manager_fontconfig_skity.h"
#endif
#include "platform/embedder/public/lynx_load_meta.h"
#include "platform/embedder/public/lynx_native_view.h"
#include "platform/linux/gtk/lynx_gtk_view.h"
class PatternSurface final : public lynx::pub::LynxNativeView {
 public:
  explicit PatternSurface(void*) {}
  bool IsSurfaceEnabled() override { return true; }
  void OnLayoutChanged(float, float, float width, float height,
                       float ratio) override {
    const int w = static_cast<int>(std::ceil(width * ratio));
    const int h = static_cast<int>(std::ceil(height * ratio));
    if (w <= 0 || h <= 0 || w > 8192 || h > 8192) return;
    auto* pixels = reinterpret_cast<uint8_t*>(AcquireSurface(w, h));
    if (!pixels) {
      g_printerr("GTK4 external surface acquire failed\n");
      return;
    }
    // Native8888 is BGRA. Surface buffers use the standard bottom-left origin.
    for (int y = 0; y < h; ++y)
      for (int x = 0; x < w; ++x) {
        auto* p = pixels + (y * w + x) * 4;
        p[0] = y < h / 2 ? 80 : 60;
        p[1] = y < h / 2 ? 180 : 40;
        p[2] = y < h / 2 ? 20 : 200;
        p[3] = 255;
      }
    g_assert(SwapBack());
    g_print("GTK4 external surface SHM frame published PASS\n");
  }
};
int main(int argc, char** argv) {
  if (argc < 3) {
    g_printerr("Usage: lynx_gtk_smoke bundle icudtl.dat [gl]\n");
    return 2;
  }
  gtk_init();
#ifdef GTK_HOST_SKITY
  g_print("GTK4_ENGINE skity\n");
  auto fonts = txt::MakeFontconfigSkityFontManager();
  for (auto character : {0x4c, 0x4e2d}) {
    auto face = fonts->MatchFamilyStyleCharacter(
        "sans-serif", skity::FontStyle::Normal(), nullptr, 0, character);
    if (!face || !face->UnicharToGlyph(character)) return 4;
  }
  g_print("GTK4_FONT Latin/CJK PASS families=%d\n", fonts->CountFamilies());
#else
  g_print("GTK4_ENGINE skia\n");
#endif
  if (!lynx::gtk::View::InitializeTaskRunner()) return 3;
  auto* window = gtk_window_new();
  g_object_ref_sink(window);
  gtk_window_set_title(GTK_WINDOW(window), "Lynx GTK4 Rendering");
  gtk_window_set_default_size(GTK_WINDOW(window), 720, 600);
  auto* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_window_set_child(GTK_WINDOW(window), box);
  gtk_window_present(GTK_WINDOW(window));
  gtk_widget_realize(box);
  lynx::pub::LynxView::Builder builder;
  builder.SetScreenSize(1280, 800, gtk_widget_get_scale_factor(box))
      .SetFrame(0, 0, 720, 600)
      .SetICUDataPath(argv[2])
      .SetEnableJSRuntime(g_getenv("LYNX_GTK_ENABLE_JS") != nullptr);
  if (g_getenv("LYNX_GTK_EXTERNAL_TEXTURE"))
    builder.RegisterNativeView<PatternSurface>("x-test-surface");
  auto view = std::make_unique<lynx::gtk::View>(
      box, argc > 3 && std::string(argv[3]) == "gl", builder);
  std::ifstream input(argv[1], std::ios::binary);
  if (!input) return 4;
  std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(input)), {});
  auto meta = std::make_shared<lynx::pub::LynxLoadMeta>();
  meta->SetUrl(argv[1]);
  meta->SetBinaryData(std::move(bytes));
  view->lynx_view()->LoadTemplate(meta);
  auto* loop = g_main_loop_new(nullptr, FALSE);
  g_signal_connect(window, "close-request",
                   G_CALLBACK(+[](GtkWindow*, gpointer data) -> gboolean {
                     g_main_loop_quit(static_cast<GMainLoop*>(data));
                     return TRUE;
                   }),
                   loop);
  struct ReparentTest {
    GtkWidget** window;
    lynx::gtk::View* view;
    GMainLoop* loop;
    unsigned moves = 0;
  } reparent{&window, view.get(), loop};
  guint reparent_source = 0;
  struct HiddenTest {
    GtkWidget* window;
    lynx::gtk::View* view;
    GMainLoop* loop;
    unsigned stage = 0, polls = 0;
    uint64_t paused_frames = 0;
  } hidden{window, view.get(), loop};
  guint hidden_source = 0;
  if (g_getenv("LYNX_GTK_TEST_HIDDEN")) {
    hidden_source = g_timeout_add(
        400,
        +[](gpointer data) -> gboolean {
          auto* test = static_cast<HiddenTest*>(data);
          g_assert(++test->polls < 30);
          const auto count = test->view->FrameCountForTesting();
          if (test->stage == 0 && count >= 3) {
            gtk_widget_set_visible(test->window, FALSE);
            test->stage = 1;
          } else if (test->stage == 1) {
            test->paused_frames = count;
            test->stage = 2;
          } else if (test->stage == 2) {
            g_assert(count == test->paused_frames);
            gtk_widget_set_visible(test->window, TRUE);
            test->stage = 3;
          } else if (test->stage == 3 && count >= test->paused_frames + 3) {
            g_print("GTK4_HIDDEN animation paused/resumed PASS\n");
            g_main_loop_quit(test->loop);
          }
          return G_SOURCE_CONTINUE;
        },
        &hidden);
  }
  guint recovery_source = 0;
  struct RecoveryAnimationTest {
    lynx::gtk::View* view;
    GMainLoop* loop;
    uint64_t minimum_frames = 3;
    unsigned cycles = 0, polls = 0;
  } recovery_animation{view.get(), loop};
  guint recovery_animation_source = 0;
  if (g_getenv("LYNX_GTK_RECOVERY_ANIMATION")) {
    recovery_animation_source = g_timeout_add(
        100,
        +[](gpointer data) -> gboolean {
          auto* test = static_cast<RecoveryAnimationTest*>(data);
          g_assert(++test->polls < 1000);
          const auto count = test->view->FrameCountForTesting();
          if (count < test->minimum_frames) return G_SOURCE_CONTINUE;
          if (test->cycles == 20) {
            g_print("GTK4_RECOVERY_ANIMATION cycles=20 PASS\n");
            g_main_loop_quit(test->loop);
            return G_SOURCE_CONTINUE;
          }
          if (g_getenv("LYNX_GTK_RECOVERY_INJECT"))
            g_assert(test->view->InjectGLFailureForTesting());
          else
            g_assert(test->view->RecreateGLContext());
          ++test->cycles;
          test->minimum_frames = count + 3;
          return G_SOURCE_CONTINUE;
        },
        &recovery_animation);
  }

  if (g_getenv("LYNX_GTK_RECOVERY_FILE")) {
    recovery_source = g_timeout_add(
        100,
        +[](gpointer data) -> gboolean {
          auto* test = static_cast<ReparentTest*>(data);
          const char* path = g_getenv("LYNX_GTK_RECOVERY_FILE");
          if (!g_file_test(path, G_FILE_TEST_EXISTS)) return G_SOURCE_CONTINUE;
          std::remove(path);
          g_assert(test->view->RecreateGLContext());
          g_print("GTK4_RECOVERY %u PASS\n", ++test->moves);
          return G_SOURCE_CONTINUE;
        },
        &reparent);
  }
  if (g_getenv("LYNX_GTK_REPARENT_FILE")) {
    reparent_source = g_timeout_add(
        100,
        +[](gpointer data) -> gboolean {
          auto* test = static_cast<ReparentTest*>(data);
          const char* path = g_getenv("LYNX_GTK_REPARENT_FILE");
          if (!g_file_test(path, G_FILE_TEST_EXISTS)) return G_SOURCE_CONTINUE;
          std::remove(path);
          auto* next = gtk_window_new();
          g_object_ref_sink(next);
          gtk_window_set_title(GTK_WINDOW(next), "Lynx GTK4 Rendering");
          gtk_window_set_default_size(GTK_WINDOW(next), 720, 600);
          auto* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
          gtk_window_set_child(GTK_WINDOW(next), box);
          g_signal_connect(
              next, "close-request",
              G_CALLBACK(+[](GtkWindow*, gpointer loop) -> gboolean {
                g_main_loop_quit(static_cast<GMainLoop*>(loop));
                return TRUE;
              }),
              test->loop);
          gtk_window_present(GTK_WINDOW(next));
          g_assert(test->view->Detach());
          gtk_window_destroy(GTK_WINDOW(*test->window));
          g_object_unref(*test->window);
          // The original native window no longer exists when GL reconnects.
          g_assert(test->view->Attach(box));
          *test->window = next;
          g_print("GTK4_REPARENT %u PASS\n", ++test->moves);
          return G_SOURCE_CONTINUE;
        },
        &reparent);
  }
  struct PartialCancelTest {
    lynx::gtk::View* view;
    GtkWidget* window;
    unsigned step = 0;
  } partial_cancel{view.get(), window};
  guint partial_cancel_source = 0;
  if (g_getenv("LYNX_GTK_TEST_PARTIAL_CANCEL")) {
    partial_cancel_source = g_timeout_add(
        250,
        +[](gpointer data) -> gboolean {
          auto* test = static_cast<PartialCancelTest*>(data);
          if (!test->view->FrameCountForTesting()) return G_SOURCE_CONTINUE;
          const double scale = gdk_surface_get_scale(
              gtk_native_get_surface(GTK_NATIVE(test->window)));
          auto send = [&](int64_t id, lynx_pointer_phase_e phase, double x) {
            lynx_pointer_event_t event{};
            event.struct_size = sizeof(event);
            event.device = id;
            event.device_kind = kLynxPointerDeviceKindTouch;
            event.phase = phase;
            event.timestamp = g_get_monotonic_time();
            event.x = x * scale;
            event.y = 200 * scale;
            g_assert(test->view->SendPointerEventForTesting(&event));
          };
          switch (test->step++) {
            case 0:
              send(17, kLynxPointerPhaseAdd, 200);
              send(17, kLynxPointerPhaseDown, 200);
              break;
            case 1:
              send(29, kLynxPointerPhaseAdd, 350);
              send(29, kLynxPointerPhaseDown, 350);
              break;
            case 2:
              send(17, kLynxPointerPhaseCancel, 200);
              send(17, kLynxPointerPhaseRemove, 200);
              break;
            case 3:
              send(29, kLynxPointerPhaseMove, 380);
              break;
            case 4:
              send(29, kLynxPointerPhaseUp, 380);
              send(29, kLynxPointerPhaseRemove, 380);
              break;
            default:
              if (test->step == 6) g_print("GTK4_PARTIAL_CANCEL_INJECTED\n");
              return G_SOURCE_CONTINUE;
          }
          return G_SOURCE_CONTINUE;
        },
        &partial_cancel);
  }
  struct ReloadContactTest {
    lynx::gtk::View* view;
    GtkWidget* window;
    const char* bundle;
    unsigned step = 0;
    uint64_t before_reload = 0;
  } reload_contact{view.get(), window, argv[1]};
  guint reload_contact_source = 0;
  if (g_getenv("LYNX_GTK_TEST_RELOAD_CONTACT")) {
    reload_contact_source = g_timeout_add(
        300,
        +[](gpointer data) -> gboolean {
          auto* test = static_cast<ReloadContactTest*>(data);
          if (!test->view->FrameCountForTesting()) return G_SOURCE_CONTINUE;
          if (test->step == 2 &&
              test->view->FrameCountForTesting() <= test->before_reload)
            return G_SOURCE_CONTINUE;
          const double scale = gdk_surface_get_scale(
              gtk_native_get_surface(GTK_NATIVE(test->window)));
          auto send = [&](int64_t id, lynx_pointer_phase_e phase, double x) {
            lynx_pointer_event_t event{};
            event.struct_size = sizeof(event);
            event.device = id;
            event.device_kind = kLynxPointerDeviceKindTouch;
            event.phase = phase;
            event.timestamp = g_get_monotonic_time();
            event.x = x * scale;
            event.y = 200 * scale;
            g_assert(test->view->SendPointerEventForTesting(&event));
          };
          switch (test->step++) {
            case 0:
              send(17, kLynxPointerPhaseAdd, 200);
              send(17, kLynxPointerPhaseDown, 200);
              break;
            case 1: {
              test->before_reload = test->view->FrameCountForTesting();
              std::ifstream input(test->bundle, std::ios::binary);
              std::vector<uint8_t> bytes(
                  (std::istreambuf_iterator<char>(input)), {});
              auto meta = std::make_shared<lynx::pub::LynxLoadMeta>();
              meta->SetUrl(std::string(test->bundle) + "#reload");
              meta->SetBinaryData(std::move(bytes));
              g_print("GTK4_CONTACT_RELOAD_REQUEST\n");
              test->view->lynx_view()->LoadTemplate(meta);
              break;
            }
            case 2:
              send(29, kLynxPointerPhaseAdd, 350);
              send(29, kLynxPointerPhaseDown, 350);
              break;
            case 3:
              send(29, kLynxPointerPhaseMove, 380);
              break;
            case 4:
              send(17, kLynxPointerPhaseUp, 200);
              send(17, kLynxPointerPhaseRemove, 200);
              break;
            case 5:
              send(29, kLynxPointerPhaseMove, 400);
              break;
            case 6:
              send(29, kLynxPointerPhaseUp, 400);
              send(29, kLynxPointerPhaseRemove, 400);
              break;
            default:
              if (test->step == 8) g_print("GTK4_CONTACT_RELOAD_INJECTED\n");
              break;
          }
          return G_SOURCE_CONTINUE;
        },
        &reload_contact);
  }
  if (g_getenv("LYNX_GTK_AUTO_CLOSE"))
    g_timeout_add_seconds(
        5,
        +[](gpointer data) -> gboolean {
          g_main_loop_quit(static_cast<GMainLoop*>(data));
          return G_SOURCE_REMOVE;
        },
        loop);
  g_print("GTK4_MODE %s\n", view->uses_gl() ? "gl" : "software");
  g_main_loop_run(loop);
  if (reparent_source) g_source_remove(reparent_source);
  if (recovery_source) g_source_remove(recovery_source);
  if (recovery_animation_source) g_source_remove(recovery_animation_source);
  if (hidden_source) g_source_remove(hidden_source);
  if (partial_cancel_source) g_source_remove(partial_cancel_source);
  if (reload_contact_source) g_source_remove(reload_contact_source);
  view.reset();
  gtk_window_destroy(GTK_WINDOW(window));
  g_object_unref(window);
  g_main_loop_unref(loop);
  g_print("GTK4_CLOSED PASS\n");
  return 0;
}
