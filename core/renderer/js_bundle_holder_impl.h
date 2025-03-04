// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_JS_BUNDLE_HOLDER_IMPL_H_
#define CORE_RENDERER_JS_BUNDLE_HOLDER_IMPL_H_

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

#include "base/include/fml/synchronization/shared_mutex.h"
#include "core/runtime/piper/js/js_bundle_holder.h"

namespace lynx {
namespace tasm {

/**
 * Provide background thread with an interface to get lazy bundles' js
 * content.
 * `GetJSBundleFromBT` will be invoked in js thread and other methods
 * will be invoked in tasm thread.
 */
class JsBundleHolderImpl : public piper::JsBundleHolder {
 public:
  JsBundleHolderImpl() = default;
  ~JsBundleHolderImpl() override = default;

  std::optional<piper::JsBundle> GetJSBundleFromBT(
      const std::string& url) override;

  void InsertJSBundle(const std::string& url, const piper::JsBundle& js_bundle);

  void SetEnable(bool enable);

  class RequestScope {
   public:
    RequestScope(const std::string& target_url, JsBundleHolderImpl& holder);
    ~RequestScope();
    RequestScope(const RequestScope&) = delete;
    RequestScope& operator=(const RequestScope&) = delete;
    RequestScope(RequestScope&&) = delete;
    RequestScope& operator=(RequestScope&&) = delete;

   private:
    JsBundleHolderImpl& holder_;
  };

  std::unique_ptr<RequestScope> CreateRequestScope(const std::string& url);

 private:
  std::optional<piper::JsBundle> GetJSBundleInternal(const std::string& url);

  bool IsRequesting(const std::string& url);

  std::unordered_map<std::string, piper::JsBundle> js_bundle_map_{};

  std::atomic<bool> enable_{false};
  std::mutex mutex_;
  std::condition_variable request_cv_;
  std::optional<std::string> requesting_url_{std::nullopt};

  friend class RequestScope;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_JS_BUNDLE_HOLDER_IMPL_H_
