// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <memory>

#include "core/runtime/lepus/json_parser.h"
#include "core/shell/runtime/bts/lynx_bts_runtime_proxy_impl.h"
#include "platform/embedder/fetcher/lynx_generic_resource_fetcher_priv.h"
#include "platform/embedder/fetcher/lynx_resource_fetcher_holder.h"
#include "platform/embedder/lynx_group_priv.h"
#include "platform/embedder/lynx_load_meta_priv.h"
#include "platform/embedder/lynx_runtime_lifecycle_observer_priv.h"
#include "platform/embedder/lynx_service/lynx_security_service_priv.h"
#include "platform/embedder/lynx_service/lynx_service_center_priv.h"
#include "platform/embedder/lynx_update_meta_priv.h"
#include "platform/embedder/lynx_view_builder_priv.h"
#include "platform/embedder/lynx_view_priv.h"
#include "platform/embedder/lynx_vsync_monitor_priv.h"
#include "platform/embedder/module/global_module_registry.h"
#include "platform/embedder/resource/lynx_resource_loader_embedder.h"

LYNX_EXTERN_C lynx_view_t* lynx_view_create(lynx_view_builder_t* builder,
                                            void* user_data) {
  lynx_view_t* view = new lynx_view_t;
  view->user_data = user_data;
  const char* webview_fixed_runtime_path =
      builder->GetWebView2FixedRuntimePath();
  view->webview2_fixed_runtime_path =
      webview_fixed_runtime_path ? webview_fixed_runtime_path : "";
  // Construct ui renderer with builder.
#if defined(ENABLE_WINDOWLESS)
  if (builder->windowless_renderer) {
    view->lynx_ui_renderer =
        lynx::embedder::LynxUIRenderer::CreateWindowlessUIRenderer(builder);
  } else {
    view->lynx_ui_renderer =
        lynx::embedder::LynxUIRenderer::CreateWithBuilder(builder);
  }
#else
  view->lynx_ui_renderer =
      lynx::embedder::LynxUIRenderer::CreateWithBuilder(builder);
#endif
  // get UIDelegate from lynx_ui_renderer.
  auto* ui_delegate = view->lynx_ui_renderer->GetUIDelegate();

  // Construct template renderer with builder.
  lynx::embedder::LynxTemplateRenderer::Settings settings;
  settings.screen_size.cx = builder->screen_size.width;
  settings.screen_size.cy = builder->screen_size.height;
  settings.viewport_size.cx = builder->frame.width;
  settings.viewport_size.cy = builder->frame.height;
  settings.device_pixel_ratio = builder->screen_size.pixel_ratio;
  auto resource_loader =
      std::make_shared<lynx::embedder::LynxResourceLoaderEmbedder>();
  settings.resource_loader = resource_loader;
  if (builder->generic_fetcher) {
    view->resource_fetcher_holder =
        std::make_shared<lynx::embedder::LynxResourceFetcherHolder>(
            builder->generic_fetcher);
    resource_loader->SetResourceFetcherHolder(view->resource_fetcher_holder);
  }
  if (builder->group) {
    settings.group_id = builder->group->id;
    settings.enable_js_group_thread = builder->group->enable_js_group_thread;
    settings.preload_js_paths = builder->group->preload_js_paths;
  }
#if defined(OS_HARMONY) && defined(ENABLE_WINDOWLESS)
  settings.use_quickjs = true;
#endif
#if ENABLE_NAPI_BINDING
  std::unordered_map<std::string,
                     std::tuple<extension_module_creator, bool, void*>>
      extension_module_creators;
  lynx::embedder::GlobalModuleRegistry::GetInstance()
      .MergeWithInstanceExtensionModuleMap(builder->extension_modules_,
                                           extension_module_creators);
  view->extension_factory_ =
      std::make_shared<lynx::embedder::ExtensionModuleFactoryImpl>(
          std::move(extension_module_creators));
  view->extension_factory_->OnLynxViewCreate(view, ui_delegate);
#endif
  if (view->custom_vsync_monitor) {
    settings.vsync_monitor_platform_impl =
        std::make_shared<lynx::embedder::LynxVSyncMonitor>(
            view->custom_vsync_monitor);
  }

  auto lynx_template_renderer = std::make_unique<
      lynx::embedder::LynxTemplateRenderer>(
      settings, ui_delegate, nullptr, nullptr,
#if ENABLE_NAPI_BINDING
      [view, native_modules = builder->native_modules](
          std::shared_ptr<lynx::shell::LynxEngineProxy>,
          std::shared_ptr<lynx::shell::LynxRuntimeProxy> proxy,
          std::shared_ptr<lynx::runtime::js::LynxModuleManager> module_manager,
          const fml::RefPtr<fml::TaskRunner>& js_runner) {
        // napi NativeModuleFactory.
        std::unordered_map<std::string, std::pair<napi_module_creator, void*>>
            module_creators;
        // Merge global modules into instance modules.
        lynx::embedder::GlobalModuleRegistry::GetInstance()
            .MergeWithInstanceModuleMap(native_modules, module_creators);
        view->lynx_module_manager =
            std::make_shared<lynx::embedder::LynxModuleManagerNAPI>(
                view, module_manager, std::move(module_creators));
        // Setup the runtime lifecycle listener.
        view->lynx_module_manager->SetupRuntimeLifecycleListener(proxy);
        module_manager->SetExtensionModuleFactory(view->extension_factory_);
        view->extension_factory_->OnRuntimeInit(js_runner);
      }
#else
      nullptr
#endif
  );
  view->lynx_template_renderer = std::move(lynx_template_renderer);
  view->lynx_view_clients =
      std::make_unique<lynx::embedder::LynxViewClients>(view);
  view->lynx_ui_renderer->AddClient(view->lynx_view_clients.get());
  view->lynx_template_renderer->AddClient(view->lynx_view_clients.get());
  view->lynx_template_renderer->SetTemplateVerification(
      [](uint8_t* content, size_t length, const std::string url,
         const char** error_msg) {
        lynx_security_service_t* security_service =
            reinterpret_cast<lynx_security_service_t*>(lynx_service_get_service(
                lynx_service_get_center_instance(), kServiceTypeSecurity));
        // verify content with security service.
        if (lynx_security_service_verify_tasm(security_service, content, length,
                                              url.c_str(), kTypeTemplate,
                                              error_msg) != 0) {
          LOGE("LynxTemplateRenderer LoadTemplate verification failed");
          return false;
        }
        return true;
      });

  if (builder->font_scale != 1.0f) {
    view->lynx_template_renderer->SetFontScale(builder->font_scale);
  }
  view->lynx_template_renderer->UpdateViewport(builder->frame.width, 1,
                                               builder->frame.height, 1, true);
  return view;
}

LYNX_EXTERN_C void* lynx_view_get_user_data(lynx_view_t* view) {
  return view->user_data;
}

LYNX_EXTERN_C const char* lynx_view_get_webview2_fixed_runtime_path(
    lynx_view_t* view) {
  return view->webview2_fixed_runtime_path.c_str();
}

LYNX_EXTERN_C void lynx_view_add_client(lynx_view_t* view,
                                        lynx_view_client_t* client) {
  if (!client) {
    return;
  }
  view->lynx_view_clients->AddClient(client);
}

LYNX_EXTERN_C void lynx_view_remove_client(lynx_view_t* view,
                                           lynx_view_client_t* client) {
  if (!client) {
    return;
  }
  view->lynx_view_clients->RemoveClient(client);
}

LYNX_EXTERN_C void lynx_view_register_runtime_lifecycle_observer(
    lynx_view_t* view, lynx_runtime_lifecycle_observer_t* observer) {
#if ENABLE_NAPI_BINDING
  auto runtime_proxy = view->lynx_template_renderer->GetRuntimeProxy();
  static_cast<lynx::shell::LynxBTSRuntimeProxyImpl*>(runtime_proxy.get())
      ->AddLifecycleListener(
          std::make_unique<
              lynx::embedder::LynxRuntimeLifecycleListenerDelegate>(observer));
#endif
}

LYNX_EXTERN_C void lynx_view_load_template(lynx_view_t* view,
                                           lynx_load_meta_t* load_meta) {
  if (load_meta->global_props) {
    view->global_props = load_meta->global_props;
    view->lynx_template_renderer->UpdateGlobalProps(
        load_meta->global_props->GetValue());
  }
  if (load_meta->template_bundle) {
    view->lynx_template_renderer->LoadTemplateBundle(
        load_meta->url, *load_meta->template_bundle, nullptr,
        load_meta->initial_data);
  } else if (load_meta->binary_data.data && load_meta->binary_data.length > 0) {
    std::vector<uint8_t> source(
        load_meta->binary_data.data,
        load_meta->binary_data.data + load_meta->binary_data.length);
    view->lynx_template_renderer->LoadTemplate(
        load_meta->url, std::move(source), nullptr, load_meta->initial_data);
  } else if (!load_meta->url.empty()) {
    view->lynx_template_renderer->LoadTemplateFromURL(load_meta->url,
                                                      load_meta->initial_data);
  } else {
    // UNREACHABLE.
  }
}

LYNX_EXTERN_C void lynx_view_update_data(lynx_view_t* view,
                                         lynx_update_meta_t* update_meta) {
  if (!update_meta->update_data) {
    return;
  }
  if (update_meta->global_props) {
    view->global_props = update_meta->global_props;
  }
  view->lynx_template_renderer->UpdateMetaData(
      update_meta->update_data, view->global_props
                                    ? view->global_props->GetValue()
                                    : lynx::lepus::Value());
}

LYNX_EXTERN_C void lynx_view_reload_template(
    lynx_view_t* view, lynx_template_data_t* data,
    lynx_template_data_t* global_props) {
  view->lynx_template_renderer->ReloadTemplate(
      data->template_data, nullptr,
      (global_props && global_props->template_data)
          ? global_props->template_data->GetValue()
          : lynx::lepus::Value());
}

LYNX_EXTERN_C void lynx_view_send_global_event(lynx_view_t* view,
                                               const char* name,
                                               const char* json) {
  if (name == nullptr) {
    return;
  }
  view->lynx_template_renderer->SendGlobalEvent(
      name, lynx::lepus::jsonValueTolepusValue(json == nullptr ? "" : json));
}

LYNX_EXTERN_C void lynx_view_update_screen_metrics(lynx_view_t* view,
                                                   const float& width,
                                                   const float& height,
                                                   const float& pixel_ratio) {
  view->lynx_template_renderer->UpdateScreenMetrics(
      width * pixel_ratio, height * pixel_ratio, pixel_ratio);
  view->lynx_ui_renderer->SetPixelRatio(pixel_ratio);
}

LYNX_EXTERN_C void lynx_view_set_frame(lynx_view_t* view, const float& x,
                                       const float& y, const float& width,
                                       const float& height) {
  view->lynx_template_renderer->UpdateViewport(width, 1, height, 1, true);
  view->lynx_ui_renderer->SetFrame(x, y, width, height);
}

LYNX_EXTERN_C void lynx_view_set_font_scale(lynx_view_t* view,
                                            const float& font_scale) {
  view->lynx_template_renderer->UpdateFontScale(font_scale);
}

LYNX_EXTERN_C void lynx_view_set_parent(lynx_view_t* view,
                                        NativeWindow parent) {
  view->lynx_ui_renderer->SetParent(parent);
}

LYNX_EXTERN_C NativeWindow lynx_view_get_native_window(lynx_view_t* view) {
  return view->lynx_ui_renderer->GetNativeWindow();
}

LYNX_CAPI_EXPORT lynx_generic_resource_fetcher_t*
lynx_view_get_generic_resource_fetcher(lynx_view_t* view) {
  if (!view->resource_fetcher_holder) {
    return nullptr;
  }
  auto* fetcher = view->resource_fetcher_holder->GenericFetcher();
  if (fetcher) {
    // Manually increment the ref count before handing the pointer to the
    // outside world. The caller is now responsible for calling
    // release(lynx_generic_resource_fetcher_release).
    lynx_generic_resource_fetcher_ref(fetcher);
  }
  return fetcher;
}

LYNX_EXTERN_C void lynx_view_enter_foreground(lynx_view_t* view) {
  view->lynx_template_renderer->OnEnterForeground();
  view->lynx_ui_renderer->OnEnterForeground();
#if ENABLE_NAPI_BINDING
  view->extension_factory_->OnEnterForeground();
#endif
}

LYNX_EXTERN_C void lynx_view_enter_background(lynx_view_t* view) {
  view->lynx_template_renderer->OnEnterBackground();
  view->lynx_ui_renderer->OnEnterBackground();
#if ENABLE_NAPI_BINDING
  view->extension_factory_->OnEnterBackground();
#endif
}

LYNX_EXTERN_C void lynx_view_inject_bubble_event(lynx_view_t* view,
                                                 const char* params) {
  view->lynx_ui_renderer->InjectBubbleEvent(params);
}

LYNX_EXTERN_C void lynx_view_register_native_view(
    lynx_view_t* view, const char* name, lynx_native_view_creator creator,
    void* opaque) {
  view->lynx_ui_renderer->RegisterNativeView(name, creator, opaque);
}

LYNX_EXTERN_C void lynx_view_set_custom_vsync_monitor(
    lynx_view_t* view, lynx_vsync_monitor_t* monitor) {
  view->custom_vsync_monitor = monitor;
}

LYNX_EXTERN_C void lynx_view_register_ime_handler(lynx_view_t* view,
                                                  void* ime_handler,
                                                  void* opaque) {
  view->lynx_ui_renderer->RegisterIMEHandler(ime_handler, opaque);
}

LYNX_EXTERN_C void lynx_view_release(lynx_view_t* view) {
  view->user_data = nullptr;
#if ENABLE_NAPI_BINDING
  // Detach napi modules.
  view->lynx_module_manager->Detach();
  view->lynx_module_manager.reset();
  view->extension_factory_->OnLynxViewDestroy();
#endif
  // Destroy template renderer and ui renderer.
  view->lynx_template_renderer.reset();
  view->lynx_view_clients.reset();
  view->lynx_ui_renderer.reset();
  delete view;
}

namespace {
class CallbackEventSimulationProxy
    : public lynx::pub::LynxEventSimulationProxy {
 public:
  CallbackEventSimulationProxy(lynx_emulate_touch_fn cb, void* ctx)
      : callback_(cb), context_(ctx) {}

  void EmulateTouch(const std::string& event_type, int x, int y,
                    const std::string& button, float delta_x, float delta_y,
                    int modifiers, int click_count) override {
    if (callback_) {
      callback_(context_, event_type.c_str(), x, y, button.c_str(), delta_x,
                delta_y, modifiers, click_count);
    }
  }

 private:
  lynx_emulate_touch_fn callback_;
  void* context_;
};
}  // namespace

LYNX_EXTERN_C void lynx_view_set_event_simulation_proxy(
    lynx_view_t* view, lynx_emulate_touch_fn callback, void* context) {
  if (callback) {
    view->event_simulation_proxy =
        std::make_unique<CallbackEventSimulationProxy>(callback, context);
  } else {
    view->event_simulation_proxy.reset();
  }
  view->lynx_template_renderer->SetTemplateRendererEventSimulationProxy(
      view->event_simulation_proxy.get());
}

LYNX_EXTERN_C void lynx_view_send_touch_event(lynx_view_t* view,
                                              const char* name, int32_t id,
                                              float x, float y, float client_x,
                                              float client_y, float page_x,
                                              float page_y) {
  view->lynx_template_renderer->SendTouchEvent(name, id, x, y, client_x,
                                               client_y, page_x, page_y);
}

LYNX_EXTERN_C int lynx_view_get_node_for_location(lynx_view_t* view, int x,
                                                  int y) {
  return view->lynx_template_renderer->GetNodeForLocation(x, y);
}

LYNX_EXTERN_C void lynx_view_emulate_mouse_event(lynx_view_t* view,
                                                 const char* event_name,
                                                 float x, float y,
                                                 float delta_x, float delta_y) {
  view->lynx_ui_renderer->EmulateMouseEvent(event_name, x, y, delta_x, delta_y);
}
