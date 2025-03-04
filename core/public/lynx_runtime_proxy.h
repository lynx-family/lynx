// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_PUBLIC_LYNX_RUNTIME_PROXY_H_
#define CORE_PUBLIC_LYNX_RUNTIME_PROXY_H_

#include <memory>
#include <string>

#include "core/public/pub_value.h"

namespace lynx {
namespace shell {

class LynxRuntimeProxy {
 public:
  virtual ~LynxRuntimeProxy() = default;

  virtual void CallJSFunction(std::string module_id, std::string method_id,
                              std::unique_ptr<pub::Value> params) = 0;

  virtual void CallJSApiCallbackWithValue(
      int32_t callback_id, std::unique_ptr<pub::Value> params) = 0;

  virtual void CallJSIntersectionObserver(
      int32_t observer_id, int32_t callback_id,
      std::unique_ptr<pub::Value> params) = 0;

  virtual void EvaluateScript(const std::string& url, std::string script,
                              int32_t callback_id) = 0;

  virtual void RejectDynamicComponentLoad(const std::string& url,
                                          int32_t callback_id, int32_t err_code,
                                          const std::string& err_msg) = 0;
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_PUBLIC_LYNX_RUNTIME_PROXY_H_
