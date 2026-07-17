// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/resource/lazy_bundle/lazy_bundle_loader.h"

#include <algorithm>
#include <chrono>
#include <future>
#include <utility>

#include "base/include/log/logging.h"
#include "base/include/timer/time_utils.h"
#include "base/trace/native/trace_event.h"
#include "core/build/gen/lynx_sub_error_code.h"
#include "core/resource/lazy_bundle/lazy_bundle_utils.h"
#include "core/resource/trace/resource_trace_event_def.h"
#include "core/runtime/common/js_error_reporter.h"
#include "core/shell/lynx_engine.h"
#include "core/shell/runtime/bts/bts_runtime.h"
#ifdef OS_ANDROID
#include "core/runtime/js/bytecode/js_cache_manager_facade.h"
#endif

namespace lynx {
namespace tasm {

namespace {
constexpr char kFormatErrorMessageBegin[] =
    "Load lazy bundle failed, the error message is: ";
constexpr char kEmptyBinaryErrorMessage[] = "template binary is empty";
constexpr char kUnknownResourceErrorMessage[] = "resource request failed";

std::string ConstructErrorMessage(const std::string& error_info) {
  return kFormatErrorMessageBegin + error_info;
}

void DecodeBundle(LazyBundleLoader::CallBackInfo& callback_info, bool is_card) {
  if (callback_info.bundle) {
    // if already got a template bundle object.
    return;
  }
  if (callback_info.Success()) {
    lynx::tasm::LynxTemplateBundle bundle;
    std::string error = bundle.FromBinaryGreedy(std::move(callback_info.data),
                                                "", false, is_card);
    if (error.empty()) {
      callback_info.bundle = std::move(bundle);
    } else {
      callback_info.error_code = error::E_LAZY_BUNDLE_LOAD_DECODE_FAILED;
      callback_info.error_msg =
          ConstructErrorMessage("Decoder error: " + error);
    }
  }
}
}  // namespace

void LazyBundleLoader::CallBackInfo::HandleStatus(
    std::optional<StatusInfo> status) {
  if (status) {
    error_code = status->code > 0 ? status->code
                                  : error::E_LAZY_BUNDLE_LOAD_BAD_RESPONSE;
    error_msg = ConstructErrorMessage(status->message.empty()
                                          ? kUnknownResourceErrorMessage
                                          : status->message);
  } else if (bundle == std::nullopt && data.empty()) {
    // TODO(nihao.royal): add a new error_code for null bundle.
    error_code = error::E_LAZY_BUNDLE_LOAD_EMPTY_FILE;
    error_msg = ConstructErrorMessage(kEmptyBinaryErrorMessage);
  }
}

void LazyBundleLoader::DidLoadComponent(
    LazyBundleLoader::CallBackInfo callback_info) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, DYNAMIC_COMPONENT_DID_LOAD_COMPONENT, "url",
              callback_info.component_url);
  callback_info.sync = SyncRequiring(callback_info.component_url);

  if (!callback_info.sync && enable_component_async_decode_) {
    DecodeBundle(callback_info, false);
  }

  if (callback_info.Success() && callback_info.bundle) {
    InsertTemplateBundle(callback_info.component_url, *callback_info.bundle);
  }

  if (engine_actor_) {
    engine_actor_->Act(
        [this, callback_info = std::move(callback_info)](auto& engine) mutable {
          EndRecordRequireTime(callback_info);
          // require end. remove from requiring urls.
          requiring_urls_.erase(callback_info.component_url);
          engine->DidLoadComponent(std::move(callback_info));
        });
  }
}

bool LazyBundleLoader::RequireTemplateCollected(RadonLazyComponent* lazy_bundle,
                                                const std::string& url,
                                                int instance_id) {
  // The return value indicates whether a request was actually sent.
  if (requiring_urls_.find(url) == requiring_urls_.end()) {
    StartRecordRequireTime(url);
    {
      TRACE_EVENT(LYNX_TRACE_CATEGORY, DYNAMIC_COMPONENT_REQUIRE_TEMPLATE,
                  "url", url);
      this->RequireTemplate(lazy_bundle, url, instance_id);
    }
    return true;
  } else {
    return false;
  }
}

void LazyBundleLoader::LoadFrameBundle(const std::string& src) {
  auto requiring_res = requiring_urls_.emplace(src);
  // request with the same src will only be sent once
  if (!requiring_res.second) {
    return;
  }

  // Check if the bundle was pre-registered via registerDynamicComponent
  auto preloaded_bundle = GetTemplateBundle(src);
  if (preloaded_bundle.has_value()) {
    // Hit cache, dispatch directly without network request
    LOGE("Load Frame Bundle with preloaded bundle. Src： " << src.c_str());
    LazyBundleLoader::CallBackInfo callback_info(
        src, std::vector<uint8_t>(), std::move(preloaded_bundle), std::nullopt);
    lazy_bundle::LynxLazyBundleRequest request{
        .url = src, .resource_type = pub::LynxResourceType::kFrame};
    callback_info.request = std::move(request);
    DidFetchBundle(std::move(callback_info));
    return;
  }

  if (!resource_loader_) {
    LOGE("failed to query bundle, resource_loader is null, src: " << src);
    return;
  }
  lazy_bundle::LynxLazyBundleRequest request{
      .url = src, .resource_type = pub::LynxResourceType::kFrame};
  FetchBundle(std::move(request));
}

void LazyBundleLoader::MarkComponentLoading(const std::string& url) {
  requiring_urls_.emplace(url);
}

void LazyBundleLoader::AppendUrlToLifecycleOptionMap(
    const std::string& url,
    std::unique_ptr<LazyBundleLifecycleOption> lifecycle_option) {
  auto& options = url_to_lifecycle_option_map_[url];
  // sync some information of previous options if need
  if (!options.empty()) {
    lifecycle_option->SyncOption(**options.cbegin());
  }
  options.emplace_back(std::move(lifecycle_option));
}

bool LazyBundleLoader::DispatchOnComponentLoaded(TemplateAssembler* tasm,
                                                 const std::string& url) {
  DCHECK(engine_actor_->CanRunNow());
  auto iter = url_to_lifecycle_option_map_.find(url);
  if (iter == url_to_lifecycle_option_map_.end()) {
    return false;
  }

  // TODO(nihao.royal): add test case for nested query component cases.
  auto option_handle = url_to_lifecycle_option_map_.extract(url);
  if (option_handle.empty()) {
    return false;
  }

  bool need_dispatch = false;
  for (const auto& option : option_handle.mapped()) {
    need_dispatch = option->OnLazyBundleLifecycleEnd(tasm) || need_dispatch;
    // send LazyBundleEntry
    if (perf_controller_actor_ != nullptr) {
      auto lazyBundleEntry = option->GetLazyBundleEntry();
      if (lazyBundleEntry != nullptr) {
        perf_controller_actor_->ActAsync(
            [entry = std::move(lazyBundleEntry)](auto& performance) mutable {
              performance->OnPerformanceEvent(std::move(entry),
                                              tasm::performance::kEventTypeAll);
            });
      }
    }
  }

  return need_dispatch;
}

void LazyBundleLoader::RequireTemplate(RadonLazyComponent* lazy_bundle,
                                       const std::string& url,
                                       int instance_id) {
  auto cached_bundle = GetTemplateBundle(url);
  if (cached_bundle) {
    DidLoadComponent(CallBackInfo{url,
                                  {},
                                  std::move(cached_bundle),
                                  std::nullopt,
                                  lazy_bundle,
                                  instance_id});
    return;
  }

  if (!resource_loader_) {
    LOGE(
        "RequireTemplate:Use default implementation but resource_loader_ is "
        "null");
    return;
  }
  auto request =
      pub::LynxResourceRequest{url, pub::LynxResourceType::kLazyBundle};
  resource_loader_->LoadResource(
      request, [url, weak_self = weak_from_this(), lazy_bundle,
                instance_id](pub::LynxResourceResponse& response) {
        auto self = weak_self.lock();
        if (!self) {
          return;
        }
        auto status = LazyBundleLoader::CallBackInfo::CreateStatusInfo(
            response.err_code, std::move(response.err_msg));
        std::optional<LynxTemplateBundle> bundle = std::nullopt;
        if (response.bundle != nullptr) {
          bundle = *static_cast<LynxTemplateBundle*>(response.bundle);
        }
        self->DidLoadComponent(LazyBundleLoader::CallBackInfo{
            std::move(url), std::move(response.data), std::move(bundle),
            std::move(status), lazy_bundle, instance_id});
      });
}

/**
 * This method should be implemented at the platform layer
 * and callback LazyBundleLoader::DidFetchBundle
 */
void LazyBundleLoader::PreloadTemplates(const std::vector<std::string>& urls) {
  std::for_each(urls.begin(), urls.end(), [this](const auto& url) {
    lazy_bundle::LynxLazyBundleRequest request{.url = url};
    FetchBundle(request);
  });
}

void LazyBundleLoader::FetchBundle(
    lazy_bundle::LynxLazyBundleRequest bundle_request) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, LAZY_BUNDLE_FETCH_BUNDLE, "url",
              bundle_request.url);

  auto cached_bundle = GetTemplateBundle(bundle_request.url);
  if (cached_bundle) {
    auto callback_info = CallBackInfo{
        bundle_request.url, {}, std::move(cached_bundle), std::nullopt};
    callback_info.request = std::move(bundle_request);
    DidFetchBundle(std::move(callback_info));
    return;
  }

  if (!resource_loader_) {
    LOGE("LazyBundleLoader::FetchBundle fails, error: no resource loader, url: "
         << bundle_request.url);
    if (bundle_request.response_promise) {
      bundle_request.response_promise->SetValue(
          {.url = std::move(bundle_request.url),
           .code = LYNX_BUNDLE_RESOURCE_INFO_REQUEST_FAILED});
    }
    return;
  }
  auto request = pub::LynxResourceRequest{bundle_request.url,
                                          pub::LynxResourceType::kLazyBundle};
  resource_loader_->LoadResource(
      request, [bundle_request = std::move(bundle_request),
                weak_self = weak_from_this()](
                   pub::LynxResourceResponse& response) mutable {
        auto self = weak_self.lock();
        if (!self) {
          return;
        }
        auto status = LazyBundleLoader::CallBackInfo::CreateStatusInfo(
            response.err_code, std::move(response.err_msg));
        std::optional<LynxTemplateBundle> bundle = std::nullopt;
        if (response.bundle != nullptr) {
          bundle = *static_cast<LynxTemplateBundle*>(response.bundle);
        }
        auto callback_info = LazyBundleLoader::CallBackInfo{
            bundle_request.url, std::move(response.data), std::move(bundle),
            std::move(status)};
        callback_info.request = std::move(bundle_request);
        self->DidFetchBundle(std::move(callback_info));
      });
}

void LazyBundleLoader::DidFetchBundle(
    LazyBundleLoader::CallBackInfo callback_info) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, LAZY_BUNDLE_DID_FETCH_BUNDLE, "url",
              callback_info.component_url);

#ifdef OS_ANDROID
  // TODO(zhoupeng): Currently, there is no easy way to get JsEngineType, so
  // QUICK_JS is used by default. Fix it later.
  if (callback_info.bundle) {
    lynx::runtime::js::cache::JsCacheManagerFacade::PostCacheGenerationTask(
        *callback_info.bundle, callback_info.component_url,
        lynx::runtime::js::JSRuntimeType::quickjs);
  }
#endif
  DecodeBundle(callback_info, callback_info.request.resource_type ==
                                  pub::LynxResourceType::kFrame);
  if (callback_info.Success() && callback_info.bundle) {
    // Make the decoded bundle available to every loader that shares the same
    // BundleManager before notifying either actor.
    InsertTemplateBundle(callback_info.request.url, *callback_info.bundle);
    // notify response promise;
    if (callback_info.request.response_promise) {
      callback_info.request.response_promise->SetValue(
          {.url = callback_info.request.url,
           .code = LYNX_BUNDLE_RESOURCE_INFO_SUCCESS});
    }
  } else {
    // bundle fetched failed here, notify response promise with error.
    if (callback_info.request.response_promise) {
      callback_info.request.response_promise->SetValue(
          {.url = callback_info.request.url,
           .code = LYNX_BUNDLE_RESOURCE_INFO_REQUEST_FAILED});
    }
  }

  if (engine_actor_) {
    engine_actor_->Act(
        [callback_info = std::move(callback_info)](auto& engine) mutable {
          engine->DidFetchBundle(std::move(callback_info));
        });
  }
}

shell::ExternalResourceInfo LazyBundleLoader::LoadScript(const std::string& url,
                                                         long timeout) {
  if (!resource_loader_) {
    auto error_msg = "LoadScript:resource_loader_ is null";
    LOGE(error_msg);
    return shell::ExternalResourceInfo(
        error::E_RESOURCE_EXTERNAL_RESOURCE_REQUEST_FAILED,
        std::move(error_msg));
  }
  auto promise = std::make_shared<std::promise<shell::ExternalResourceInfo>>();
  std::future<shell::ExternalResourceInfo> future = promise->get_future();
  auto request =
      pub::LynxResourceRequest{url, pub::LynxResourceType::kExternalJs};
  resource_loader_->LoadResource(
      request,
      [promise_weak = std::weak_ptr<std::promise<shell::ExternalResourceInfo>>(
           promise)](pub::LynxResourceResponse& response) mutable {
        auto p = promise_weak.lock();
        if (!p) {
          return;
        }
        p->set_value(shell::ExternalResourceInfo(std::move(response.data),
                                                 response.err_code,
                                                 std::move(response.err_msg)));
      });
  timeout = timeout > 0 ? timeout : 5;
  if (future.wait_for(std::chrono::seconds(timeout)) !=
      std::future_status::ready) {
    return shell::ExternalResourceInfo(
        error::E_RESOURCE_EXTERNAL_RESOURCE_REQUEST_FAILED, "timeout");
  }
  return future.get();
}

void LazyBundleLoader::LoadScriptAsync(const std::string& url,
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

shell::ExternalResourceInfo LazyBundleLoader::LoadByteCode(
    const std::string& url, long timeout) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, LOAD_BYTE_CODE, "source", url);
  if (!resource_loader_) {
    auto error_msg = "LoadByteCode: resource_loader_ is null.";
    return shell::ExternalResourceInfo(
        error::E_RESOURCE_EXTERNAL_RESOURCE_REQUEST_FAILED,
        std::move(error_msg));
  }

  auto promise = std::make_shared<std::promise<shell::ExternalResourceInfo>>();
  std::future<shell::ExternalResourceInfo> future = promise->get_future();
  auto request =
      pub::LynxResourceRequest{url, pub::LynxResourceType::kExternalByteCode};
  resource_loader_->LoadBytecode(
      request,
      [promise_weak = std::weak_ptr<std::promise<shell::ExternalResourceInfo>>(
           promise)](pub::LynxResourceResponse& response) mutable {
        auto p = promise_weak.lock();
        if (!p) {
          return;
        }
        p->set_value(shell::ExternalResourceInfo(std::move(response.data),
                                                 response.err_code,
                                                 std::move(response.err_msg)));
      });

  timeout = timeout > 0 ? timeout : 5;
  if (future.wait_for(std::chrono::seconds(timeout)) !=
      std::future_status::ready) {
    return shell::ExternalResourceInfo(
        error::E_RESOURCE_EXTERNAL_RESOURCE_REQUEST_FAILED,
        "loadByteCode timeout");
  }
  return future.get();
}

void LazyBundleLoader::LoadLazyBundle(const std::string& url,
                                      int32_t callback_id) {
  LoadLazyBundle(url, callback_id, {});
}

void LazyBundleLoader::LoadLazyBundle(std::string url, int32_t callback_id,
                                      std::vector<std::string> component_ids) {
  auto cached_bundle = GetTemplateBundle(url);
  if (cached_bundle) {
    DidLoadComponentFromJS(CallBackInfo{std::move(url),
                                        {},
                                        std::move(cached_bundle),
                                        std::nullopt,
                                        true,
                                        callback_id,
                                        std::move(component_ids)});
    return;
  }

  if (!resource_loader_) {
    LOGE("LoadLazyBundle:resource_loader_ is null");
    return;
  }
  auto request =
      pub::LynxResourceRequest{url, pub::LynxResourceType::kLazyBundle};
  resource_loader_->LoadResource(
      request,
      [url = std::move(url), callback_id,
       component_ids = std::move(component_ids), weak_self = weak_from_this()](
          pub::LynxResourceResponse& response) mutable {
        auto self = weak_self.lock();
        if (!self) {
          LOGI("LoadLazyBundle:self is null");
          return;
        }

        auto status = CallBackInfo::CreateStatusInfo(
            response.err_code, std::move(response.err_msg));
        std::optional<LynxTemplateBundle> bundle = std::nullopt;
        if (response.bundle != nullptr) {
          bundle = *static_cast<LynxTemplateBundle*>(response.bundle);
        }

        self->DidLoadComponentFromJS(CallBackInfo{
            std::move(url), std::move(response.data), std::move(bundle),
            std::move(status), true, callback_id, std::move(component_ids)});
      });
}

void LazyBundleLoader::DidLoadComponentFromJS(
    LazyBundleLoader::CallBackInfo callback_info) {
  if (callback_info.Success()) {
    if (callback_info.bundle) {
      InsertTemplateBundle(callback_info.component_url, *callback_info.bundle);
    }
    if (!engine_actor_) {
      LOGI("LoadLazyBundle:engine_actor is null");
      return;
    }
    engine_actor_->Act(
        [callback_info = std::move(callback_info)](auto& engine) mutable {
          engine->DidLoadComponentFromJS(std::move(callback_info));
        });
    return;
  }

  auto runtime_actor = runtime_actor_.lock();
  if (!runtime_actor) {
    LOGI("LoadLazyBundle:runtime_actor is null");
    return;
  }
  const auto callback_id = callback_info.callback_id;
  runtime_actor->Act([callback_info = std::move(callback_info),
                      callback_id](auto& runtime) mutable {
    auto lynx_error =
        base::LynxError{callback_info.error_code, callback_info.error_msg};
    runtime::FormatErrorUrl(lynx_error, callback_info.component_url);
    runtime->OnErrorOccurred(std::move(lynx_error));

    runtime->CallJSApiCallbackWithValue(
        runtime::js::ApiCallBack(callback_id),
        lazy_bundle::ConstructErrorMessageForBTS(callback_info.component_url,
                                                 callback_info.error_code,
                                                 callback_info.error_msg));
  });
}

std::vector<uint8_t> LazyBundleLoader::LoadJSSource(const std::string& url) {
  if (!resource_loader_) {
    LOGE("LoadJSSource:resource_loader_ is null");
    return {};
  }
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

bool LazyBundleLoader::SyncRequiring(const std::string& url) {
  // running on TASM thread and not in requiring_urls_
  return engine_actor_ != nullptr && engine_actor_->CanRunNow() &&
         requiring_urls_.count(url) == 0;
}

void LazyBundleLoader::StartRecordRequireTime(const std::string& url) {
  DCHECK(engine_actor_->CanRunNow());
  uint64_t time = base::CurrentSystemTimeMilliseconds();
  for (const auto& option : url_to_lifecycle_option_map_[url]) {
    option->start_require_time = time;
  }
}

void LazyBundleLoader::EndRecordRequireTime(const CallBackInfo& callback_info) {
  DCHECK(engine_actor_->CanRunNow());
  std::string url = callback_info.component_url;
  uint64_t time = base::CurrentSystemTimeMilliseconds();
  for (const auto& option : url_to_lifecycle_option_map_[url]) {
    option->sync = callback_info.sync;
    option->end_require_time = time;
    if (callback_info.Success()) {
      option->binary_size = callback_info.data.size();
    }
  }
}

void LazyBundleLoader::StartRecordDecodeTime(const std::string& url) {
  DCHECK(engine_actor_->CanRunNow());
  uint64_t time = base::CurrentSystemTimeMilliseconds();
  for (const auto& option : url_to_lifecycle_option_map_[url]) {
    option->start_decode_time = time;
  }
}

void LazyBundleLoader::EndRecordDecodeTime(const std::string& url) {
  DCHECK(engine_actor_->CanRunNow());
  uint64_t time = base::CurrentSystemTimeMilliseconds();
  for (const auto& option : url_to_lifecycle_option_map_[url]) {
    option->end_decode_time = time;
  }
}

void LazyBundleLoader::MarkComponentLoadedFailed(
    const std::string& url, int32_t error_code, const lepus::Value& error_msg) {
  DCHECK(engine_actor_->CanRunNow());
  for (const auto& option : url_to_lifecycle_option_map_[url]) {
    option->is_success = false;
    option->error_code = error_code;
    option->message = error_msg;
  }
}

void LazyBundleLoader::MarkComponentLoadedSuccess(
    const std::string& url, const lepus::Value& success_msg) {
  DCHECK(engine_actor_->CanRunNow());
  for (const auto& option : url_to_lifecycle_option_map_[url]) {
    option->is_success = true;
    option->message = success_msg;
  }
}

lepus::Value LazyBundleLoader::GetPerfInfo(const std::string& url) {
  DCHECK(engine_actor_->CanRunNow());
  auto options = url_to_lifecycle_option_map_.find(url);
  if (options != url_to_lifecycle_option_map_.end() &&
      !options->second.empty()) {
    return options->second.front()->GetPerfInfo();
  }
  return lepus::Value();
}

void LazyBundleLoader::InsertTemplateBundle(const std::string& url,
                                            LynxTemplateBundle bundle) {
  LOGE("LazyBundleLoader::InsertTemplateBundle: " << url.c_str()
                                                  << "this: " << this);
  TRACE_EVENT(LYNX_TRACE_CATEGORY, LAZY_BUNDLE_LOADER_INSERT_BUNDLE, "url",
              url);

  std::lock_guard<std::mutex> lock(bundle_manager_mutex_);
  bundle_manager_->InsertTemplateBundle(url, std::move(bundle));
}

std::optional<LynxTemplateBundle> LazyBundleLoader::GetTemplateBundle(
    const std::string& url) {
  LOGE("LazyBundleLoader::GetTemplateBundle: " << url.c_str()
                                               << "this: " << this);
  std::lock_guard<std::mutex> lock(bundle_manager_mutex_);
  return bundle_manager_->GetTemplateBundle(url);
}

std::shared_ptr<BundleManager> LazyBundleLoader::GetBundleManager() const {
  std::lock_guard<std::mutex> lock(bundle_manager_mutex_);
  return bundle_manager_;
}

void LazyBundleLoader::SetBundleManager(
    std::shared_ptr<BundleManager> bundle_manager) {
  if (!bundle_manager) {
    return;
  }

  std::lock_guard<std::mutex> lock(bundle_manager_mutex_);
  bundle_manager->MergeFrom(*bundle_manager_);
  bundle_manager_ = std::move(bundle_manager);
}

}  // namespace tasm
}  // namespace lynx
