// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_PUBLIC_LYNX_VIEW_H_
#define PLATFORM_EMBEDDER_PUBLIC_LYNX_VIEW_H_

#include <algorithm>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "capi/lynx_view_builder_capi.h"
#include "capi/lynx_view_capi.h"
#include "lynx_generic_resource_fetcher.h"
#include "lynx_group.h"
#include "lynx_load_meta.h"
#include "lynx_native_module.h"
#include "lynx_runtime_lifecycle_observer.h"
#include "lynx_update_meta.h"
#include "lynx_view_client.h"
#include "lynx_windowless_renderer.h"

namespace lynx {
namespace pub {

/**
 * @apidoc
 * @brief LynxView is the main entry point for rendering and interacting with
 * Lynx. It provides methods for loading and rendering templates, updating data,
 * and sending events.
 */
class LynxView {
 public:
  class Builder {
   public:
    Builder() : builder_(lynx_view_builder_create()) {}
    ~Builder() { lynx_view_builder_release(builder_); }

    Builder& SetScreenSize(float width, float height, float pixel_ratio) {
      lynx_view_builder_set_screen_size(builder_, width, height, pixel_ratio);
      return *this;
    }

    Builder& SetFrame(float x, float y, float width, float height) {
      lynx_view_builder_set_frame(builder_, x, y, width, height);
      return *this;
    }

    Builder& SetFontScale(float scale) {
      lynx_view_builder_set_font_scale(builder_, scale);
      return *this;
    }

    Builder& SetICUDataPath(const std::string& icu_data_path) {
      lynx_view_builder_set_icu_data_path(builder_, icu_data_path.c_str());
      return *this;
    }

    Builder& SetLynxGroup(std::shared_ptr<LynxGroup> group) {
      group_ = std::move(group);
      lynx_view_builder_set_lynx_group(builder_, group_->Impl());
      return *this;
    }

    Builder& SetParent(NativeWindow parent) {
      lynx_view_builder_set_parent(builder_, parent);
      return *this;
    }
    Builder& SetWindowlessRenderer(
        std::shared_ptr<LynxWindowlessRenderer> renderer) {
      renderer->InitIfNeeded();
      lynx_view_builder_set_windowless_renderer(builder_, renderer->Impl());
      windowless_renderer_ = renderer;
      return *this;
    }
    Builder& SetGenericResourceFetcher(
        std::shared_ptr<LynxGenericResourceFetcher> fetcher) {
      fetcher->InitIfNeeded();
      lynx_view_builder_set_generic_resource_fetcher(builder_, fetcher->Impl());
      generic_fetcher_ = fetcher;
      return *this;
    }

    Builder& RegisterNativeModule(const std::string& module_name,
                                  napi_module_creator creator, void* opaque) {
      lynx_view_builder_register_native_module(builder_, module_name.c_str(),
                                               creator, opaque);
      return *this;
    }

    template <typename T>
    Builder& RegisterNativeView(const std::string& tag_name,
                                void* opaque = nullptr) {
      lynx_view_builder_register_native_view(
          builder_, tag_name.c_str(),
          [](void* opaque) -> lynx_native_view_t* {
            return (new T(opaque))->native_view();
          },
          opaque);
      return *this;
    }

    lynx_view_builder_t* Impl() { return builder_; }

    std::unique_ptr<LynxView> Build() {
      std::unique_ptr<LynxView> view = std::make_unique<LynxView>(this);
      view->generic_fetcher_ = std::move(generic_fetcher_);
      view->windowless_renderer_ = std::move(windowless_renderer_);
      return view;
    }

    Builder(const Builder&) = delete;
    Builder& operator=(const Builder&) = delete;

   private:
    lynx_view_builder_t* builder_;
    std::shared_ptr<LynxGroup> group_;
    std::shared_ptr<LynxWindowlessRenderer> windowless_renderer_;
    std::shared_ptr<LynxGenericResourceFetcher> generic_fetcher_;
    friend class LynxView;
  };

  static LynxView* Unwrap(lynx_view_t* view) {
    if (view == nullptr) {
      return nullptr;
    }
    return static_cast<LynxView*>(lynx_view_get_user_data(view));
  }

  explicit LynxView(Builder* builder)
      : lynx_view_(lynx_view_create(builder->builder_, this)) {}

  virtual ~LynxView() { lynx_view_release(lynx_view_); }

  /**
   * @apidoc
   * @brief Register a native view for the `LynxView`.
   * @param tag_name The tag name of element, specifying exists tag name will
   * override corresponding builtin element.
   * @param opaque An opaque pointer to user-specific data.
   */
  template <typename T>
  LynxView& RegisterNativeView(const std::string& tag_name,
                               void* opaque = nullptr) {
    lynx_view_register_native_view(
        lynx_view_, tag_name.c_str(),
        [](void* opaque) -> lynx_native_view_t* {
          return (new T(opaque))->native_view();
        },
        opaque);
    return *this;
  }

  /**
   * @apidoc
   * @brief Register lifecycle event observer `LynxViewClient` for the
   * `LynxView`.
   * @param client The structure implemented by the client and registered to
   * the `LynxView` instance is used to obtain the callbacks of each process
   * in the `LynxView` lifecycle.
   */
  void AddClient(std::shared_ptr<LynxViewClient> client) {
    if (std::find(lynx_view_clients_.begin(), lynx_view_clients_.end(),
                  client) == lynx_view_clients_.end()) {
      lynx_view_add_client(lynx_view_, client->Impl());
      lynx_view_clients_.emplace_back(client);
    }
  }

  /**
   * @apidoc
   * @brief Remove `LynxView` lifecycle event monitoring.
   * @param client The structure implemented by the client and registered to the
   * `LynxView` instance is used to obtain the callbacks of each process in the
   * LynxView life cycle.
   */
  void RemoveClient(std::shared_ptr<LynxViewClient> client) {
    lynx_view_remove_client(lynx_view_, client->Impl());
    lynx_view_clients_.erase(std::remove(lynx_view_clients_.begin(),
                                         lynx_view_clients_.end(), client),
                             lynx_view_clients_.end());
  }

  void RegisterRuntimeLifecycleObserver(
      std::shared_ptr<LynxRuntimeLifecycleObserver> observer) {
    observer->InitIfNeeded();
    lynx_view_register_runtime_lifecycle_observer(lynx_view_, observer->Impl());
    runtime_lifecycle_observer_ = observer;
  }

  /**
   * @apidoc
   * @brief Using `LynxLoadMeta` to render `LynxView`, it is the main entrance
   * for the client to load `Lynx` templates.
   * @param load_meta Lynx template loads metadata, and developers use this data
   * structure to specify optional data loaded by the template.
   */
  void LoadTemplate(std::shared_ptr<LynxLoadMeta> load_meta) {
    lynx_view_load_template(lynx_view_, load_meta->Impl());
  }

  /**
   * @apidoc
   * @brief Using `LynxUpdateMeta` to update `LynxView`, it is the main entrance
   * for the client to update template data.
   * @param meta Lynx template update metadata, developers use this data
   * structure to specify the optional data content of the update template.
   */
  void UpdateData(std::shared_ptr<LynxUpdateMeta> update_meta) {
    lynx_view_update_data(lynx_view_, update_meta->Impl());
  }

  /**
   * @apidoc
   * @brief Reload the `LynxView` template with new data.
   * @param data The template data to be loaded.
   * @param global_props The global properties of the template.
   */
  void ReloadTemplate(std::shared_ptr<LynxTemplateData> data,
                      std::shared_ptr<LynxTemplateData> global_props) {
    lynx_view_reload_template(lynx_view_, data->Impl(),
                              global_props ? global_props->Impl() : nullptr);
  }

  /**
   * @apidoc
   * @brief Send global events to the front end through the client, and the
   * front end can listen to the event through `GlobalEventEmitter`.
   * @param name Global event name.
   * @param params Parameters carried by global events.
   */
  void SendGlobalEvent(const std::string& name, const std::string& json) {
    lynx_view_send_global_event(lynx_view_, name.c_str(), json.c_str());
  }

  /**
   * @apidoc
   * @brief Update the screen metrics of the `LynxView`.
   * @param width The width of the screen.
   * @param height The height of the screen.
   * @param pixel_ratio The pixel ratio of the screen.
   */
  void UpdateScreenMetrics(float width, float height, float pixel_ratio) {
    lynx_view_update_screen_metrics(lynx_view_, width, height, pixel_ratio);
  }

  /**
   * @apidoc
   * @brief Set the position and size of the `LynxView` in the window.
   * @param x The x coordinate of the `LynxView` position.
   * @param y The y coordinate of the `LynxView` position.
   * @param width The width of the `LynxView`.
   * @param height The height of the `LynxView`.
   */
  void SetFrame(float x, float y, float width, float height) {
    lynx_view_set_frame(lynx_view_, x, y, width, height);
  }

  /**
   * @apidoc
   * @brief Changing the font scaling ratio in client settings will
   * automatically change the text size.
   * @param scale The new font scaling ratio.
   */
  void SetFontScale(float scale) {
    lynx_view_set_font_scale(lynx_view_, scale);
  }

  /**
   * @apidoc
   * @brief Set the parent window of the `LynxView`.
   * @param parent The parent window of the `LynxView`.
   */
  void SetParent(NativeWindow parent) {
    lynx_view_set_parent(lynx_view_, parent);
  }

  NativeWindow GetNativeWindow() {
    return lynx_view_get_native_window(lynx_view_);
  }

  /**
   * @apidoc
   * @brief Notify the `LynxView` that it has entered the foreground.
   */
  void OnEnterForeground() { lynx_view_enter_foreground(lynx_view_); }

  /**
   * @apidoc
   * @brief Notify the `LynxView` that it has entered the background.
   */
  void OnEnterBackground() { lynx_view_enter_background(lynx_view_); }

  void InjectBubbleEvent(const std::string& params) {
    lynx_view_inject_bubble_event(lynx_view_, params.c_str());
  }

  lynx_view_t* Impl() { return lynx_view_; }

  LynxView(const LynxView&) = delete;
  LynxView& operator=(const LynxView&) = delete;

 private:
  lynx_view_t* lynx_view_;
  std::shared_ptr<LynxGenericResourceFetcher> generic_fetcher_;
  std::vector<std::shared_ptr<LynxViewClient>> lynx_view_clients_;
  std::shared_ptr<LynxWindowlessRenderer> windowless_renderer_;
  std::shared_ptr<LynxRuntimeLifecycleObserver> runtime_lifecycle_observer_;
};

}  // namespace pub
}  // namespace lynx

#endif  // PLATFORM_EMBEDDER_PUBLIC_LYNX_VIEW_H_
