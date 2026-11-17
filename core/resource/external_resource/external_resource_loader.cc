// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/resource/external_resource/external_resource_loader.h"

#include <future>
#include <tuple>
#include <utility>

#include "base/include/log/logging.h"
#include "core/build/gen/lynx_sub_error_code.h"
#include "core/public/lynx_resource_handle.h"
#include "core/resource/trace/resource_trace_event_def.h"
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
  auto promise = std::make_shared<std::promise<ExternalResourceInfo>>();
  std::future<ExternalResourceInfo> future = promise->get_future();
  auto request =
      pub::LynxResourceRequest{url, pub::LynxResourceType::kExternalJs};
  resource_loader_->LoadResource(
      request,
      [promise_weak = std::weak_ptr<std::promise<ExternalResourceInfo>>(
           promise)](pub::LynxResourceResponse& response) mutable {
        auto p = promise_weak.lock();
        if (!p) {
          return;
        }
        p->set_value(ExternalResourceInfo(std::move(response.data),
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
    LOGE("LoadScriptAsync::resource_loader_ is null");
    return;
  }
  auto request =
      pub::LynxResourceRequest{url, pub::LynxResourceType::kExternalJs};
  resource_loader_->LoadResource(
      request, [url, callback_id, weak_self = weak_from_this()](
                   pub::LynxResourceResponse& response) {
        auto self = weak_self.lock();
        if (!self) {
          LOGI("LoadScriptAsync::self is null");
          return;
        }
        auto runtime_actor = self->runtime_actor_.lock();
        if (!runtime_actor) {
          LOGI("LoadScriptAsync::runtime_actor is null");
          return;
        }

        std::string script(response.data.begin(), response.data.end());
        runtime_actor->Act([url, script = std::move(script),
                            error = response.err_msg,
                            callback_id](auto& runtime) mutable {
          runtime->OnScriptLoaded(url, std::move(script), std::move(error),
                                  runtime::js::ApiCallBack(callback_id));
        });
      });
}

ExternalResourceInfo ExternalResourceLoader::LoadByteCode(
    const std::string& url, long timeout) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, LOAD_BYTE_CODE, "source", url);
  if (!resource_loader_) {
    LOGE(
        "[ResourceHandle] External bytecode failure_stage=dispatch, "
        "failure_reason=resource_loader_unavailable");
    auto error_msg = "LoadByteCode: resource_loader_ is null.";
    return ExternalResourceInfo(
        error::E_RESOURCE_EXTERNAL_RESOURCE_REQUEST_FAILED,
        std::move(error_msg));
  }

  auto promise = std::make_shared<std::promise<ExternalResourceInfo>>();
  std::future<ExternalResourceInfo> future = promise->get_future();
  auto request =
      pub::LynxResourceRequest{url, pub::LynxResourceType::kExternalByteCode};
  resource_loader_->LoadBytecode(
      request,
      [promise_weak = std::weak_ptr<std::promise<ExternalResourceInfo>>(
           promise)](pub::LynxResourceResponse& response) mutable {
        auto p = promise_weak.lock();
        if (!p) {
          LOGE(
              "[ResourceHandle] External bytecode "
              "failure_stage=fetch_callback, failure_reason=late_callback");
          return;
        }
        if (!response.Success()) {
          LOGE(
              "[ResourceHandle] External bytecode "
              "failure_stage=fetch_callback, failure_reason=request_failed, "
              "error_code="
              << response.err_code);
          p->set_value(ExternalResourceInfo(std::move(response.data),
                                            response.err_code,
                                            std::move(response.err_msg)));
          return;
        }

        if (response.resource_handle != nullptr) {
          TRACE_EVENT_INSTANT(LYNX_TRACE_CATEGORY, EXTERNAL_BYTECODE_LOAD_PATH,
                              "load_path", "resource_handle",
                              "platform_bytes_copy_avoided", "true");
          LOGI(
              "[ResourceHandle] External bytecode load_path=resource_handle, "
              "platform_bytes_copy_avoided=true");
          auto result = [&response]() {
            TRACE_EVENT(LYNX_TRACE_CATEGORY,
                        EXTERNAL_BYTECODE_READ_RESOURCE_HANDLE);
            return response.resource_handle->ReadAllBytes();
          }();
          if (!result.has_value()) {
            LOGE(
                "[ResourceHandle] External bytecode "
                "failure_stage=handle_read, failure_reason=read_failed");
            p->set_value(ExternalResourceInfo(
                error::E_RESOURCE_EXTERNAL_RESOURCE_REQUEST_FAILED,
                std::move(result.error())));
            return;
          }
          if (result.value().empty()) {
            LOGE(
                "[ResourceHandle] External bytecode "
                "failure_stage=handle_read, failure_reason=empty_resource");
            p->set_value(ExternalResourceInfo(
                error::E_RESOURCE_EXTERNAL_RESOURCE_REQUEST_FAILED,
                "external bytecode resource is empty"));
            return;
          }
          response.data = std::move(result.value());
          LOGI("[ResourceHandle] External bytecode read success, byte_size="
               << response.data.size());
        } else {
          TRACE_EVENT_INSTANT(LYNX_TRACE_CATEGORY, EXTERNAL_BYTECODE_LOAD_PATH,
                              "load_path", "bytes",
                              "platform_bytes_copy_avoided", "false");
          LOGI(
              "[ResourceHandle] External bytecode load_path=bytes, "
              "platform_bytes_copy_avoided=false, byte_size="
              << response.data.size());
        }

        p->set_value(ExternalResourceInfo(std::move(response.data),
                                          response.err_code,
                                          std::move(response.err_msg)));
      });

  timeout = timeout > 0 ? timeout : 5;
  if (future.wait_for(std::chrono::seconds(timeout)) !=
      std::future_status::ready) {
    LOGE(
        "[ResourceHandle] External bytecode failure_stage=fetch_wait, "
        "failure_reason=timeout");
    return ExternalResourceInfo(
        error::E_RESOURCE_EXTERNAL_RESOURCE_REQUEST_FAILED,
        "loadByteCode timeout");
  }
  return future.get();
}

void ExternalResourceLoader::LoadLazyBundle(const std::string& url,
                                            int32_t callback_id) {
  LoadLazyBundle(url, callback_id, {});
}

void ExternalResourceLoader::LoadLazyBundle(std::string url,
                                            int32_t callback_id,
                                            std::vector<std::string> ids) {
  if (!resource_loader_) {
    LOGE("LoadLazyBundle:resource_loader_ is null");
    return;
  }
  auto request =
      pub::LynxResourceRequest{url, pub::LynxResourceType::kLazyBundle};
  resource_loader_->LoadResource(
      request, [url = std::move(url), callback_id,
                component_ids = std::move(ids), weak_self = weak_from_this()](
                   pub::LynxResourceResponse& response) mutable {
        auto self = weak_self.lock();
        if (!self) {
          LOGI("LoadLazyBundle:self is null");
          return;
        }

        // Use LazyBundleLoader::CallBackInfo to handle status code and message.
        auto status = tasm::LazyBundleLoader::CallBackInfo::CreateStatusInfo(
            response.err_code, std::move(response.err_msg));

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
                                                 std::move(status),
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
            runtime::FormatErrorUrl(lynx_error, callback_info.component_url);
            runtime->OnErrorOccurred(std::move(lynx_error));

            runtime->CallJSApiCallbackWithValue(
                runtime::js::ApiCallBack(callback_id),
                tasm::lazy_bundle::ConstructErrorMessageForBTS(
                    callback_info.component_url, callback_info.error_code,
                    callback_info.error_msg));
          });
        }
      });
}

std::vector<uint8_t> ExternalResourceLoader::LoadJSSource(
    const std::string& url) {
  auto promise = std::make_shared<std::promise<std::vector<uint8_t>>>();
  std::future<std::vector<uint8_t>> future = promise->get_future();
  auto request = pub::LynxResourceRequest{
      .url = url, .type = pub::LynxResourceType::kAssets};
  resource_loader_->LoadResource(
      request,
      [promise_weak = std::weak_ptr<std::promise<std::vector<uint8_t>>>(
           promise)](pub::LynxResourceResponse& response) mutable {
        auto p = promise_weak.lock();
        if (!p) {
          return;
        }
        p->set_value(std::move(response.data));
      });
  return future.get();
}

}  // namespace shell
}  // namespace lynx
