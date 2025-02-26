// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/resource/external_resource/external_resource_loader.h"

#include <future>
#include <tuple>
#include <utility>

#include "base/include/log/logging.h"
#include "core/build/gen/lynx_sub_error_code.h"
#include "core/runtime/common/js_error_reporter.h"
#include "core/template_bundle/lynx_template_bundle.h"

namespace lynx {
namespace shell {

ExternalResourceInfo ExternalResourceLoader::LoadScript(const std::string& url,
                                                        long timeout) {
  if (!resource_loader_) {
    auto error_msg = "LoadScript:resource_loader_ is null";
    LOGE(error_msg);
    return ExternalResourceInfo(
        error::E_RESOURCE_EXTERNAL_RESOURCE_REQUEST_FAILED,
        std::move(error_msg));
  }
  std::promise<ExternalResourceInfo> promise;
  std::future<ExternalResourceInfo> future = promise.get_future();
  auto request =
      pub::LynxResourceRequest{url, pub::LynxResourceType::kExternalJs};
  resource_loader_->LoadResource(
      request, true,
      [promise =
           std::move(promise)](pub::LynxResourceResponse& response) mutable {
        promise.set_value(ExternalResourceInfo(std::move(response.data),
                                               response.err_code,
                                               std::move(response.err_msg)));
      });
  timeout = timeout > 0 ? timeout : 5;
  if (future.wait_for(std::chrono::seconds(timeout)) !=
      std::future_status::ready) {
    return ExternalResourceInfo(
        error::E_RESOURCE_EXTERNAL_RESOURCE_REQUEST_FAILED, "timeout");
  }
  return future.get();
}

void ExternalResourceLoader::LoadScriptAsync(const std::string& url,
                                             int32_t callback_id) {
  if (!resource_loader_) {
    LOGE("LoadScriptAsync:resource_loader_ is null");
    return;
  }
  auto request =
      pub::LynxResourceRequest{url, pub::LynxResourceType::kExternalJs};
  resource_loader_->LoadResource(
      request, true,
      [url, callback_id,
       weak_self = weak_from_this()](pub::LynxResourceResponse& response) {
        auto self = weak_self.lock();
        if (!self) {
          LOGI("LoadScriptAsync:self is null");
          return;
        }
        auto runtime_actor = self->runtime_actor_.lock();
        if (!runtime_actor) {
          LOGI("LoadScriptAsync:runtime_actor is null");
          return;
        }

        std::string script(response.data.begin(), response.data.end());
        runtime_actor->Act([url, script = std::move(script),
                            callback_id](auto& runtime) mutable {
          runtime->EvaluateScript(url, std::move(script),
                                  piper::ApiCallBack(callback_id));
        });
      });
}

void ExternalResourceLoader::LoadLazyBundle(const std::string& url,
                                            int32_t callback_id) {
  LoadLazyBundle(url, callback_id, {});
}

void ExternalResourceLoader::LoadLazyBundle(
    const std::string& url, int32_t callback_id,
    const std::vector<std::string>& ids) {
  if (!resource_loader_) {
    LOGE("LoadLazyBundle:resource_loader_ is null");
    return;
  }
  auto request =
      pub::LynxResourceRequest{url, pub::LynxResourceType::kLazyBundle};
  resource_loader_->LoadResource(
      request, true,
      [url, callback_id, component_ids = ids, weak_self = weak_from_this()](
          pub::LynxResourceResponse& response) mutable {
        auto self = weak_self.lock();
        if (!self) {
          LOGI("LoadLazyBundle:self is null");
          return;
        }

        // use LazyBundleLoader::CallBackInfo to handle error code and
        // error message
        std::optional<std::string> error =
            response.Success()
                ? std::nullopt
                : std::optional<std::string>(std::move(response.err_msg));

        std::optional<tasm::LynxTemplateBundle> bundle = std::nullopt;
        if (response.bundle != nullptr) {
          bundle = *static_cast<tasm::LynxTemplateBundle*>(response.bundle);
        }

        // TODO(nihao.royal): we pass sync: true here is just for compatibility,
        // in needed sync is not meaningfull here.
        auto callback_info =
            tasm::LazyBundleLoader::CallBackInfo{std::move(url),
                                                 std::move(response.data),
                                                 std::move(bundle),
                                                 error,
                                                 true,
                                                 callback_id,
                                                 std::move(component_ids)};

        if (callback_info.Success()) {
          auto engine_actor = self->engine_actor_.lock();
          if (!engine_actor) {
            LOGI("LoadLazyBundle:engine_actor is null");
            return;
          }
          engine_actor->Act(
              [callback_info = std::move(callback_info)](auto& engine) mutable {
                engine->DidLoadComponentFromJS(std::move(callback_info));
              });
        } else {
          auto runtime_actor = self->runtime_actor_.lock();
          if (!runtime_actor) {
            LOGI("LoadLazyBundle:runtime_actor is null");
            return;
          }
          runtime_actor->Act([callback_info = std::move(callback_info),
                              callback_id](auto& runtime) mutable {
            auto lynx_error = base::LynxError{callback_info.error_code,
                                              callback_info.error_msg};
            common::FormatErrorUrl(lynx_error, callback_info.component_url);
            runtime->OnErrorOccurred(std::move(lynx_error));

            runtime->CallJSApiCallbackWithValue(
                piper::ApiCallBack(callback_id),
                tasm::lazy_bundle::ConstructErrorMessageForBTS(
                    callback_info.component_url, callback_info.error_code,
                    callback_info.error_msg));
          });
        }
      });
}

std::vector<uint8_t> ExternalResourceLoader::LoadJSSource(
    const std::string& url) {
  std::promise<std::vector<uint8_t>> promise;
  std::future<std::vector<uint8_t>> future = promise.get_future();
  auto request = pub::LynxResourceRequest{
      .url = url, .type = pub::LynxResourceType::kAssets};
  resource_loader_->LoadResource(
      request, true,
      [promise =
           std::move(promise)](pub::LynxResourceResponse& response) mutable {
        promise.set_value(std::move(response.data));
      });
  return future.get();
}

}  // namespace shell
}  // namespace lynx
