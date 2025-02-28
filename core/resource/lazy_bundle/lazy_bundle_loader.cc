// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/resource/lazy_bundle/lazy_bundle_loader.h"

#include <utility>

#include "base/include/timer/time_utils.h"
#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"
#include "core/build/gen/lynx_sub_error_code.h"
#include "core/shell/lynx_engine.h"
#ifdef OS_ANDROID
#include "core/runtime/jscache/js_cache_manager_facade.h"
#endif
#include "core/template_bundle/template_codec/binary_decoder/lynx_binary_reader.h"

namespace lynx {
namespace tasm {

namespace {
constexpr char kFormatErrorMessageBegin[] =
    "Load lazy bundle failed, the error message is: ";
constexpr char kEmptyBinaryErrorMessage[] = "template binary is empty";

std::string ConstructErrorMessage(const std::string& error_info) {
  return kFormatErrorMessageBegin + error_info;
}

void DecodeComponent(LazyBundleLoader::CallBackInfo& callback_info) {
  if (callback_info.bundle) {
    // if already got a template bundle object.
    return;
  }
  if (callback_info.Success()) {
    auto reader =
        LynxBinaryReader::CreateLynxBinaryReader(std::move(callback_info.data));
    reader.SetIsCardType(false);
    if (reader.Decode()) {
      callback_info.bundle = reader.GetTemplateBundle();
    } else {
      callback_info.error_code = error::E_LAZY_BUNDLE_LOAD_DECODE_FAILED;
      callback_info.error_msg =
          ConstructErrorMessage("Decoder error: " + reader.error_message_);
    }
  }
}
}  // namespace

void LazyBundleLoader::CallBackInfo::HandleError(
    const std::optional<std::string>& error) {
  if (error) {
    error_code = error::E_LAZY_BUNDLE_LOAD_BAD_RESPONSE;
    error_msg = ConstructErrorMessage(*error);
  } else if (bundle == std::nullopt && data.empty()) {
    // TODO(nihao.royal): add a new error_code for null bundle.
    error_code = error::E_LAZY_BUNDLE_LOAD_EMPTY_FILE;
    error_msg = ConstructErrorMessage(kEmptyBinaryErrorMessage);
  }
}

void LazyBundleLoader::DidLoadComponent(
    LazyBundleLoader::CallBackInfo callback_info) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "DynamicComponent::DidLoadComponent", "url",
              callback_info.component_url);
  callback_info.sync = SyncRequiring(callback_info.component_url);

  if (!callback_info.sync && enable_component_async_decode_) {
    DecodeComponent(callback_info);
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
      TRACE_EVENT(LYNX_TRACE_CATEGORY, "DynamicComponent::RequireTemplate",
                  "url", url);
      this->RequireTemplate(lazy_bundle, url, instance_id);
    }
    return true;
  } else {
    return false;
  }
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

  bool need_dispatch = false;
  auto iter = url_to_lifecycle_option_map_.find(url);

  for (const auto& option : iter->second) {
    need_dispatch = option->OnLazyBundleLifecycleEnd(tasm) || need_dispatch;
  }
  url_to_lifecycle_option_map_.erase(iter);

  return need_dispatch;
}

void LazyBundleLoader::RequireTemplate(RadonLazyComponent* lazy_bundle,
                                       const std::string& url,
                                       int instance_id) {
  if (!resource_loader_) {
    LOGE(
        "RequireTemplate:Use default implementation but resource_loader_ is "
        "null");
    return;
  }
  auto request =
      pub::LynxResourceRequest{url, pub::LynxResourceType::kTemplateLazyBundle};
  resource_loader_->LoadResource(
      request, true,
      [url, weak_self = weak_from_this(), lazy_bundle,
       instance_id](pub::LynxResourceResponse& response) {
        auto self = weak_self.lock();
        if (!self) {
          return;
        }
        std::optional<std::string> err_msg = std::nullopt;
        if (!response.Success()) {
          err_msg = std::move(response.err_msg);
        }
        std::optional<LynxTemplateBundle> bundle = std::nullopt;
        if (response.bundle != nullptr) {
          bundle = *static_cast<LynxTemplateBundle*>(response.bundle);
        }
        self->DidLoadComponent(LazyBundleLoader::CallBackInfo{
            std::move(url), std::move(response.data), std::move(bundle),
            err_msg, lazy_bundle, instance_id});
      });
}

/**
 * This method should be implemented at the platform layer
 * and callback LazyBundleLoader::DidPreloadTemplate
 */
void LazyBundleLoader::PreloadTemplates(const std::vector<std::string>& urls) {
  if (!resource_loader_) {
    LOGE(
        "PreloadTemplates:Use default implementation but resource_loader_ is "
        "null");
    return;
  }
  std::for_each(urls.begin(), urls.end(), [this](const auto& url) {
    auto request = pub::LynxResourceRequest{
        url, pub::LynxResourceType::kTemplateLazyBundle};
    resource_loader_->LoadResource(
        request, false,
        [url,
         weak_self = weak_from_this()](pub::LynxResourceResponse& response) {
          auto self = weak_self.lock();
          if (!self) {
            return;
          }
          std::optional<std::string> err_msg = std::nullopt;
          if (!response.Success()) {
            err_msg = std::move(response.err_msg);
          }
          std::optional<LynxTemplateBundle> bundle = std::nullopt;
          if (response.bundle != nullptr) {
            bundle = *static_cast<LynxTemplateBundle*>(response.bundle);
          }
          self->DidPreloadTemplate(LazyBundleLoader::CallBackInfo{
              std::move(url), std::move(response.data), std::move(bundle),
              std::move(err_msg)});
        });
  });
}

void LazyBundleLoader::DidPreloadTemplate(
    LazyBundleLoader::CallBackInfo callback_info) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "DynamicComponent::DidPreload", "url",
              callback_info.component_url);
  DecodeComponent(callback_info);

#ifdef OS_ANDROID
  // TODO(zhoupeng): Currently, there is no easy way to get JsEngineType, so
  // QUICK_JS is used by default. Fix it later.
  if (callback_info.bundle) {
    lynx::piper::cache::JsCacheManagerFacade::PostCacheGenerationTask(
        *callback_info.bundle, callback_info.component_url,
        lynx::piper::JSRuntimeType::quickjs);
  }
#endif

  if (engine_actor_) {
    engine_actor_->ActAsync(
        [callback_info = std::move(callback_info)](auto& engine) mutable {
          engine->DidPreloadComponent(std::move(callback_info));
        });
  }
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
}  // namespace tasm
}  // namespace lynx
