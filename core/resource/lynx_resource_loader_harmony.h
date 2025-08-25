// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RESOURCE_LYNX_RESOURCE_LOADER_HARMONY_H_
#define CORE_RESOURCE_LYNX_RESOURCE_LOADER_HARMONY_H_

#include <js_native_api.h>
#include <js_native_api_types.h>
#include <rawfile/raw_file_manager.h>

#include <memory>
#include <string>
#include <utility>

#include "base/include/fml/task_runner.h"
#include "core/public/lynx_resource_loader.h"

namespace lynx {
namespace harmony {

class LynxResourceLoaderHarmony : public pub::LynxResourceLoader {
 public:
  class CallbackHandler {
   public:
    CallbackHandler(
        base::MoveOnlyClosure<void, pub::LynxResourceResponse&> callback,
        pub::ResourceLoadTiming timing, std::string url = "")
        : callback_(std::move(callback)),
          timing_(std::move(timing)),
          url_(std::move(url)) {}

    CallbackHandler(
        base::MoveOnlyClosure<void, pub::LynxPathResponse&> path_callback,
        pub::ResourceLoadTiming timing, std::string url)
        : path_callback_(std::move(path_callback)),
          timing_(std::move(timing)),
          url_(std::move(url)) {}

    CallbackHandler(std::shared_ptr<pub::LynxStreamDelegate> stream_delegate,
                    std::string url)
        : stream_delegate_(std::move(stream_delegate)), url_(std::move(url)) {}

    // callback for handling ArrayBuffer result.
    static napi_value HandleRequestCallback(napi_env env,
                                            napi_callback_info info);
    // callack for handling TemplateProviderResult result.
    static napi_value HandleTemplateRequestCallback(napi_env env,
                                                    napi_callback_info info);

    // callback for handling Path result.
    static napi_value HandlePathRequestCallback(napi_env env,
                                                napi_callback_info info);

    static napi_value HandleStreamOnStartCallback(napi_env env,
                                                  napi_callback_info info);
    static napi_value HandleStreamOnDataCallback(napi_env env,
                                                 napi_callback_info info);
    static napi_value HandleStreamOnEndCallback(napi_env env,
                                                napi_callback_info info);
    static napi_value HandleStreamOnErrorCallback(napi_env env,
                                                  napi_callback_info info);

   private:
    friend LynxResourceLoaderHarmony;
    base::MoveOnlyClosure<void, pub::LynxResourceResponse&> callback_;
    base::MoveOnlyClosure<void, pub::LynxPathResponse&> path_callback_;
    std::shared_ptr<pub::LynxStreamDelegate> stream_delegate_;
    pub::ResourceLoadTiming timing_;
    std::string url_;
  };

  LynxResourceLoaderHarmony(napi_env env, napi_value providers);
  ~LynxResourceLoaderHarmony();

  void SetUITaskRunner(const fml::RefPtr<fml::TaskRunner>& task_runner) {
    ui_task_runner_ = task_runner;
  }

  void LoadResource(const pub::LynxResourceRequest& request,
                    base::MoveOnlyClosure<void, pub::LynxResourceResponse&>
                        callback) override;

  void LoadResourcePath(const pub::LynxResourceRequest& request,
                        base::MoveOnlyClosure<void, pub::LynxPathResponse&>
                            path_callback) override;

  virtual void LoadStream(
      const pub::LynxResourceRequest& request,
      const std::shared_ptr<pub::LynxStreamDelegate>& stream_delegate) override;

  bool IsLocalResource(const std::string& url) override;
  std::string ShouldRedirectUrl(
      const pub::LynxResourceRequest& request) override;

  void DeleteRef();

  static NativeResourceManager* resource_manager;

 private:
  void PostTaskOnUIThread(base::closure task);

  napi_env env_;
  napi_ref providers_ref_;
  napi_ref generic_fetcher_;
  napi_ref media_fetcher_;
  napi_ref template_fetcher_;

  static void BuildResourceRequestValue(napi_env env, napi_value& request,
                                        const std::string& url, int32_t type);

  static void BuildStreamResourceDelegate(
      napi_env env, napi_value& delegate, const std::string& url,
      const std::shared_ptr<pub::LynxStreamDelegate>& stream_delegate);

  fml::RefPtr<fml::TaskRunner> ui_task_runner_ = nullptr;
};

}  // namespace harmony
}  // namespace lynx

#endif  // CORE_RESOURCE_LYNX_RESOURCE_LOADER_HARMONY_H_
