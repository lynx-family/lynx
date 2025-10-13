// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_LYNX_SERVICE_LYNX_SERVICE_BASE_H_
#define PLATFORM_EMBEDDER_LYNX_SERVICE_LYNX_SERVICE_BASE_H_

#include "base/include/fml/memory/ref_counted.h"

namespace lynx {
namespace embedder {
class LynxServiceBase : public fml::RefCountedThreadSafe<LynxServiceBase> {};
}  // namespace embedder
}  // namespace lynx

#endif  // PLATFORM_EMBEDDER_LYNX_SERVICE_LYNX_SERVICE_BASE_H_
