// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_BINDINGS_NAPI_NAPI_RUNTIME_PROXY_QUICKJS_H_
#define CORE_RUNTIME_BINDINGS_NAPI_NAPI_RUNTIME_PROXY_QUICKJS_H_

#include <memory>

#include "core/runtime/bindings/napi/napi_runtime_proxy.h"

struct LEPUSContext;

namespace lynx {
namespace piper {

class NapiRuntimeProxyQuickjs : public NapiRuntimeProxy {
 public:
  static std::unique_ptr<NapiRuntimeProxy> Create(
      LEPUSContext* context, runtime::TemplateDelegate* delegate = nullptr);
  NapiRuntimeProxyQuickjs(LEPUSContext* context,
                          runtime::TemplateDelegate* delegate);

  void Attach() override;
  void Detach() override;

 private:
  LEPUSContext* context_ = nullptr;
};

}  // namespace piper
}  // namespace lynx

#endif  // CORE_RUNTIME_BINDINGS_NAPI_NAPI_RUNTIME_PROXY_QUICKJS_H_
