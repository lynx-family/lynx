// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_COMMON_NAPI_NAPI_RUNTIME_PROXY_JSVM_H_
#define CORE_RUNTIME_COMMON_NAPI_NAPI_RUNTIME_PROXY_JSVM_H_

#include <ark_runtime/jsvm_types.h>

#include <memory>

#include "core/runtime/common/napi/napi_runtime_proxy.h"
#include "core/runtime/common/napi/napi_runtime_proxy_jsvm_factory.h"
#include "core/runtime/js/jsi/jsvm/jsvm_context_wrapper.h"

namespace lynx {
namespace piper {
class NapiRuntimeProxyJSVM : public NapiRuntimeProxy {
 public:
  static std::unique_ptr<NapiRuntimeProxy> Create(
      const std::shared_ptr<JSVMContextWrapper>& context,
      runtime::TemplateDelegate* delegate = nullptr);
  NapiRuntimeProxyJSVM(const std::shared_ptr<JSVMContextWrapper>& context,
                       runtime::TemplateDelegate* delegate);
  void Attach() override;
  void Detach() override;

 private:
  JSVM_Env jsvm_env_ = nullptr;
  JSVM_VM vm_ = nullptr;
  JSVM_VMScope vm_scope_ = nullptr;
};

class NapiRuntimeProxyJSVMFactoryImpl : public NapiRuntimeProxyJSVMFactory {
 public:
  std::unique_ptr<NapiRuntimeProxy> Create(
      std::shared_ptr<Runtime> runtime,
      runtime::TemplateDelegate* delegate = nullptr) override;
};
}  // namespace piper
}  // namespace lynx

#endif  // CORE_RUNTIME_COMMON_NAPI_NAPI_RUNTIME_PROXY_JSVM_H_
