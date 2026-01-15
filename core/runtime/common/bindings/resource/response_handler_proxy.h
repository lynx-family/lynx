// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_COMMON_BINDINGS_RESOURCE_RESPONSE_HANDLER_PROXY_H_
#define CORE_RUNTIME_COMMON_BINDINGS_RESOURCE_RESPONSE_HANDLER_PROXY_H_

#include <future>
#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "core/resource/lazy_bundle/bundle_resource_info.h"
#include "core/runtime/common/bindings/resource/response_promise.h"

namespace lynx {
namespace runtime {

class ResponseHandlerProxy {
 public:
  class Delegate {
   public:
    Delegate() = default;
    virtual ~Delegate() = default;
    virtual void InvokeResponsePromiseCallback(base::closure closure) = 0;
  };

  ResponseHandlerProxy(
      Delegate& delegate, const std::string& url,
      const std::shared_ptr<runtime::ResponsePromise<tasm::BundleResourceInfo>>&
          promise)
      : delegate_(delegate), url_(url), promise_(promise) {}

  virtual ~ResponseHandlerProxy() = default;

  /**
   * Retrieve the result of promise
   * if promise get a std::nullopt, it will return with code
   * `LYNX_BUNDLE_RESOURCE_INFO_TIMEOUT`
   */
  tasm::BundleResourceInfo WaitAndGetResource(double timeout) {
    auto result = promise_->Wait(timeout);
    if (result.has_value()) {
      return *result;
    }
    return tasm::BundleResourceInfo(
        {.url = url_, .code = tasm::LYNX_BUNDLE_RESOURCE_INFO_TIMEOUT});
  }

  /**
   * Add Listener for ResponsePromise
   */
  virtual void AddResourceListener(
      base::MoveOnlyClosure<void, tasm::BundleResourceInfo> closure) = 0;

 protected:
  Delegate& delegate_;

  std::string url_;
  std::shared_ptr<runtime::ResponsePromise<tasm::BundleResourceInfo>> promise_;
};

}  // namespace runtime
}  // namespace lynx

#endif  // CORE_RUNTIME_COMMON_BINDINGS_RESOURCE_RESPONSE_HANDLER_PROXY_H_
