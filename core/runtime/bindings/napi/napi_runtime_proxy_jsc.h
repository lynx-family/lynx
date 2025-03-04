// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_BINDINGS_NAPI_NAPI_RUNTIME_PROXY_JSC_H_
#define CORE_RUNTIME_BINDINGS_NAPI_NAPI_RUNTIME_PROXY_JSC_H_

#include <memory>

#include "core/runtime/bindings/napi/napi_runtime_proxy.h"
#include "core/runtime/jsi/jsc/jsc_context_wrapper.h"

namespace lynx {
namespace piper {

class NapiRuntimeProxyJSC : public NapiRuntimeProxy {
 public:
  static std::unique_ptr<NapiRuntimeProxy> Create(
      std::shared_ptr<JSCContextWrapper> context,
      runtime::TemplateDelegate *delegate = nullptr);
  NapiRuntimeProxyJSC(std::shared_ptr<JSCContextWrapper> context,
                      runtime::TemplateDelegate *delegate);

  void Attach() override;
  void Detach() override;

 private:
  // weak_ptr is a workaround for context leak in shared context mode.
  std::weak_ptr<JSCContextWrapper> context_;
};

}  // namespace piper
}  // namespace lynx

#endif  // CORE_RUNTIME_BINDINGS_NAPI_NAPI_RUNTIME_PROXY_JSC_H_
