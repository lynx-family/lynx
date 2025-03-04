// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_BINDINGS_NAPI_NAPI_RUNTIME_PROXY_QUICKJS_FACTORY_H_
#define CORE_RUNTIME_BINDINGS_NAPI_NAPI_RUNTIME_PROXY_QUICKJS_FACTORY_H_

#include <memory>

#include "core/runtime/bindings/napi/napi_runtime_proxy.h"

namespace lynx {
namespace piper {

// Used by DevTool on iOS.
class NapiRuntimeProxyQuickjsFactory {
 public:
  BASE_EXPORT virtual std::unique_ptr<NapiRuntimeProxy> Create(
      std::shared_ptr<Runtime> runtime,
      runtime::TemplateDelegate *delegate = nullptr) = 0;
};

}  // namespace piper
}  // namespace lynx

#endif  // CORE_RUNTIME_BINDINGS_NAPI_NAPI_RUNTIME_PROXY_QUICKJS_FACTORY_H_
