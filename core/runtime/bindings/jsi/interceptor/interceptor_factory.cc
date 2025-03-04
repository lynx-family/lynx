// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/bindings/jsi/interceptor/interceptor_factory.h"

namespace lynx {
namespace piper {

std::shared_ptr<GroupInterceptor> InterceptorFactory::CreateGroupInterceptor() {
  return nullptr;
}

}  // namespace piper
}  // namespace lynx
