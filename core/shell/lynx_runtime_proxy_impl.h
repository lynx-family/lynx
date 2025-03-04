// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_LYNX_RUNTIME_PROXY_IMPL_H_
#define CORE_SHELL_LYNX_RUNTIME_PROXY_IMPL_H_

#include <memory>
#include <string>
#include <utility>

#include "core/public/lynx_runtime_proxy.h"
#include "core/shell/lynx_shell.h"

namespace lynx {
namespace shell {
class LynxRuntimeProxyImpl : public LynxRuntimeProxy {
 public:
  explicit LynxRuntimeProxyImpl(
      std::shared_ptr<LynxActor<runtime::LynxRuntime>> actor)
      : actor_(std::move(actor)) {}
  LynxRuntimeProxyImpl(std::shared_ptr<LynxActor<runtime::LynxRuntime>> actor,
                       bool is_runtime_standalone_mode)
      : actor_(std::move(actor)),
        is_runtime_standalone_mode_(is_runtime_standalone_mode) {}
  ~LynxRuntimeProxyImpl() = default;

  void CallJSFunction(std::string module_id, std::string method_id,
                      std::unique_ptr<pub::Value> params) override;

  void CallJSApiCallbackWithValue(int32_t callback_id,
                                  std::unique_ptr<pub::Value> params) override;

  void CallJSIntersectionObserver(int32_t observer_id, int32_t callback_id,
                                  std::unique_ptr<pub::Value> params) override;

  void EvaluateScript(const std::string& url, std::string script,
                      int32_t callback_id) override;

  void RejectDynamicComponentLoad(const std::string& url, int32_t callback_id,
                                  int32_t err_code,
                                  const std::string& err_msg) override;

 protected:
  std::shared_ptr<LynxActor<runtime::LynxRuntime>> actor_;

 private:
  bool is_runtime_standalone_mode_ = false;
};
}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_LYNX_RUNTIME_PROXY_IMPL_H_
