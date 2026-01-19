// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_TASM_PLATFORM_INVOKER_PROVIDER_H_
#define CORE_SHELL_TASM_PLATFORM_INVOKER_PROVIDER_H_

#include <memory>

#include "core/shell/tasm_platform_invoker.h"

namespace lynx {
namespace shell {

// Interface owned by the C++ core for creating platform-specific
// TasmPlatformInvoker instances on demand. Platform layers provide
// concrete implementations that capture the necessary platform
// context (JNI objects, Objective-C instances, etc.).
class TasmPlatformInvokerProvider {
 public:
  TasmPlatformInvokerProvider() = default;
  virtual ~TasmPlatformInvokerProvider() = default;

  TasmPlatformInvokerProvider(const TasmPlatformInvokerProvider&) = delete;
  TasmPlatformInvokerProvider& operator=(const TasmPlatformInvokerProvider&) =
      delete;

  // Create a new platform invoker instance.
  virtual std::unique_ptr<TasmPlatformInvoker> Create() = 0;
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_TASM_PLATFORM_INVOKER_PROVIDER_H_
