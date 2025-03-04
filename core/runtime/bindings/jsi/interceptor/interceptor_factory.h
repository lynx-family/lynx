// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_BINDINGS_JSI_INTERCEPTOR_INTERCEPTOR_FACTORY_H_
#define CORE_RUNTIME_BINDINGS_JSI_INTERCEPTOR_INTERCEPTOR_FACTORY_H_

#include <memory>
#include <string>

#include "core/public/prop_bundle.h"
#include "core/runtime/bindings/jsi/modules/lynx_module.h"
#include "lynx/base/include/debug/lynx_error.h"

namespace lynx {
namespace piper {

class InterceptorFactory {
 public:
  InterceptorFactory() = delete;
  ~InterceptorFactory() = delete;

  static std::shared_ptr<GroupInterceptor> CreateGroupInterceptor();
};

}  // namespace piper
}  // namespace lynx

#endif  // CORE_RUNTIME_BINDINGS_JSI_INTERCEPTOR_INTERCEPTOR_FACTORY_H_
