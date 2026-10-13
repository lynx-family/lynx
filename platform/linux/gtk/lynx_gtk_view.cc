// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0.
#include "platform/linux/gtk/lynx_gtk_view.h"

#include <dlfcn.h>

#include <algorithm>
#include <cmath>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

#include "clay/shell/platform/linux/gtk/clay_gtk_surface.h"
#include "platform/embedder/lynx_view_priv.h"
#include "platform/embedder/public/lynx_load_meta.h"
#include "platform/embedder/windowless/lynx_ui_renderer_windowless.h"
#include "platform/linux/gtk/gtk_keyboard_bridge.h"
#include "platform/linux/gtk/gtk_pointer_bridge.h"
#include "platform/linux/gtk/gtk_text_input.h"
#include "platform/linux/gtk/lynx_gtk_native_plane.h"
#include "platform/linux/gtk/lynx_gtk_view_capi.h"
namespace lynx::gtk {
namespace {
std::thread::id ui_thread;
bool initialized = false;
void Post(uint64_t delay_ns, std::function<void()> fn) {
  auto* source = g_timeout_source_new(static_cast<guint>(std::min<uint64_t>(
      delay_ns / 1000000 + (delay_ns % 1000000 != 0), G_MAXUINT)));
  auto* task = new std::function<void()>(std::move(fn));
  g_source_set_callback(
      source,
      +[](gpointer data) -> gboolean {
        (*static_cast<std::function<void()>*>(data))();
        return G_SOURCE_REMOVE;
      },
      task,
      +[](gpointer data) { delete static_cast<std::function<void()>*>(data); });
  g_source_attach(source, g_main_context_default());
  g_source_unref(source);
}
}  // namespace
bool View::InitializeTaskRunner() {
  if (const char* error = gtk_check_version(4, LYNX_GTK_REQUIRED_MINOR, 0)) {
    g_printerr("GTK4 host runtime is unsupported: %s\n", error);
    return false;
  }
  if (initialized) return std::this_thread::get_id() == ui_thread;
  ui_thread = std::this_thread::get_id();
  lynx_windowless_ui_task_runner_config_t config{};
  config.struct_size = sizeof(config);
  config.runs_on_current_thread_callback =
      +[](void*) { return std::this_thread::get_id() == ui_thread; };
  config.post_task_callback = +[](lynx_task_t task, uint64_t target, void*) {
    uint64_t now = static_cast<uint64_t>(g_get_monotonic_time()) * 1000;
    Post(target > now ? target - now : 0,
         [task] { lynx_windowless_run_ui_task(task); });
  };
  initialized = lynx_windowless_set_global_ui_task_runner(&config);
  return initialized;
}
class View::Renderer : public pub::LynxWindowlessRenderer {
 public:
  Renderer(GtkWidget* parent, bool gl)
      : LynxWindowlessRenderer(kRendererTypeSoftware) {
    // The render surface and platform child widgets share the same origin.
    // Keep the fixed overlay out of measurement so native children cannot
    // enlarge the viewport. Empty parts of GtkFixed do not consume input.
    container_ = gtk_overlay_new();
    g_object_ref_sink(container_);
    gtk_widget_set_hexpand(container_, true);
    gtk_widget_set_vexpand(container_, true);
    native_children_ = CreateNativePlane();
    gtk_widget_set_overflow(native_children_, GTK_OVERFLOW_HIDDEN);
    gtk_widget_set_hexpand(native_children_, true);
    gtk_widget_set_vexpand(native_children_, true);
    surface = std::make_shared<clay::GtkSurface>(gl);
    gtk_overlay_set_child(GTK_OVERLAY(container_), surface->widget());
    gtk_overlay_add_overlay(GTK_OVERLAY(container_), native_children_);
    gtk_box_append(GTK_BOX(parent), container_);
    if (gl && gtk_widget_get_native(parent))
      gtk_widget_realize(surface->widget());
    if (gl && gtk_widget_get_realized(surface->widget()) &&
        surface->InitializeGL())
      type_ = kRendererTypeGLDirect;
    else if (gl) {
      gtk_overlay_set_child(GTK_OVERLAY(container_), nullptr);
      surface.reset();
      surface = std::make_shared<clay::GtkSurface>(false);
      gtk_overlay_set_child(GTK_OVERLAY(container_), surface->widget());
      g_printerr("GTK4 GL unavailable; software fallback\n");
    }
    pointer_ = std::make_unique<PointerBridge>(
        surface->widget(), [this] { return surface->DeviceScale(); },
        [this](lynx_pointer_event_t* event) {
          if (g_getenv("LYNX_GTK_INPUT_TRACE"))
            g_print("GTK4_POINTER %d %d %d %.2f %.2f %ld %.2f %.2f\n",
                    event->device_kind, event->device, event->phase, event->x,
                    event->y, event->buttons, event->scroll_delta_x,
                    event->scroll_delta_y);
          SendPointerEvent(event);
        });
    keyboard_ = std::make_unique<KeyboardBridge>(
        surface->widget(), [this](lynx_key_event_t* event) {
          if (g_getenv("LYNX_GTK_INPUT_TRACE") &&
              (!text_input_ || !text_input_->IsPassword()))
            g_print("GTK4_KEY %d %lx %lx %d\n", event->type, event->physical,
                    event->logical, event->synthesized);
          SendKeyEvent(event);
        });
  }
  ~Renderer() override {
    // Engine/raster work has joined before the adapter drops this renderer.
    for (auto* source : sources_) {
      g_source_destroy(source);
      g_source_unref(source);
    }
    keyboard_.reset();
    text_input_.reset();
    pointer_.reset();
    if (auto* parent = gtk_widget_get_parent(container_))
      gtk_box_remove(GTK_BOX(parent), container_);
    gtk_overlay_set_child(GTK_OVERLAY(container_), nullptr);
    g_object_unref(container_);
  }
  bool OnGLMakeCurrent() override { return surface->MakeCurrent(); }
  void ShowTextInput(bool show) override {
    if (text_input_) text_input_->Show(show);
  }
  void UpdateCaretPosition(float x, float y, float width,
                           float height) override {
    if (text_input_) text_input_->Caret(x, y, width, height);
  }
  bool OnGLClearCurrent() override { return surface->ClearCurrent(); }
  uint32_t OnGLCreateFBO(int width, int height) override {
    return surface->CreateFBO(width, height);
  }
  bool OnGLPresent() override { return surface->PresentGL(); }
  void* OnGLProcResolver(const char* name) override {
    // Resolve the driver used by GDK, bypassing statically linked ANGLE.
    using Resolver = void* (*)(const char*);
    static void* library = dlopen("libEGL.so.1", RTLD_NOW | RTLD_LOCAL);
    static auto resolve =
        library
            ? reinterpret_cast<Resolver>(dlsym(library, "eglGetProcAddress"))
            : nullptr;
    return resolve ? resolve(name) : nullptr;
  }
  bool OnSoftwarePresent(const void* allocation, size_t stride,
                         size_t height) override {
    return surface->PresentSoftware(allocation, stride, height);
  }
  void OnPostTask(lynx_task_t task, uint64_t delay) override {
    auto* source = g_timeout_source_new(static_cast<guint>(std::min<uint64_t>(
        delay / 1000000 + (delay % 1000000 != 0), G_MAXUINT)));
    auto* callback =
        new std::function<void()>([weak = weak_from_this(), source, task] {
          if (auto base = weak.lock()) {
            auto self = std::static_pointer_cast<Renderer>(base);
            {
              std::lock_guard<std::mutex> lock(self->sources_mutex_);
              auto& sources = self->sources_;
              sources.erase(std::find(sources.begin(), sources.end(), source));
              g_source_unref(source);
            }
            self->RunTask(task);
          }
        });
    g_source_set_callback(
        source,
        +[](gpointer data) -> gboolean {
          (*static_cast<std::function<void()>*>(data))();
          return G_SOURCE_REMOVE;
        },
        callback,
        +[](gpointer data) {
          delete static_cast<std::function<void()>*>(data);
        });
    std::lock_guard<std::mutex> lock(sources_mutex_);
    sources_.push_back(source);
    g_source_attach(source, g_main_context_default());
  }
  bool gl() const { return type_ == kRendererTypeGLDirect; }
  GtkWidget* container_ = nullptr;
  GtkWidget* native_children_ = nullptr;
  std::shared_ptr<clay::GtkSurface> surface;
  std::unique_ptr<PointerBridge> pointer_;
  std::unique_ptr<KeyboardBridge> keyboard_;
  std::unique_ptr<TextInput> text_input_;
  std::mutex sources_mutex_;
  std::vector<GSource*> sources_;
};
}  // namespace lynx::gtk

namespace lynx::embedder {
// Reuse the host-driven Clay engine, while owning the GTK presentation and
// keeping its lifetime longer than the engine. No headless build mode required.
class LynxUIRendererGtk final : public LynxUIRenderer {
 public:
  explicit LynxUIRendererGtk(lynx_view_builder_t* builder)
      : LynxUIRenderer(builder) {
    renderer_ = std::make_shared<gtk::View::Renderer>(
        static_cast<GtkWidget*>(builder->parent), builder->gtk_use_gl);
    renderer_->InitIfNeeded();
    auto previous = builder->windowless_renderer;
    lynx_view_builder_set_windowless_renderer(builder, renderer_->Impl());
    engine_ = std::make_unique<LynxUIRendererWindowless>(builder);
    builder->windowless_renderer = std::move(previous);
    renderer_->text_input_ = std::make_unique<gtk::TextInput>(
        widget(),
        [this](int client, const std::string& text, int base, int extent,
               int composing_base, int composing_extent) {
          const bool password = renderer_->text_input_->IsPassword();
          const bool accepted = engine_->UpdateGtkEditingState(
              client, text, base, extent, composing_base, composing_extent);
          if (accepted && g_getenv("LYNX_GTK_INPUT_TRACE")) {
            char* hash = password
                             ? nullptr
                             : g_compute_checksum_for_data(
                                   G_CHECKSUM_SHA256,
                                   reinterpret_cast<const guchar*>(text.data()),
                                   text.size());
            g_print(
                "GTK4_EDIT client=%d selection=%d,%d composing=%d,%d bytes=%zu "
                "hash=%s\n",
                client, base, extent, composing_base, composing_extent,
                text.size(), hash ? hash : "private");
            g_free(hash);
          }
          return accepted;
        },
        [this](GtkIMContext* context) {
          renderer_->keyboard_->SetIMContext(context);
        });
    auto* input = renderer_->text_input_.get();
    clay::GtkTextInputCallbacks input_callbacks;
    input_callbacks.set_client = [input](int client, const char* action,
                                         const char* type) {
      input->SetClient(client, action, type);
      if (g_getenv("LYNX_GTK_INPUT_TRACE"))
        g_print("GTK4_INPUT_CLIENT %d %s\n", client, type ? type : "");
    };
    input_callbacks.clear_client = [input] { input->ClearClient(); };
    input_callbacks.set_state =
        [input](uint64_t base, uint64_t composing_extent,
                const std::string& affinity, const std::string& text,
                bool directional, uint64_t extent, uint64_t composing_base) {
          input->SetState(base, composing_extent, affinity, text, directional,
                          extent, composing_base);
        };
    engine_->SetGtkTextInputCallbacks(std::move(input_callbacks));

    renderer_->surface->SetRecoveryCallback(
        [this] { return RecreateGLContext(); });
  }
  ~LynxUIRendererGtk() override {
    renderer_->surface->SetRecoveryCallback(nullptr);
    g_signal_handlers_disconnect_by_data(widget(), this);
    if (tick_) gtk_widget_remove_tick_callback(widget(), tick_);
    view_ = nullptr;
    renderer_->keyboard_->SetIMContext(nullptr);
    engine_->SetGtkTextInputCallbacks({});
    renderer_->text_input_->ClearClient();
    engine_.reset();  // Join raster work before GTK resources are released.
  }
  void OnViewCreated(lynx_view_t* view) override {
    view_ = view;
    tick_ = gtk_widget_add_tick_callback(
        widget(),
        +[](GtkWidget*, GdkFrameClock*, gpointer data) -> gboolean {
          static_cast<LynxUIRendererGtk*>(data)->UpdateViewport();
          return G_SOURCE_CONTINUE;
        },
        this, nullptr);
    g_signal_connect(widget(), "map",
                     G_CALLBACK(+[](GtkWidget*, gpointer data) {
                       static_cast<LynxUIRendererGtk*>(data)->Visibility(true);
                     }),
                     this);
    g_signal_connect(widget(), "unmap",
                     G_CALLBACK(+[](GtkWidget*, gpointer data) {
                       static_cast<LynxUIRendererGtk*>(data)->Visibility(false);
                     }),
                     this);
    Visibility(gtk_widget_get_mapped(widget()));
  }
  GtkWidget* widget() const { return renderer_->surface->widget(); }
  std::shared_ptr<gtk::View::Renderer> renderer() const { return renderer_; }
  bool RecreateGLContext() {
#ifdef ENABLE_SKITY
    if (!renderer_->gl() || !gtk_widget_get_realized(widget())) return false;
    if (!engine_->SuspendGLSurface()) return false;
    if (!renderer_->surface->RecreateGL()) return false;
    return engine_->ResumeGLSurface();
#else
    return false;
#endif
  }
  bool InjectGLFailureForTesting() {
#ifdef ENABLE_SKITY
    if (!renderer_->gl()) return false;
    renderer_->surface->InjectGLFailureForTesting();
    engine_->RequestPaint();
    return true;
#else
    return false;
#endif
  }
  bool Attach(GtkWidget* parent) {
    if (!GTK_IS_BOX(parent)) return false;
    auto* old = gtk_widget_get_parent(renderer_->container_);
    if (old == parent) return true;
    // Shared textures cannot cross GDK displays. Reject before mutating parent.
    if (gtk_widget_get_display(parent) != gtk_widget_get_display(widget()))
      return false;
    if (old) gtk_box_remove(GTK_BOX(old), renderer_->container_);
    gtk_box_append(GTK_BOX(parent), renderer_->container_);
    return true;
  }
  bool Detach() {
    if (auto* parent = gtk_widget_get_parent(renderer_->container_))
      gtk_box_remove(GTK_BOX(parent), renderer_->container_);
    return true;
  }
  void SetParent(NativeWindow parent) override {
    if (parent)
      Attach(static_cast<GtkWidget*>(parent));
    else
      Detach();
  }
  // Linux native views receive a GtkFixed hosting plane, in logical pixels.
  NativeWindow GetNativeWindow() override {
    return renderer_->native_children_;
  }
  void SetPixelRatio(float ratio) override {
    pixel_ratio_ = ratio;
    engine_->SetPixelRatio(ratio);
  }
  void SetFrame(float x, float y, float width, float height) override {
    LynxUIRenderer::SetFrame(x, y, width, height);
    engine_->SetFrame(x, y, width, height);
  }
  void Reset() override { engine_->Reset(); }
  void OnEnterForeground() override { engine_->OnEnterForeground(); }
  void OnEnterBackground() override { engine_->OnEnterBackground(); }
  void RegisterIMEHandler(void* handler, void* opaque) override {
    engine_->RegisterIMEHandler(handler, opaque);
  }
  void RegisterNativeView(const char* name, lynx_native_view_creator creator,
                          void* opaque) override {
    engine_->RegisterNativeView(name, creator, opaque);
  }
  tasm::UIDelegate* GetUIDelegate() override {
    return engine_->GetUIDelegate();
  }

 private:
  void Visibility(bool visible) {
    if (foreground_ == visible) return;
    foreground_ = visible;
    if (visible)
      engine_->OnEnterForeground();
    else
      engine_->OnEnterBackground();
  }
  void UpdateViewport() {
    int w = gtk_widget_get_width(widget()), h = gtk_widget_get_height(widget());
    double scale = renderer_->surface->DeviceScale();
    if (w <= 0 || h <= 0 ||
        (w == last_width_ && h == last_height_ && scale == last_scale_))
      return;
    last_width_ = w;
    last_height_ = h;
    last_scale_ = scale;
    int screen_width = w, screen_height = h;
    if (auto* native = gtk_widget_get_native(widget())) {
      if (auto* monitor = gdk_display_get_monitor_at_surface(
              gtk_widget_get_display(widget()),
              gtk_native_get_surface(native))) {
        GdkRectangle geometry;
        gdk_monitor_get_geometry(monitor, &geometry);
        screen_width = geometry.width;
        screen_height = geometry.height;
      }
    }
    lynx_view_update_screen_metrics(view_, screen_width, screen_height, scale);
    lynx_view_set_frame(view_, 0, 0, w, h);
    g_print("GTK4_VIEWPORT %d %d scale=%g\n", w, h, scale);
  }
  // Members destroy in reverse order: engine must stop before renderer release.
  std::shared_ptr<gtk::View::Renderer> renderer_;
  std::unique_ptr<LynxUIRendererWindowless> engine_;
  lynx_view_t* view_ = nullptr;
  guint tick_ = 0;
  int last_width_ = 0, last_height_ = 0;
  double last_scale_ = 0;
  bool foreground_ = true;
};
std::unique_ptr<LynxUIRenderer> LynxUIRenderer::CreateWithBuilder(
    lynx_view_builder_t* builder) {
  if (!gtk::View::InitializeTaskRunner() || !GTK_IS_BOX(builder->parent))
    return nullptr;
  return std::make_unique<LynxUIRendererGtk>(builder);
}
}  // namespace lynx::embedder

namespace lynx::gtk {
View::View(GtkWidget* parent, bool gl, pub::LynxView::Builder& builder) {
  g_assert(initialized && std::this_thread::get_id() == ui_thread);
  g_assert(GTK_IS_BOX(parent));
  g_assert(!builder.Impl()->windowless_renderer);
  builder.SetParent(parent);
  builder.Impl()->gtk_use_gl = gl;
  view_ = builder.Build();
  g_assert(view_->Impl());
  renderer_ = static_cast<embedder::LynxUIRendererGtk*>(
                  view_->Impl()->lynx_ui_renderer.get())
                  ->renderer();
}
View::~View() {
  g_assert(std::this_thread::get_id() == ui_thread);
  view_.reset();
  renderer_.reset();
}
bool View::Attach(GtkWidget* parent) {
  g_return_val_if_fail(std::this_thread::get_id() == ui_thread, false);
  return static_cast<embedder::LynxUIRendererGtk*>(
             view_->Impl()->lynx_ui_renderer.get())
      ->Attach(parent);
}
bool View::Detach() {
  g_return_val_if_fail(std::this_thread::get_id() == ui_thread, false);
  return static_cast<embedder::LynxUIRendererGtk*>(
             view_->Impl()->lynx_ui_renderer.get())
      ->Detach();
}
bool View::RecreateGLContext() {
  g_return_val_if_fail(std::this_thread::get_id() == ui_thread, false);
  return static_cast<embedder::LynxUIRendererGtk*>(
             view_->Impl()->lynx_ui_renderer.get())
      ->RecreateGLContext();
}
bool View::InjectGLFailureForTesting() {
  g_return_val_if_fail(std::this_thread::get_id() == ui_thread, false);
  return static_cast<embedder::LynxUIRendererGtk*>(
             view_->Impl()->lynx_ui_renderer.get())
      ->InjectGLFailureForTesting();
}
GtkWidget* View::widget() const { return renderer_->surface->widget(); }
bool View::uses_gl() const { return renderer_->gl(); }
bool View::SendPointerEventForTesting(lynx_pointer_event_t* event) {
  if (std::this_thread::get_id() != ui_thread || !event ||
      event->struct_size != sizeof(*event))
    return false;
  renderer_->SendPointerEvent(event);
  return true;
}
uint64_t View::FrameCountForTesting() const {
  return renderer_->surface->FrameCountForTesting();
}
}  // namespace lynx::gtk

struct lynx_gtk_view_t {
  lynx_view_t* engine;
};
namespace {
bool OnGtkThread() {
  return lynx::gtk::initialized &&
         std::this_thread::get_id() == lynx::gtk::ui_thread;
}
lynx::embedder::LynxUIRendererGtk* GtkRenderer(lynx_gtk_view_t* view) {
  return static_cast<lynx::embedder::LynxUIRendererGtk*>(
      view->engine->lynx_ui_renderer.get());
}
}  // namespace
LYNX_EXTERN_C bool lynx_gtk_initialize(void) {
  return lynx::gtk::View::InitializeTaskRunner();
}
LYNX_EXTERN_C lynx_gtk_view_t* lynx_gtk_view_create(
    GtkWidget* parent, const lynx_gtk_view_options_t* options) {
  if (!options || options->struct_size != sizeof(*options) ||
      !std::isfinite(options->width) || !std::isfinite(options->height) ||
      options->width <= 0 || options->height <= 0 || !options->icu_data_path ||
      !*options->icu_data_path || !GTK_IS_BOX(parent) ||
      !lynx::gtk::View::InitializeTaskRunner())
    return nullptr;
  auto* builder = lynx_view_builder_create();
  lynx_view_builder_set_parent(builder, parent);
  builder->gtk_use_gl = options->prefer_gl;
  lynx_view_builder_set_frame(builder, 0, 0, options->width, options->height);
  lynx_view_builder_set_screen_size(builder, options->width, options->height,
                                    gtk_widget_get_scale_factor(parent));
  lynx_view_builder_set_icu_data_path(builder, options->icu_data_path);
  lynx_view_builder_set_enable_js_runtime(builder, options->enable_js);
  auto* engine = lynx_view_create(builder, nullptr);
  lynx_view_builder_release(builder);
  return engine ? new lynx_gtk_view_t{engine} : nullptr;
}
LYNX_EXTERN_C GtkWidget* lynx_gtk_view_get_widget(lynx_gtk_view_t* view) {
  return view && OnGtkThread() ? GtkRenderer(view)->widget() : nullptr;
}
LYNX_EXTERN_C lynx_view_t* lynx_gtk_view_get_engine(lynx_gtk_view_t* view) {
  return view && OnGtkThread() ? view->engine : nullptr;
}
LYNX_EXTERN_C bool lynx_gtk_view_uses_gl(lynx_gtk_view_t* view) {
  return view && OnGtkThread() && GtkRenderer(view)->renderer()->gl();
}
LYNX_EXTERN_C bool lynx_gtk_view_attach(lynx_gtk_view_t* view,
                                        GtkWidget* parent) {
  return view && OnGtkThread() && GtkRenderer(view)->Attach(parent);
}
LYNX_EXTERN_C bool lynx_gtk_view_detach(lynx_gtk_view_t* view) {
  return view && OnGtkThread() && GtkRenderer(view)->Detach();
}
LYNX_EXTERN_C bool lynx_gtk_view_load_template(lynx_gtk_view_t* view,
                                               const char* url,
                                               const uint8_t* bytes,
                                               size_t length) {
  if (!view || !OnGtkThread() || !url || !bytes || !length) return false;
  auto meta = std::make_shared<lynx::pub::LynxLoadMeta>();
  meta->SetUrl(url);
  meta->SetBinaryData(std::vector<uint8_t>(bytes, bytes + length));
  lynx_view_load_template(view->engine, meta->Impl());
  return true;
}
LYNX_EXTERN_C bool lynx_gtk_view_destroy(lynx_gtk_view_t* view) {
  if (!view) return true;
  if (!OnGtkThread()) return false;
  lynx_view_release(view->engine);
  delete view;
  return true;
}
