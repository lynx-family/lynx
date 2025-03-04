// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "lynx/core/renderer/js_bundle_holder_impl.h"

#include "base/include/log/logging.h"

namespace lynx {
namespace tasm {
// called in DidDecodeTemplate, when JS has not been started
void JsBundleHolderImpl::SetEnable(bool enable) { enable_.store(enable); }

std::optional<piper::JsBundle> JsBundleHolderImpl::GetJSBundleFromBT(
    const std::string& url) {
  if (!enable_.load()) {
    return std::nullopt;
  }
  std::unique_lock<std::mutex> lock(mutex_);
  auto bundle = GetJSBundleInternal(url);
  if (bundle) {
    return bundle;
  }

  // check if this url is being requested
  constexpr std::chrono::milliseconds kRequestTimeOut{500};
  bool wait_for_request =
      request_cv_.wait_for(lock, kRequestTimeOut,
                           [this, &url]() { return !this->IsRequesting(url); });
  if (!wait_for_request) {
    // if is not being requested, return nullopt
    LOGE("Wait JsBundleHolderImpl::GetJSBundleFromBT timeout, url" << url);
    return std::nullopt;
  }
  // read the ready bundles
  bundle = GetJSBundleInternal(url);
  return bundle;
}

bool JsBundleHolderImpl::IsRequesting(const std::string& url) {
  return requesting_url_ && *requesting_url_ == url;
}

std::optional<piper::JsBundle> JsBundleHolderImpl::GetJSBundleInternal(
    const std::string& url) {
  auto bundle = js_bundle_map_.find(url);
  return bundle != js_bundle_map_.end() ? std::make_optional(bundle->second)
                                        : std::nullopt;
}

void JsBundleHolderImpl::InsertJSBundle(const std::string& url,
                                        const piper::JsBundle& js_bundle) {
  // will dismiss the JsBundle of DEFAULT_ENTRY_NAME, but it will not be got by
  // js so it's acceptable
  if (!enable_.load()) {
    return;
  }
  std::unique_lock<std::mutex> lock(mutex_);
  js_bundle_map_.emplace(url, js_bundle);
}

std::unique_ptr<JsBundleHolderImpl::RequestScope>
JsBundleHolderImpl::CreateRequestScope(const std::string& url) {
  return enable_.load() ? std::make_unique<RequestScope>(url, *this) : nullptr;
}

JsBundleHolderImpl::RequestScope::RequestScope(const std::string& url,
                                               JsBundleHolderImpl& holder)
    : holder_(holder) {
  std::unique_lock<std::mutex> lock(holder_.mutex_);
  holder_.requesting_url_ = url;
}

JsBundleHolderImpl::RequestScope::~RequestScope() {
  {
    std::unique_lock<std::mutex> lock(holder_.mutex_);
    holder_.requesting_url_ = std::nullopt;
  }
  holder_.request_cv_.notify_all();
}

}  // namespace tasm
}  // namespace lynx
